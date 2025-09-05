/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file DvbChannel.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include <unistd.h>
#include <errno.h>
#include <cstring>

#include <opensand_output/Output.h>

#include "DvbChannel.h"

#include "DvbFifo.h"
#include "Plugin.h"
#include "DvbS2Std.h"
#include "PhysicStd.h"
#include "EncapPlugin.h"
#include "FifoElement.h"
#include "TerminalCategoryDama.h"
#include "TerminalCategorySaloha.h"

std::shared_ptr<OutputLog> DvbChannel::dvb_fifo_log = nullptr;

DvbChannel::DvbChannel(StackPlugin *upper_encap, const std::string &name) : req_burst_length(0),
																			super_frame_counter(0),
																			fwd_down_frame_duration(),
																			ret_up_frame_duration(),
																			pkt_hdl(nullptr),
																			upper_encap(upper_encap),
																			stats_period_ms(),
																			stats_period_frame(),
																			check_send_stats(0)
{
	// register static log
	auto output = Output::Get();
	dvb_fifo_log = output->registerLog(LEVEL_WARNING, "Dvb.FIFO");
	this->log_init_channel = output->registerLog(LEVEL_WARNING, "Dvb." + name + ".Channel.init");
	this->log_receive_channel = output->registerLog(LEVEL_WARNING, "Dvb." + name + ".Channel.receive");
	this->log_send_channel = output->registerLog(LEVEL_WARNING, "Dvb." + name + ".Channel.send");
};

bool DvbChannel::initModcodDefinitionTypes(void)
{
	// Set the MODCOD definition type
	vol_sym_t dummy = 0;

	if (!OpenSandModelConf::Get()->getRcs2BurstLength(dummy))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"cannot get RCS2 burst length value");
		return false;
	}
	this->req_burst_length = dummy;

	LOG(this->log_init_channel, LEVEL_NOTICE,
		"required burst length = %d\n",
		this->req_burst_length);

	return true;
}

bool DvbChannel::initPktHdl(EncapSchemeList encap_schemes,
							EncapPlugin *&pkt_hdl)
{
	auto encap_conf = OpenSandModelConf::Get()->getProfileData()->getComponent("encap");
	std::string encap_plugin;
	switch (encap_schemes)
	{
	case EncapSchemeList::FORWARD_DOWN:
		encap_plugin = "GSE";
		break;

	case EncapSchemeList::RETURN_SCPC:
		encap_plugin = "GSE";
		break;

	case EncapSchemeList::RETURN_UP:
		encap_plugin = "RLE";
		break;

	case EncapSchemeList::TRANSPARENT_NO_SCHEME:
		LOG(this->log_init_channel, LEVEL_INFO,
			"Skipping packet handler initialization for "
			"transparent satellite");
		return true;

	default:
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Unknown encap schemes link: '%d'\n",
			encap_schemes);
		return false;
	}

	EncapPlugin *plugin;
	if (!Plugin::getEncapsulationPlugin(encap_plugin, &plugin))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"cannot get plugin for %s encapsulation\n",
			encap_plugin);
		return false;
	}

	pkt_hdl = plugin->getSharedPlugin();
	if (!pkt_hdl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"cannot get %s packet handler\n", encap_plugin.c_str());
		return false;
	}

	return true;
}

void DvbChannel::setFilterTalId(tal_id_t filter)
{
	uint8_t filter_u8 = (uint8_t)(filter & 0xFF);
	this->pkt_hdl->setFilterTalId(filter_u8);
}

bool DvbChannel::initCommon(EncapSchemeList encap_schemes)
{
	auto Conf = OpenSandModelConf::Get();

	//********************************************************
	//         init Common values from sections
	//********************************************************
	// frame duration
	if (!Conf->getReturnFrameDuration(this->ret_up_frame_duration))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"missing parameter 'return link frame duration'\n");
		return false;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
		"frame duration set to %uμs\n",
		this->ret_up_frame_duration.count());

	if (!this->initPktHdl(encap_schemes, this->pkt_hdl))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"failed to initialize packet handler\n");
		return false;
	}

	// statistics timer
	if (!Conf->getStatisticsPeriod(this->stats_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"missing parameter 'statistics period'\n");
		return false;
	}

	return true;
}

void DvbChannel::initStatsTimer(time_us_t frame_duration)
{
	// convert the pediod into a number of frames here to be
	// precise when computing statistics
	this->stats_period_frame = std::max(1L, this->stats_period_ms / frame_duration);
	LOG(this->log_init_channel, LEVEL_NOTICE,
		"statistics_timer set to %d, converted into %d frame(s)\n",
		this->stats_period_ms.count(), this->stats_period_frame);
	this->stats_period_ms = std::chrono::duration_cast<time_ms_t>(this->stats_period_frame * frame_duration);
}

bool DvbChannel::pushInFifo(DvbFifo &fifo,
							Rt::Ptr<NetContainer> data,
							time_ms_t fifo_delay)
{
	std::string data_name = data->getName();

	// append the data in the fifo
	if (!fifo.push(std::move(data), fifo_delay))
	{
		LOG(DvbChannel::dvb_fifo_log, LEVEL_ERROR,
			"FIFO is full: drop data\n");
		return false;
	}

	LOG(DvbChannel::dvb_fifo_log, LEVEL_NOTICE,
		"%s data stored in FIFO %s (delay = %ums)\n",
		data_name, fifo.getName(), fifo_delay.count());

	return true;
}

bool DvbChannel::doSendStats(void)
{
	bool res = (this->check_send_stats == 0);
	this->check_send_stats = (this->check_send_stats + 1) % this->stats_period_frame;
	return res;
}

/***** DvbFmt ****/
DvbFmt::DvbFmt() : input_sts(nullptr),
				   s2_modcod_def(),
				   output_sts(nullptr),
				   rcs_modcod_def(),
				   log_fmt(nullptr)
{
	this->log_fmt = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Fmt.Channel");
}

bool DvbFmt::initModcodDefFile(ModcodDefFileType def, FmtDefinitionTable &modcod_def, vol_sym_t req_burst_length)
{
	auto Conf = OpenSandModelConf::Get();
	std::vector<OpenSandModelConf::fmt_definition_parameters> modcod_params;

	switch (def)
	{
	case MODCOD_DEF_S2:
		if (!Conf->getS2WaveFormsDefinition(modcod_params))
		{
			LOG(this->log_fmt, LEVEL_ERROR,
				"failed to load the MODCOD definitions for S2 waveforms\n");
			return false;
		}
		for (auto &param : modcod_params)
		{
			if (!modcod_def.add(std::make_unique<FmtDefinition>(param.id,
																param.modulation_type,
																param.coding_type,
																param.spectral_efficiency,
																param.required_es_no)))
			{
				LOG(this->log_fmt, LEVEL_ERROR,
					"failed to create MODCOD table for S2 waveforms\n");
				return false;
			}
		}
		return true;
	case MODCOD_DEF_RCS2:
		if (!Conf->getRcs2WaveFormsDefinition(modcod_params, req_burst_length))
		{
			LOG(this->log_fmt, LEVEL_ERROR,
				"failed to load the MODCOD definitions for RCS2 waveforms\n");
			return false;
		}
		for (auto &param : modcod_params)
		{
			if (!modcod_def.add(std::make_unique<FmtDefinition>(param.id,
																param.modulation_type,
																param.coding_type,
																param.spectral_efficiency,
																param.required_es_no,
																req_burst_length)))
			{
				LOG(this->log_fmt, LEVEL_ERROR,
					"failed to create MODCOD table for RCS2 waveforms\n");
				return false;
			}
		}
		return true;
	default:
		LOG(this->log_fmt, LEVEL_ERROR,
			"modcod definition file type '%s' is unknown\n",
			def);
		return false;
	}
}

bool DvbFmt::addInputTerminal(tal_id_t id, const FmtDefinitionTable &modcod_def)
{
	// set less robust modcod at init
	fmt_id_t modcod = modcod_def.getMaxId();
	this->input_sts->addTerminal(id, modcod, modcod_def);
	return true;
}

bool DvbFmt::addOutputTerminal(tal_id_t id, const FmtDefinitionTable &modcod_def)
{
	// set less robust modcod at init
	fmt_id_t modcod = modcod_def.getMaxId();
	this->output_sts->addTerminal(id, modcod, modcod_def);
	return true;
}

bool DvbFmt::delTerminal(tal_id_t st_id, StFmtSimuList &sts)
{
	return sts.delTerminal(st_id);
}

bool DvbFmt::delInputTerminal(tal_id_t id)
{
	return this->delTerminal(id, *(this->input_sts));
}

bool DvbFmt::delOutputTerminal(tal_id_t id)
{
	return this->delTerminal(id, *(this->output_sts));
}

void DvbFmt::setInputSts(std::shared_ptr<StFmtSimuList> new_input_sts)
{
	this->input_sts = new_input_sts;
}

void DvbFmt::setOutputSts(std::shared_ptr<StFmtSimuList> new_output_sts)
{
	this->output_sts = new_output_sts;
}

void DvbFmt::setRequiredCniInput(tal_id_t tal_id,
								 double cni)
{
	this->input_sts->setRequiredCni(tal_id, cni);
}

void DvbFmt::setRequiredCniOutput(tal_id_t tal_id,
								  double cni)
{
	this->output_sts->setRequiredCni(tal_id, cni);
}

uint8_t DvbFmt::getCurrentModcodIdInput(tal_id_t id) const
{
	return this->input_sts->getCurrentModcodId(id);
}

uint8_t DvbFmt::getCurrentModcodIdOutput(tal_id_t id) const
{
	return this->output_sts->getCurrentModcodId(id);
}

double DvbFmt::getRequiredCniInput(tal_id_t tal_id)
{
	return this->input_sts->getRequiredCni(tal_id);
}

double DvbFmt::getRequiredCniOutput(tal_id_t tal_id)
{
	return this->output_sts->getRequiredCni(tal_id);
}

bool DvbFmt::getCniInputHasChanged(tal_id_t tal_id)
{
	return this->input_sts->getCniHasChanged(tal_id);
}

bool DvbFmt::getCniOutputHasChanged(tal_id_t tal_id)
{
	return this->output_sts->getCniHasChanged(tal_id);
}

Rt::Ptr<NetPacket> DvbFmt::setPacketExtension(EncapPlugin *pkt_hdl,
											  Rt::Ptr<NetPacket> packet,
											  tal_id_t source,
											  tal_id_t dest,
											  std::string extension_name,
											  time_sf_t super_frame_counter,
											  bool is_gw)
{
	uint32_t opaque = 0;
	double cni;
	if (is_gw)
	{
		cni = this->getRequiredCniInput(dest);

		LOG(this->log_fmt, LEVEL_INFO, "Add CNI extension with value %.2f dB for ST%u\n", cni, dest);
	}
	else
	{
		cni = this->getRequiredCniInput(source);

		LOG(this->log_fmt, LEVEL_INFO, "Add CNI extension with value %.2f dB\n", cni);
	}

	opaque = hcnton(cni);
	Rt::Ptr<NetPacket> extension_pkt = Rt::make_ptr<NetPacket>(nullptr);

	if (!pkt_hdl->setHeaderExtensions(std::move(packet),
									  extension_pkt,
									  source,
									  dest,
									  extension_name,
									  &opaque))
	{
		LOG(this->log_fmt, LEVEL_DEBUG,
			"SF#%d: cannot add header extension in packet",
			super_frame_counter);
	}
	else if (!extension_pkt)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
			"SF#%d: failed to create the GSE packet with "
			"extensions\n",
			super_frame_counter);
	}

	return extension_pkt;
}

// Implementation of functions with templates

template <class T>
bool DvbChannel::initBand(const OpenSandModelConf::spot &spot,
						  std::string section,
						  AccessType access_type,
						  time_us_t duration,
						  const FmtDefinitionTable &fmt_def,
						  TerminalCategories<T> &categories,
						  TerminalMapping<T> &terminal_affectation,
						  std::shared_ptr<T> &default_category,
						  fmt_groups_t &fmt_groups)
{
	// Get the value of the bandwidth
	freq_khz_t bandwidth_khz = spot.bandwidth_khz;
	LOG(this->log_init_channel, LEVEL_INFO,
		"%s: bandwitdh is %u kHz\n",
		section.c_str(), bandwidth_khz);

	// Get the value of the roll off
	double roll_off = spot.roll_off;

	unsigned int carrier_id = 0;
	group_id_t group_id = 0;
	for (auto &carrier : spot.carriers)
	{
		bool is_vcm = carrier.format_ratios.size() > 1;
		for (auto &format_ratios : carrier.format_ratios)
		{
			std::shared_ptr<FmtGroup> group = nullptr;
			std::string fmt_ids = format_ratios.first;
			if (carrier.access_type == access_type)
			{
				// we won't initialize FMT group here for other access
				group = std::make_shared<FmtGroup>(++group_id, fmt_ids, fmt_def);
				fmt_groups[group_id] = group;

				auto modcod_amount = group->getFmtIds().size();
				if ((is_vcm || access_type == AccessType::ALOHA) && modcod_amount > 1)
				{
					LOG(this->log_init_channel, LEVEL_ERROR,
						"Carrier cannot have more than one modcod for saloha or VCM\n");
					return false;
				}
			}

			std::string name = carrier.category;
			unsigned int ratio = format_ratios.second;
			rate_symps_t symbol_rate_symps = carrier.symbol_rate;

			// TODO: Improve this log, esp. for access type
			LOG(this->log_init_channel, LEVEL_NOTICE,
				"%s: new carriers: category=%s, Rs=%G, FMTs=%s, "
				"ratio=%d, access type=%d\n",
				section.c_str(), name.c_str(),
				symbol_rate_symps, fmt_ids.c_str(), ratio,
				carrier.access_type);

			// group may be NULL if this is not the good access type, this should be
			// only used in other_carriers in TerminalCategory that won't access
			// fmt_groups

			// create the category if it does not exist
			// we also create categories with wrong access type because:
			//  - we may have many access types in the category
			//  - we need to get all carriers for band computation
			std::shared_ptr<T> category;
			auto cat_iter = categories.find(name);
			if (cat_iter == categories.end())
			{
				category = std::make_shared<T>(name, access_type);
				categories.emplace(name, category);
			}
			else
			{
				category = cat_iter->second;
			}
			category->addCarriersGroup(carrier_id,
									   group, ratio,
									   symbol_rate_symps,
									   carrier.access_type);
		}
		++carrier_id;
	}

	// Compute bandplan
	if (!this->computeBandplan(bandwidth_khz, roll_off, duration, categories))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Cannot compute band plan for %s\n",
			section.c_str());
		return false;
	}

	auto cat_iter = categories.begin();
	// delete category with no carriers corresponding to the access type
	while (cat_iter != categories.end())
	{
		std::shared_ptr<T> category = cat_iter->second;
		// getCarriersNumber returns the number of carriers with the desired
		// access type only
		if (!category->getCarriersNumber())
		{
			LOG(this->log_init_channel, LEVEL_INFO,
				"Skip category %s with no carriers with desired access type\n",
				category->getLabel());
			categories.erase(cat_iter++);
		}
		else
		{
			++cat_iter;
		}
	}

	if (categories.size() == 0)
	{
		// no more category here, this will be handled by caller
		return true;
	}

	spot_id_t default_spot_id;
	std::string default_category_name;
	std::map<tal_id_t, std::pair<spot_id_t, std::string>> terminals;
	if (!OpenSandModelConf::Get()->getTerminalAffectation(default_spot_id,
														  default_category_name,
														  terminals))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Terminals categories initialisation failed\n");
		return false;
	}

	// Look for associated category
	default_category = nullptr;
	cat_iter = categories.find(default_category_name);
	if (cat_iter != categories.end())
	{
		default_category = cat_iter->second;
	}
	if (default_category == nullptr)
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
			"Section %s, could not find category %s, "
			"no default category for access type %u\n",
			section,
			default_category_name,
			access_type);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
			"ST default category: %s in %s\n",
			default_category->getLabel(),
			section);
	}

	for (auto &terminal : terminals)
	{
		tal_id_t tal_id = terminal.first;
		std::string name = terminal.second.second;
		cat_iter = categories.find(name);
		if (cat_iter == categories.end())
		{
			LOG(this->log_init_channel, LEVEL_NOTICE,
				"Could not find category %s for terminal %u affectation, "
				"it is maybe concerned by another access type",
				name.c_str(), tal_id);
			// keep the NULL affectation for this terminal to avoid
			// setting default category
			terminal_affectation.emplace(tal_id, nullptr);
		}
		else
		{
			terminal_affectation.emplace(tal_id, cat_iter->second);
			LOG(this->log_init_channel, LEVEL_INFO,
				"%s: terminal %u will be affected to category %s\n",
				section.c_str(), tal_id, name.c_str());
		}
	}

	return true;
}
template bool DvbChannel::initBand(
	const OpenSandModelConf::spot &spot,
	std::string section,
	AccessType access_type,
	time_us_t duration,
	const FmtDefinitionTable &fmt_def,
	TerminalCategories<TerminalCategoryDama> &categories,
	TerminalMapping<TerminalCategoryDama> &terminal_affectation,
	std::shared_ptr<TerminalCategoryDama> &default_category,
	fmt_groups_t &fmt_groups);
template bool DvbChannel::initBand(
	const OpenSandModelConf::spot &spot,
	std::string section,
	AccessType access_type,
	time_us_t duration,
	const FmtDefinitionTable &fmt_def,
	TerminalCategories<TerminalCategorySaloha> &categories,
	TerminalMapping<TerminalCategorySaloha> &terminal_affectation,
	std::shared_ptr<TerminalCategorySaloha> &default_category,
	fmt_groups_t &fmt_groups);

template <class T>
bool DvbChannel::computeBandplan(freq_khz_t available_bandplan_khz,
								 double roll_off,
								 time_us_t duration,
								 TerminalCategories<T> &categories)
{
	double weighted_sum_symps = 0.0;

	// compute weighted sum
	for (auto &&category_it : categories)
	{
		weighted_sum_symps += category_it.second->getWeightedSum();
	}

	LOG(this->log_init_channel, LEVEL_DEBUG,
		"Weigthed ratio sum: %f sym/s\n", weighted_sum_symps);

	if (weighted_sum_symps == 0.0)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Weighted ratio sum is 0\n");
		return false;
	}

	// compute carrier number per category
	for (auto &&category_it : categories)
	{
		unsigned int carriers_number = 0;
		std::shared_ptr<T> category = category_it.second;
		unsigned int ratio = category->getRatio();

		// convert bandwidth in Hz since weighted sum is in sym/s
		carriers_number = round(
			(ratio / weighted_sum_symps) *
			(1000 * available_bandplan_khz / (1 + roll_off)));
		// create at least one carrier
		if (carriers_number == 0)
		{
			LOG(this->log_init_channel, LEVEL_WARNING,
				"Band is too small for one carrier. "
				"Increase band for one carrier\n");
			carriers_number = 1;
		}
		LOG(this->log_init_channel, LEVEL_NOTICE,
			"Number of carriers for category %s: %d\n",
			category->getLabel().c_str(), carriers_number);

		// set the carrier numbers and capacity in carrier groups
		category->updateCarriersGroups(carriers_number, duration);
	}

	return true;
}
template bool DvbChannel::computeBandplan(
	freq_khz_t available_bandplan_khz,
	double roll_off,
	time_us_t duration,
	TerminalCategories<TerminalCategoryDama> &categories);
template bool DvbChannel::computeBandplan(
	freq_khz_t available_bandplan_khz,
	double roll_off,
	time_us_t duration,
	TerminalCategories<TerminalCategorySaloha> &categories);

template <class T>
bool DvbChannel::allocateBand(time_us_t duration,
							  std::string cat_label,
							  rate_kbps_t new_rate_kbps,
							  TerminalCategories<T> &categories)
{
	// Category SNO (the default one)
	std::string cat_sno_label("SNO");
	auto cat_sno_it = categories.find(cat_sno_label);
	if (cat_sno_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"%s category doesn't exist",
			cat_sno_label.c_str());
		return false;
	}
	std::shared_ptr<T> cat_sno = cat_sno_it->second;

	// The category we are interesting on
	auto cat_it = categories.find(cat_label);
	if (cat_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"This category %s doesn't exist yet\n",
			cat_label.c_str());
		return false; // TODO or create it ?
	}
	std::shared_ptr<T> cat = cat_it->second;

	// Fmt
	std::shared_ptr<const FmtGroup> cat_fmt_group = cat->getFmtGroup();

	unsigned int id;
	rate_symps_t new_rs;
	rate_symps_t old_rs;
	rate_symps_t rs_sno;
	rate_symps_t rs_needed;
	std::map<rate_symps_t, unsigned int> carriers;

	// Get the new total symbol rate
	id = cat_fmt_group->getMaxFmtId();
	new_rs = cat_fmt_group->getModcodDefinitions().kbitsToSym(id, new_rate_kbps);

	// Get the old total symbol rate
	old_rs = cat->getTotalSymbolRate();

	if (new_rs <= old_rs)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Request for an allocation while the rate (%.2E symps) is smaller "
			"than before (%.2E symps)\n",
			new_rs, old_rs);
		return false;
	}

	// Calculation of the symbol rate needed
	rs_needed = new_rs - old_rs;

	// Get the total symbol rate available
	rs_sno = cat_sno->getTotalSymbolRate();

	if (rs_sno < rs_needed)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Not enough rate available\n");
		return false;
	}

	if (!this->carriersTransferCalculation(cat_sno, rs_needed, carriers))
	{
		return false;
	}

	return this->carriersTransfer(duration, cat_sno, cat, carriers);
}
template bool DvbChannel::allocateBand(
	time_us_t duration,
	std::string cat_label,
	rate_kbps_t new_rate_kbps,
	TerminalCategories<TerminalCategoryDama> &categories);
template bool DvbChannel::allocateBand(
	time_us_t duration,
	std::string cat_label,
	rate_kbps_t new_rate_kbps,
	TerminalCategories<TerminalCategorySaloha> &categories);

template <class T>
bool DvbChannel::releaseBand(time_us_t duration,
							 std::string cat_label,
							 rate_kbps_t new_rate_kbps,
							 TerminalCategories<T> &categories)
{
	// Category SNO (the default one)
	std::string cat_sno_label("SNO");
	auto cat_sno_it = categories.find(cat_sno_label);
	if (cat_sno_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"%s category doesn't exist",
			cat_sno_label.c_str());
		return false;
	}
	std::shared_ptr<T> cat_sno = cat_sno_it->second;

	// The category we are interesting on
	auto cat_it = categories.find(cat_label);
	if (cat_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"This category %s doesn't exist\n",
			cat_label.c_str());
		return false;
	}
	std::shared_ptr<T> cat = cat_it->second;

	// Fmt
	std::shared_ptr<const FmtGroup> cat_fmt_group = cat->getFmtGroup();

	unsigned int id;
	rate_symps_t new_rs;
	rate_symps_t old_rs;
	rate_symps_t rs_unneeded;
	std::map<rate_symps_t, unsigned int> carriers;

	// Get the new total symbol rate
	id = cat_fmt_group->getMaxFmtId();
	new_rs = cat_fmt_group->getModcodDefinitions().kbitsToSym(id, new_rate_kbps);

	// Get the old total symbol rate
	old_rs = cat->getTotalSymbolRate();

	if (old_rs <= new_rs)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Request for an release while the rate is higher than before\n");
		return false;
	}

	// Calculation of the symbol rate needed
	rs_unneeded = old_rs - new_rs;

	if (!this->carriersTransferCalculation(cat, rs_unneeded, carriers))
		return false;

	if (rs_unneeded < 0)
	{
		carriers.begin()->second -= 1;
		rs_unneeded += carriers.begin()->first; // rs_unneeded should be positive
	}

	return this->carriersTransfer(duration, cat, cat_sno, carriers);
}
template bool DvbChannel::releaseBand(
	time_us_t duration,
	std::string cat_label,
	rate_kbps_t new_rate_kbps,
	TerminalCategories<TerminalCategoryDama> &categories);
template bool DvbChannel::releaseBand(
	time_us_t duration,
	std::string cat_label,
	rate_kbps_t new_rate_kbps,
	TerminalCategories<TerminalCategorySaloha> &categories);

template <class T>
bool DvbChannel::carriersTransferCalculation(std::shared_ptr<T> cat, rate_symps_t &rate_symps,
											 std::map<rate_symps_t, unsigned int> &carriers)
{
	unsigned int num_carriers;

	// List of the carriers available (Rs, number)
	std::map<rate_symps_t, unsigned int> carriers_available;
	std::map<rate_symps_t, unsigned int>::reverse_iterator carriers_ite1;
	std::map<rate_symps_t, unsigned int>::reverse_iterator carriers_ite2;

	// Get the classification of the available
	// carriers in function of their symbol rate
	carriers_available = cat->getSymbolRateList();

	// Calculation of the needed carriers
	carriers_ite1 = carriers_available.rbegin();
	while (rate_symps > 0)
	{
		if (carriers_ite1 == carriers_available.rend())
		{
			carriers[carriers_ite2->first] += 1;
			/*
			if(carriers.find(carriers_ite2->first) == carriers.end())
			{
				carriers.emplace(carriers_ite2->first, 1U);
			}
			else
			{
				carriers.find(carriers_ite2->first)->second += 1;
			}
			*/
			rate_symps -= carriers_ite2->first; // rate should be negative now
			carriers_available.find(carriers_ite2->first)->second -= 1;
			// Erase the smaller carriers (because they are wasted)
			carriers_ite2++;
			while (carriers_ite2 != carriers_available.rend())
			{
				if (carriers.find(carriers_ite2->first) != carriers.end())
				{
					// rate should still be negative after that
					rate_symps += (carriers_ite2->first * carriers_ite2->second);
					carriers_available.find(carriers_ite2->first)->second += carriers_ite2->second;
				}
				carriers.erase(carriers_ite2->first);
				carriers_ite2++;
			}
			continue;
		}
		if (rate_symps < carriers_ite1->first)
		{
			// in case the next carriers aren't enought
			carriers_ite2 = carriers_ite1;
			carriers_ite1++;
			continue;
		}
		num_carriers = floor(rate_symps / carriers_ite1->first);
		if (num_carriers > carriers_ite1->second)
		{
			num_carriers = carriers_ite1->second;
		}
		carriers_available.find(carriers_ite1->first)->second -= num_carriers;
		carriers.emplace(carriers_ite1->first, num_carriers);
		rate_symps -= (carriers_ite1->first * num_carriers);
		if (num_carriers != carriers_ite1->second)
		{
			carriers_ite2 = carriers_ite1;
		}
		carriers_ite1++;
	}

	return true;
}
template bool DvbChannel::carriersTransferCalculation(
	std::shared_ptr<TerminalCategoryDama> cat,
	rate_symps_t &rate_symps,
	std::map<rate_symps_t, unsigned int> &carriers);
template bool DvbChannel::carriersTransferCalculation(
	std::shared_ptr<TerminalCategorySaloha> cat,
	rate_symps_t &rate_symps,
	std::map<rate_symps_t, unsigned int> &carriers);

template <class T>
bool DvbChannel::carriersTransfer(time_us_t duration,
								  std::shared_ptr<T> cat1, std::shared_ptr<T> cat2,
								  std::map<rate_symps_t, unsigned int> carriers)
{
	unsigned int highest_id;
	unsigned int associated_ratio;

	// Allocation and deallocation of carriers
	highest_id = cat2->getHighestCarrierId();
	for (auto &&[carriers_rate, carriers_count] : carriers)
	{
		if (carriers_count == 0)
		{
			LOG(this->log_init_channel, LEVEL_INFO,
				"Empty carriers group\n");
			continue;
		}

		// Deallocation of SNO carriers
		if (!cat1->deallocateCarriers(carriers_rate, carriers_count, associated_ratio))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
				"Wrong calculation of the needed carriers");
			return false;
		}

		// Allocation of cat carriers
		highest_id++;
		cat2->addCarriersGroup(highest_id, cat2->getFmtGroup(),
							   carriers_count, associated_ratio,
							   carriers_rate, cat2->getDesiredAccess(),
							   duration);
	}

	return true;
}
template bool DvbChannel::carriersTransfer(
	time_us_t duration,
	std::shared_ptr<TerminalCategoryDama> cat1,
	std::shared_ptr<TerminalCategoryDama> cat2,
	std::map<rate_symps_t, unsigned int> carriers);
template bool DvbChannel::carriersTransfer(
	time_us_t duration,
	std::shared_ptr<TerminalCategorySaloha> cat1,
	std::shared_ptr<TerminalCategorySaloha> cat2,
	std::map<rate_symps_t, unsigned int> carriers);

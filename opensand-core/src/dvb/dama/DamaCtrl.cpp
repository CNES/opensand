/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file    DamaCtrl.cpp
 * @brief   This class defines the DAMA controller interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 * @author  Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include <math.h>
#include <algorithm>

#include <opensand_output/Output.h>

#include "DamaCtrl.h"
#include "OpenSandModelConf.h"


DamaCtrl::DamaCtrl(spot_id_t spot):
	is_parent_init(false),
	terminals(), // TODO not very useful, they are stored in categories
	current_superframe_sf(0),
	frame_duration(0),
	rbdc_timeout_sf(0),
	fca_kbps(0),
	enable_rbdc(false),
	enable_vbdc(false),
	available_bandplan_khz(0),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	input_sts(NULL),
	input_modcod_def(NULL),
	simulated(false),
	spot_id(spot)
{
	// Output Log
	auto output = Output::Get();
	this->log_init = output->registerLog(LEVEL_WARNING, "Dvb.init");
	this->log_logon = output->registerLog(LEVEL_WARNING, "Dvb.DamaCtrl.Logon");
	this->log_super_frame_tick = output->registerLog(LEVEL_WARNING, "Dvb.DamaCtrl.SuperFrameTick");
	this->log_run_dama = output->registerLog(LEVEL_WARNING, "Dvb.DamaCtrl.RunDama");
	this->log_sac = output->registerLog(LEVEL_WARNING, "Dvb.SAC");
	this->log_ttp = output->registerLog(LEVEL_WARNING, "Dvb.TTP");
	this->log_pep = output->registerLog(LEVEL_WARNING, "Dvb.Ncc.PEP");
	this->log_fmt = output->registerLog(LEVEL_WARNING, "Dvb.Fmt.Update");

	// generate probes prefix
	bool is_sat = OpenSandModelConf::Get()->getComponentType() == Component::satellite;
	this->output_prefix = generateProbePrefix(spot_id, Component::gateway, is_sat);
}

DamaCtrl::~DamaCtrl()
{
	this->terminals.clear();
	this->categories.clear();
	this->terminal_affectation.clear();
}

bool DamaCtrl::initParent(time_us_t frame_duration,
                          time_sf_t rbdc_timeout_sf,
                          rate_kbps_t fca_kbps,
                          TerminalCategories<TerminalCategoryDama> categories,
                          TerminalMapping<TerminalCategoryDama> terminal_affectation,
                          std::shared_ptr<TerminalCategoryDama> default_category,
                          std::shared_ptr<const StFmtSimuList> input_sts,
                          FmtDefinitionTable *const input_modcod_def,
                          bool simulated)
{
	this->frame_duration = frame_duration;
	this->rbdc_timeout_sf = rbdc_timeout_sf;
	this->fca_kbps = fca_kbps;
	this->input_sts = input_sts;
	this->input_modcod_def = input_modcod_def;
	this->simulated = simulated;

	this->categories = categories;

	// we keep terminal affectation and default category but these affectations
	// and the default category can concern non DAMA categories
	// so be careful when adding a new terminal
	this->terminal_affectation = terminal_affectation;

	this->default_category = default_category;
	if(!this->default_category)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "No default terminal affectation defined, "
		    "some terminals may not be able to log\n");
	}

	this->is_parent_init = true;

	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "the output probes and stats initialization have "
		    "failed\n");
		goto error;
	}

	return true;
error:
	return false;
}

bool DamaCtrl::initOutput()
{
	auto output = Output::Get();

	// RBDC request number
	this->probe_gw_rbdc_req_num =
	    output->registerProbe<int>(output_prefix + "RBDC.RBDC request number", "", true, SAMPLE_LAST);
	this->gw_rbdc_req_num = 0;

	// RBDC requested capacity
	this->probe_gw_rbdc_req_size =
	    output->registerProbe<int>(output_prefix + "RBDC.RBDC requested capacity", "Kbits/s", true, SAMPLE_LAST);

	// VBDC request number
	this->probe_gw_vbdc_req_num =
	    output->registerProbe<int>(output_prefix + "VBDC.VBDC request number", "", true, SAMPLE_LAST);
	this->gw_vbdc_req_num = 0;

	// VBDC Requested capacity
	this->probe_gw_vbdc_req_size =
	    output->registerProbe<int>(output_prefix + "VBDC.VBDC requested capacity", "Kbits", true, SAMPLE_LAST);

	// Allocated ressources
	// CRA allocation
	this->probe_gw_cra_alloc =
	    output->registerProbe<int>(output_prefix + "Global.CRA allocated", "Kbits/s", true, SAMPLE_LAST);
	this->gw_cra_alloc_kbps = 0;

	// RBDC max
	this->probe_gw_rbdc_max =
	    output->registerProbe<int>(output_prefix + "RBDC.RBDC max", "Kbits/s", true, SAMPLE_LAST);
	this->gw_rbdc_max_kbps = 0;

	// RBDC allocation
	this->probe_gw_rbdc_alloc =
	    output->registerProbe<int>(output_prefix + "RBDC.RBDC allocated", "Kbits/s", true, SAMPLE_LAST);

	// VBDC allocation
	this->probe_gw_vbdc_alloc =
	    output->registerProbe<int>(output_prefix + "VBDC.VBDC allocated", "Kbits", true, SAMPLE_LAST);

	// FCA allocation
	if(this->fca_kbps != 0)
	{
		// only create FCA probe if it is enabled
		this->probe_gw_fca_alloc =
		    output->registerProbe<int>(output_prefix + "Global.FCA allocated", "Kbits/s", true, SAMPLE_LAST);
	}

	// Total and remaining capacity
	this->probe_gw_return_total_capacity = this->generateGwCapacityProbe("Available");
	this->probe_gw_return_remaining_capacity = this->generateGwCapacityProbe("Remaining");
	this->gw_remaining_capacity = 0;

	// Logged ST number
	this->probe_gw_st_num =
	    output->registerProbe<int>(output_prefix + "Global.ST number", "", true, SAMPLE_LAST);
	this->gw_st_num = 0;

	// Register output probes for simulated STs
	if(this->simulated)
	{
		// tal_id 0 is for GW so it is unused
		tal_id_t tal_id = 0;
		auto probe_cra = output->registerProbe<int>(output_prefix + "Simulated_ST.CRA allocation",
		                                            "Kbits/s", true, SAMPLE_MAX);
		this->probes_st_cra_alloc.emplace(tal_id, probe_cra);

		auto probe_rbdc_max = output->registerProbe<int>(output_prefix + "Simulated_ST.RBDC max",
		                                                 "Kbits/s", true, SAMPLE_MAX);
		this->probes_st_rbdc_max.emplace(tal_id, probe_rbdc_max);

		auto probe_rbdc = output->registerProbe<int>(output_prefix + "Simulated_ST.RBDC allocation",
		                                             "Kbits/s", true, SAMPLE_MAX);
		this->probes_st_rbdc_alloc.emplace(tal_id, probe_rbdc);

		auto probe_vbdc = output->registerProbe<int>(output_prefix + "Simulated_ST.VBDC allocation",
		                                             "Kbits", true, SAMPLE_SUM);
		this->probes_st_vbdc_alloc.emplace(tal_id, probe_vbdc);

		// only create FCA probe if it is enabled
		if(this->fca_kbps != 0)
		{
			auto probe_fca = output->registerProbe<int>(output_prefix + "Simulated_ST.FCA allocation",
			                                            "Kbits/s", true, SAMPLE_MAX);
			this->probes_st_fca_alloc.emplace(tal_id, probe_fca);
		}
	}

	return true;
}

bool DamaCtrl::hereIsLogon(Rt::Ptr<LogonRequest> logon)
{
	tal_id_t tal_id = logon->getMac();
	rate_kbps_t cra_kbps = logon->getRtBandwidth();
	rate_kbps_t max_rbdc_kbps = logon->getMaxRbdc();
	vol_kb_t max_vbdc_kb = logon->getMaxVbdc();
	LOG(this->log_logon, LEVEL_INFO,
	    "New ST: #%u, with CRA: %u bits/sec\n", tal_id, cra_kbps);

	DamaTerminalList::iterator it;
	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		TerminalMapping<TerminalCategoryDama>::const_iterator it;
		TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
		std::shared_ptr<TerminalCategoryDama> category;
		std::vector<CarriersGroupDama *> carriers;
		std::vector<CarriersGroupDama *>::const_iterator carrier_it;
		uint32_t max_capa_kbps = 0;

		// Find the associated category
		it = this->terminal_affectation.find(tal_id);
		if(it == this->terminal_affectation.end())
		{
			if(!this->default_category)
			{
				LOG(this->log_logon, LEVEL_WARNING,
				    "ST #%u cannot be logged, there is no default category\n",
				    tal_id);
				return false;
			}
			LOG(this->log_logon, LEVEL_INFO,
			    "ST #%d is not affected to a category, using "
			    "default: %s\n", tal_id,
			    this->default_category->getLabel().c_str());
			category = this->default_category;
		}
		else
		{
			category = (*it).second;
			if(!category)
			{
				LOG(this->log_logon, LEVEL_INFO,
				    "Terminal %d does not use DAMA\n",
				    tal_id);
				return true;
			}
		}

		// check if the category is concerned by DAMA
		if(this->categories.find(category->getLabel()) == this->categories.end())
		{
			LOG(this->log_logon, LEVEL_INFO,
			    "Terminal %u is affected to non DAMA category\n", tal_id);
			return true;
		}

		// create the terminal
		std::shared_ptr<TerminalContextDama> terminal;
		if(!this->createTerminal(terminal,
		                         tal_id,
		                         cra_kbps,
		                         max_rbdc_kbps,
		                         this->rbdc_timeout_sf,
		                         max_vbdc_kb))
		{
			LOG(this->log_logon, LEVEL_ERROR,
			    "Cannot create terminal context for ST #%d\n",
			    tal_id);
			return false;
		}

		if(tal_id < BROADCAST_TAL_ID)
		{
			auto output = Output::Get();

			auto prefix = output_prefix + "st" + std::to_string(tal_id) + "_allocation.";

			// Output probes and stats
			auto probe_cra = output->registerProbe<int>(prefix + "CRA allocation",
			                                            "Kbits/s", true, SAMPLE_MAX);
			this->probes_st_cra_alloc.emplace(tal_id, probe_cra);

			auto probe_rbdc_max = output->registerProbe<int>(prefix + "RBDC max",
			                                                 "Kbits/s", true, SAMPLE_MAX);
			this->probes_st_rbdc_max.emplace(tal_id, probe_rbdc_max);

			auto probe_rbdc = output->registerProbe<int>(prefix + "RBDC allocation",
			                                             "Kbits/s", true, SAMPLE_MAX);
			this->probes_st_rbdc_alloc.emplace(tal_id, probe_rbdc);

			auto probe_vbdc = output->registerProbe<int>(prefix + "VBDC allocation",
			                                             "Kbits", true, SAMPLE_SUM);
			this->probes_st_vbdc_alloc.emplace(tal_id, probe_vbdc);

			// only create FCA probe if it is enabled
			if(this->fca_kbps != 0)
			{
				auto probe_fca = output->registerProbe<int>(prefix + "FCA allocation",
				                                            "Kbits/s", true, SAMPLE_MAX);
				this->probes_st_fca_alloc.emplace(tal_id, probe_fca);
			}
		}

		// Add the new terminal to the list
		this->terminals.insert({tal_id, terminal});

		// add terminal in category and inform terminal of its category
		category->addTerminal(terminal);
		terminal->setCurrentCategory(category->getLabel());
		LOG(this->log_logon, LEVEL_NOTICE,
		    "Add terminal %u in category %s\n",
		    tal_id, category->getLabel().c_str());
		if(tal_id > BROADCAST_TAL_ID)
		{
			DC_RECORD_EVENT("LOGON st%d rt=%u rbdc=%u vbdc=%u", logon->getMac(),
			                logon->getRtBandwidth(), max_rbdc_kbps, max_vbdc_kb);
		}
		
		// Output probes and stats
		this->gw_st_num += 1;
		this->gw_rbdc_max_kbps += max_rbdc_kbps;
		this->probe_gw_rbdc_max->put(gw_rbdc_max_kbps);

		// check that CRA is not too high, else print a warning !
		for (auto &&carriers: category->getCarriersGroups())
		{
			// we can use the same function that convert sym to kbits
			// for conversion from sym/s to kbits/s
			max_capa_kbps +=
					// the last FMT ID in getFmtIds() is the one
					// which will give us the higher rate
					this->input_modcod_def->symToKbits(carriers.getFmtIds().back(),
					                                   carriers.getSymbolRate() *
					                                   carriers.getCarriersNumber());
		}

		if(cra_kbps > max_capa_kbps)
		{
			LOG(this->log_logon, LEVEL_WARNING,
			    "The CRA value for ST%u is too high compared to "
			    "the maximum carrier capacity (%u > %u)\n",
			    tal_id, cra_kbps, max_capa_kbps);
		}

	}
	else
	{
		LOG(this->log_logon, LEVEL_NOTICE,
		    "Duplicate logon received for ST #%u\n", tal_id);
	}


	return true;
}

bool DamaCtrl::hereIsLogoff(Rt::Ptr<Logoff> logoff)
{
	tal_id_t tal_id = logoff->getMac();

	auto it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		LOG(this->log_logon, LEVEL_INFO,
		    "No ST found for id %u\n", tal_id);
		return false;
	}

	auto terminal = it->second;

	// Output probes and stats
	this->gw_st_num -= 1;
	this->gw_rbdc_max_kbps -= terminal->getMaxRbdc();
	this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);

	// remove terminal from the list
	this->terminals.erase(terminal->getTerminalId());

	// remove terminal from the terminal category
	auto category_it = this->categories.find(terminal->getCurrentCategory());
	if(category_it != this->categories.end())
	{
		auto category = category_it->second;
		if(!category->removeTerminal(terminal))
		{
			return false;
		}
	}

	if(tal_id > BROADCAST_TAL_ID)
	{
		DC_RECORD_EVENT("LOGOFF st%d", tal_id);
	}

	return true;
}

bool DamaCtrl::runOnSuperFrameChange(time_sf_t superframe_number_sf)
{
	this->current_superframe_sf = superframe_number_sf;

	// reset capacity of carriers
	if(!this->resetCarriersCapacity())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot reset carriers capacity\n",
		    this->current_superframe_sf);
		return false;
	}

	// update wave forms
	if(!this->updateWaveForms())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot update wave forms\n",
		    this->current_superframe_sf);
		return false;
	}

	//TODO: update RBDC credit here, not in reset terminals allocation
	if(!this->computeTerminalsAllocations())
	{
		LOG(this->log_super_frame_tick, LEVEL_ERROR,
		    "SF#%u: Cannot compute terminals allocations\n",
		    this->current_superframe_sf);
		return false;
	}

	return 0;
}


bool DamaCtrl::computeTerminalsAllocations()
{
	DamaTerminalList::iterator tal_it;

	// reset the terminals allocations
	if(!this->resetTerminalsAllocations())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot reset terminals allocations\n",
		    this->current_superframe_sf);
		return false;
	}

	if(!this->computeTerminalsCraAllocation())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot compute terminals CRA allocation\n",
		    this->current_superframe_sf);
		return false;
	}

	if(!this->enable_rbdc)
	{
		// Output stats and probes
		for(tal_it = this->terminals.begin(); tal_it != this->terminals.end(); ++tal_it)
		{
			tal_id_t tal_id = tal_it->first;
			if(tal_id < BROADCAST_TAL_ID)
			{
				this->probes_st_rbdc_alloc[tal_id]->put(0);
			}
		}
		if(this->simulated)
		{
			this->probes_st_rbdc_alloc[0]->put(0);
		}
		this->probe_gw_rbdc_req_num->put(0);
		this->probe_gw_rbdc_req_size->put(0);
		this->probe_gw_rbdc_alloc->put(0);
	}
	else if(!this->computeTerminalsRbdcAllocation())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot compute terminals RBDC allocation\n",
		    this->current_superframe_sf);
		return false;
	}

	if(!this->enable_vbdc)
	{
		// Output stats and probes
		for(tal_it = this->terminals.begin(); tal_it != this->terminals.end(); ++tal_it)
		{
			tal_id_t tal_id = tal_it->first;
			if(tal_id < BROADCAST_TAL_ID)
			{
				this->probes_st_vbdc_alloc[tal_id]->put(0);
			}
		}
		if(this->simulated)
		{
			this->probes_st_vbdc_alloc[0]->put(0);
		}


		this->probe_gw_vbdc_req_num->put(0);
		this->probe_gw_vbdc_req_size->put(0);
		this->probe_gw_vbdc_alloc->put(0);
	}
	else if(!this->computeTerminalsVbdcAllocation())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot compute terminals VBDC allocation\n",
		    this->current_superframe_sf);
		return false;
	}

	if(!this->computeTerminalsFcaAllocation())
	{
		LOG(this->log_run_dama, LEVEL_ERROR,
		    "SF#%u: Cannot compute terminals FCA allocation\n",
		    this->current_superframe_sf);
		return false;
	}

	return true;
}

void DamaCtrl::setRecordFile(FILE *event_stream)
{
	this->event_file = event_stream;
	DC_RECORD_EVENT("%s", "# --------------------------------------\n");
}

// TODO disable timers on probes if output is disabled
// and event to reactivate them ?!
void DamaCtrl::updateStatistics(time_ms_t UNUSED(period_ms))
{
	int simu_cra = 0;
	int simu_rbdc = 0;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;

	// Update probes and stats
	this->probe_gw_st_num->put(this->gw_st_num);
	this->probe_gw_cra_alloc->put(this->gw_cra_alloc_kbps);
	this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);
	for(DamaTerminalList::iterator it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		tal_id_t tal_id = it->first;
		std::shared_ptr<TerminalContextDama> terminal = it->second;
		if(tal_id > BROADCAST_TAL_ID)
		{
			simu_cra += terminal->getRequiredCra();
			simu_rbdc += terminal->getMaxRbdc();
		}
		else
		{
			this->probes_st_cra_alloc[tal_id]->put(
				terminal->getRequiredCra());
			this->probes_st_rbdc_max[tal_id]->put(
				terminal->getMaxRbdc());
		}
	}
	if(this->simulated)
	{
		this->probes_st_cra_alloc[0]->put(simu_cra);
		this->probes_st_rbdc_max[0]->put(simu_rbdc);
	}
	this->probe_gw_return_remaining_capacity->put(this->gw_remaining_capacity);
	for(cat_it = this->categories.begin();
	    cat_it != categories.end(); ++cat_it)
	{
		std::shared_ptr<TerminalCategoryDama> category = cat_it->second;
		std::string label = category->getLabel();
		this->probes_category_return_remaining_capacity[label]->put(
			this->category_return_remaining_capacity[label]);
		for (auto &&carriers: category->getCarriersGroups())
		{
			unsigned int carrier_id = carriers.getCarriersId();

			// Create the probes if they don't exist yet
			// (necessary in case of carrier modifications with SVNO interface)
			auto &probes_remain_capa = this->probes_carrier_return_remaining_capacity[label];
			if (probes_remain_capa.find(carrier_id) == probes_remain_capa.end())
			{
				auto probe = this->generateCarrierCapacityProbe(label, carrier_id, "Remaining");
				probes_remain_capa.emplace(carrier_id, probe);
			}
			auto &remain_capa = this->carrier_return_remaining_capacity[label];
			if(remain_capa.find(carrier_id) == remain_capa.end())
			{
				remain_capa.insert({carrier_id, 0});
			}
			probes_remain_capa[carrier_id]->put(remain_capa[carrier_id]);
		}
	}
}

TerminalCategories<TerminalCategoryDama> *DamaCtrl::getCategories()
{
	return &(this->categories);
}

std::shared_ptr<TerminalContextDama> DamaCtrl::getTerminalContext(tal_id_t tal_id) const
{
	auto it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		return nullptr;
	}

	return it->second;
}

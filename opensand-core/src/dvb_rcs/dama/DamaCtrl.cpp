/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 */

#define DBG_PACKAGE PKG_DAMA_DC
#include <opensand_conf/uti_debug.h>

#include "DamaCtrl.h"

#include <assert.h>
#include <math.h>

#include <algorithm>

using std::pair;

DamaCtrl::DamaCtrl():
	is_parent_init(false),
	converter(NULL),
	terminals(), // TODO not very useful, they are stocked in categories
	with_phy_layer(false),
	current_superframe_sf(0),
	frame_duration_ms(0),
	frames_per_superframe(0),
	cra_decrease(false),
	rbdc_timeout_sf(0),
	fca_kbps(0),
	enable_rbdc(false),
	enable_vbdc(false),
	available_bandplan_khz(0),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	// TODO ouch, up and down at the same time, be careful !
	ret_fmt_simu(),
	fwd_fmt_simu(),
	roll_off(0.0)
{
	this->probe_gw_rbdc_req_num = NULL;
	this->probe_gw_rbdc_req_size = NULL;
	this->probe_gw_vbdc_req_num = NULL;
	this->probe_gw_vbdc_req_size = NULL;
	this->probe_gw_cra_alloc = NULL;
	this->probe_gw_rbdc_alloc = NULL;
	this->probe_gw_rbdc_max = NULL;
	this->probe_gw_vbdc_alloc = NULL;
	this->probe_gw_fca_alloc = NULL;
	this->probe_gw_return_total_capacity = NULL;
	this->probe_gw_return_remaining_capacity = NULL;
	this->probe_gw_st_num = NULL;
}

DamaCtrl::~DamaCtrl()
{
	if(this->converter)
	{
		delete this->converter;
	}

	for(DamaTerminalList::iterator it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		delete it->second;
	}
	this->terminals.clear();

	for(TerminalCategories::iterator it = this->categories.begin();
	    it != this->categories.end(); ++it)
	{
		delete (*it).second;
	}
	this->categories.clear();

	this->terminal_affectation.clear();
}

bool DamaCtrl::initParent(time_ms_t frame_duration_ms,
                          unsigned int frames_per_superframe,
                          bool with_phy_layer,
                          vol_bytes_t packet_length_bytes,
                          bool cra_decrease,
                          time_sf_t rbdc_timeout_sf,
                          rate_kbps_t fca_kbps,
                          TerminalCategories categories,
                          TerminalMapping terminal_affectation,
                          TerminalCategory *default_category,
                          FmtSimulation *const ret_fmt_simu,
                          FmtSimulation *const fwd_fmt_simu)
{
	this->frame_duration_ms = frame_duration_ms;
	this->frames_per_superframe = frames_per_superframe;
	this->with_phy_layer = with_phy_layer;
	this->cra_decrease = cra_decrease;
	this->rbdc_timeout_sf = rbdc_timeout_sf;
	this->fca_kbps = fca_kbps;
	this->ret_fmt_simu = ret_fmt_simu;
	this->fwd_fmt_simu = fwd_fmt_simu;

	this->converter = new UnitConverter(packet_length_bytes,
	                                    this->frame_duration_ms);
	if(converter == NULL)
	{
		UTI_ERROR("Cannot create the Unit Converter\n");
		goto error;
	}

	if(categories.size() == 0)
	{
		UTI_ERROR("No category defined\n");
		return false;
	}
	this->categories = categories;

	this->terminal_affectation = terminal_affectation;

	if(default_category == NULL)
	{
		UTI_ERROR("No default terminal affectation defined\n");
		return false;
	}
	this->default_category = default_category;

	this->is_parent_init = true;

	if (!this->initOutput())
	{
		UTI_ERROR("the output probes and stats initialization have failed\n");
		return false;
	}

	return true;
error:
	return false;
}

bool DamaCtrl::initOutput()
{
	// RBDC request number
	this->probe_gw_rbdc_req_num = Output::registerProbe<int>(
		"NCC.RBDC.RBDC request number", "", true, SAMPLE_LAST);
	this->gw_rbdc_req_num = 0;

	// RBDC requested capacity
	this->probe_gw_rbdc_req_size = Output::registerProbe<int>(
		"NCC.RBDC.RBDC requested capacity", "Kbits/s", true, SAMPLE_LAST);
	this->gw_rbdc_req_size_pktpf = 0;

	// VBDC request number
	this->probe_gw_vbdc_req_num = Output::registerProbe<int>(
		"NCC.VBDC.VBDC request number", "", true, SAMPLE_LAST);
	this->gw_vbdc_req_num = 0;

	// VBDC Requested capacity
	this->probe_gw_vbdc_req_size = Output::registerProbe<int>(
		"NCC.VBDC.VBDC requested capacity", "Kbits", true, SAMPLE_LAST);
	this->gw_vbdc_req_size_pkt = 0;

	// Allocated ressources
		// CRA allocation
	this->probe_gw_cra_alloc = Output::registerProbe<int>(
		"NCC.CRA allocated", "Kbits/s", true, SAMPLE_LAST);
	this->gw_cra_alloc_kbps = 0;

		// RBDC max
	this->probe_gw_rbdc_max = Output::registerProbe<int>(
		"NCC.RBDC.RBDC max", "Kbits/s", true, SAMPLE_LAST);
	this->gw_rbdc_max_kbps = 0;

		// RBDC allocation
	this->probe_gw_rbdc_alloc = Output::registerProbe<int>(
		"NCC.RBDC.RBDC allocated", "Kbits/s", true, SAMPLE_LAST);
	this->gw_rbdc_alloc_pktpf = 0;

		// VBDC allocation
	this->probe_gw_vbdc_alloc = Output::registerProbe<int>(
		"NCC.VBDC.VBDC allocated", "Kbits", true, SAMPLE_LAST);
	this->gw_vbdc_alloc_pkt = 0;

		// FRA allocation
	this->probe_gw_fca_alloc = Output::registerProbe<int>(
		"NCC.FCA allocated", "Kbits/s", true, SAMPLE_LAST);
	this->gw_fca_alloc_pktpf = 0;

		// Total and remaining capacity
	this->probe_gw_return_total_capacity = Output::registerProbe<int>(
		"Up/Return capacity.Total.Available", "Kbits/s", true, SAMPLE_LAST);
	this->gw_return_total_capacity_pktpf = 0;
	this->probe_gw_return_remaining_capacity = Output::registerProbe<int>(
		"Up/Return capacity.Total.Remaining", "Kbits/s", true, SAMPLE_LAST);
	this->gw_remaining_capacity_pktpf = 0;

		// Logged ST number
	this->probe_gw_st_num = Output::registerProbe<int>(
		"NCC.ST number", "", true, SAMPLE_LAST);
	this->gw_st_num = 0;

	return true;
}

bool DamaCtrl::hereIsLogon(const LogonRequest *logon)
{
	tal_id_t tal_id = logon->getMac();
	rate_kbps_t cra_kbps = logon->getRtBandwidth();
	rate_kbps_t max_rbdc_kbps = logon->getMaxRbdc();
	vol_kb_t max_vbdc_kb = logon->getMaxVbdc();
	UTI_DEBUG("New ST: #%u, with CRA: %u bits/sec\n", tal_id, cra_kbps);

	DamaTerminalList::iterator it;
	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		TerminalContext *terminal;
		TerminalMapping::const_iterator it;
		TerminalCategories::const_iterator category_it;
		TerminalCategory *category;
		vector<CarriersGroup *> carriers;
		vector<CarriersGroup *>::const_iterator carrier_it;
		const FmtDefinitionTable *modcod_def;
		uint32_t max_capa_kbps = 0;

		Probe<int> *probe_cra;
		Probe<int> *probe_rbdc_max;
		Probe<int> *probe_rbdc;
		Probe<int> *probe_vbdc;
		Probe<int> *probe_fca;

		// create the terminal
		if(!this->createTerminal(&terminal,
		                         tal_id,
		                         cra_kbps,
		                         max_rbdc_kbps,
		                         this->rbdc_timeout_sf,
		                         max_vbdc_kb))
		{
			UTI_ERROR("Cannot create terminal context for ST #%d\n",
			          tal_id);
			return false;
		}

		// Output probes and stats
		probe_cra = Output::registerProbe<int>("Kbits/s", true, SAMPLE_LAST,
		                                       "ST%u_allocation.CRA allocation",
		                                       tal_id);
		this->probes_st_cra_alloc.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_cra));
		probe_rbdc_max = Output::registerProbe<int>("Kbits/s", true, SAMPLE_LAST,
		                                            "ST%u_allocation.RBDC max",
		                                            tal_id);
		this->probes_st_rbdc_max.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_rbdc_max));
		probe_rbdc = Output::registerProbe<int>("Kbits/s", true, SAMPLE_LAST,
		                                        "ST%u_allocation.RBDC allocation",
		                                        tal_id);
		this->probes_st_rbdc_alloc.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_rbdc));
		probe_vbdc = Output::registerProbe<int>("Kbits", true, SAMPLE_LAST,
		                                        "ST%u_allocation.VBDC allocation",
		                                        tal_id);
		this->probes_st_vbdc_alloc.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_vbdc));
		probe_fca = Output::registerProbe<int>("Kbits/s", true, SAMPLE_LAST,
		                                       "ST%u_allocation.FCA allocation",
		                                       tal_id);
		this->probes_st_fca_alloc.insert(
			pair<tal_id_t, Probe<int> *>(tal_id, probe_fca));

		// Add the new terminal to the list
		this->terminals.insert(
			pair<unsigned int, TerminalContext *>(tal_id, terminal));

		// Find the associated category
		it = this->terminal_affectation.find(tal_id);
		if(it == this->terminal_affectation.end())
		{
			UTI_DEBUG("ST #%d is not affected to a category, using default: %s\n",
					  tal_id, this->default_category->getLabel().c_str());
			category = this->default_category;
		}
		else
		{
			category = it->second;
		}
		// add terminal in category and inform terminal of its category
		category->addTerminal(terminal);
		terminal->setCurrentCategory(category->getLabel());
		UTI_INFO("Add terminal %u in category %s\n",
		         tal_id, category->getLabel().c_str());
		DC_RECORD_EVENT("LOGON st%d rt = %u", logon->getMac(),
		                logon->getRtBandwidth());

		// Output probes and stats
		this->gw_st_num += 1;
		this->gw_cra_alloc_kbps += cra_kbps;
		this->probe_gw_cra_alloc->put(gw_cra_alloc_kbps);
		this->gw_rbdc_max_kbps += max_rbdc_kbps;
		this->probe_gw_rbdc_max->put(gw_rbdc_max_kbps);

		// check that CRA is not too high, else print a warning !
		carriers = category->getCarriersGroups();
		modcod_def = this->ret_fmt_simu->getModcodDefinitions();
		for(carrier_it = carriers.begin();
		    carrier_it != carriers.end();
		    ++carrier_it)
		{
			// we can use the same function that convert sym to kbits
			// for conversion from sym/s to kbits/s
			max_capa_kbps +=
					// maxium FMT ID is the last in getFmtIds() and this is the one
					// which will give us the higher rate
					modcod_def->symToKbits((*carrier_it)->getFmtIds().back(),
					                       (*carrier_it)->getSymbolRate() *
					                       (*carrier_it)->getCarriersNumber());

		}

		if(cra_kbps > max_capa_kbps)
		{
			// TODO WARNING
			UTI_INFO("The CRA value for ST%u is too high compared to the "
			         "maximum carrier capacity (%u > %u)\n",
			         tal_id, cra_kbps, max_capa_kbps);
			// TODO OUTPUT::EVENT
	}

	}
	else
	{
		UTI_INFO("Duplicate logon received for ST #%u\n", tal_id);
	}


	return true;
}

bool DamaCtrl::hereIsLogoff(const Logoff *logoff)
{
	DamaTerminalList::iterator it;
	TerminalContext *terminal;
	TerminalCategories::const_iterator category_it;
	TerminalCategory *category;
	tal_id_t tal_id = logoff->getMac();

	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		UTI_DEBUG("No ST found for id %u\n", tal_id);
		return false;
	}

	terminal = (*it).second;

	// Output probes and stats
	this->gw_st_num -= 1;
	this->gw_cra_alloc_kbps -= terminal->getCra();
	this->probe_gw_cra_alloc->put(this->gw_cra_alloc_kbps);
	this->gw_rbdc_max_kbps -= terminal->getMaxRbdc();
	this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);

	// remove terminal from the list
	this->terminals.erase(terminal->getTerminalId());

	// remove terminal from the terminal category
	category_it = this->categories.find(terminal->getCurrentCategory());
	if(category_it != this->categories.end())
	{
		category = (*category_it).second;
		if(!category->removeTerminal(terminal))
		{
			return false;
		}
	}

	DC_RECORD_EVENT("LOGOFF st%d", tal_id);

	return true;
}

bool DamaCtrl::runOnSuperFrameChange(time_sf_t superframe_number_sf)
{
	DamaTerminalList::iterator it;

	this->current_superframe_sf = superframe_number_sf;

	for(it = this->terminals.begin(); it != this->terminals.end(); it++)
	{
		TerminalContext *terminal = it->second;
		// reset/update terminal allocations/requests
		terminal->onStartOfFrame();
	}

	if(!this->runDama())
	{
		UTI_ERROR("Error during DAMA computation.\n");
		return false;
	}

	return 0;
}


bool DamaCtrl::runDama()
{
	// reset the DAMA settings
	if(!this->resetDama())
	{
		UTI_ERROR("SF#%u: Cannot reset DAMA\n", this->current_superframe_sf);
		return false;
	}

	if(this->enable_rbdc && !this->runDamaRbdc())
	{
		UTI_ERROR("SF#%u: Error while computing RBDC allocation\n",
		          this->current_superframe_sf);
		return false;
	}
	if(this->enable_vbdc && !this->runDamaVbdc())
	{
		UTI_ERROR("SF#%u: Error while computing RBDC allocation\n",
		          this->current_superframe_sf);
		return false;
	}
	if(!this->runDamaFca())
	{
		UTI_ERROR("SF#%u: Error while computing RBDC allocation\n",
		          this->current_superframe_sf);
		return false;
	}
	return true;
}

void DamaCtrl::setRecordFile(FILE *event_stream, FILE *stat_stream)
{
	this->event_file = event_stream;
	DC_RECORD_EVENT("%s", "# --------------------------------------\n");
	// TODO remove stat
	this->stat_file = stat_stream;
	DC_RECORD_STAT("%s", "# --------------------------------------\n");
}


// TODO disable timers on probes if output is disabled
// and event to reactivate them ?!
void DamaCtrl::updateStatistics(time_ms_t UNUSED(period_ms))
{
	// Update probes and stats
	this->probe_gw_st_num->put(this->gw_st_num);
	this->probe_gw_cra_alloc->put(this->gw_cra_alloc_kbps);
	this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);
	for(DamaTerminalList::iterator it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		TerminalContext* terminal;
		terminal = it->second;
		this->probes_st_cra_alloc[terminal->getTerminalId()]->put(
			terminal->getCra());
		this->probes_st_rbdc_max[terminal->getTerminalId()]->put(
			terminal->getMaxRbdc());

	}
	this->probe_gw_return_remaining_capacity->put(
		this->converter->pktpfToKbps(this->gw_remaining_capacity_pktpf));
	for(TerminalCategories::iterator it = this->categories.begin();
	    it != categories.end(); ++it)
	{
		TerminalCategory* category = it->second;
		vector<CarriersGroup *> carriers;
		vector<CarriersGroup *>::const_iterator carrier_it;
		string label = category->getLabel();
		this->probes_category_return_remaining_capacity[label]->put(
			this->converter->pktpfToKbps(
				this->category_return_remaining_capacity_pktpf[label]));
		carriers = category->getCarriersGroups();
		for(carrier_it = carriers.begin();
			carrier_it != carriers.end(); ++carrier_it)
		{
			unsigned int carrier_id = (*carrier_it)->getCarriersId();
			this->probes_carrier_return_remaining_capacity[carrier_id]->put(
				this->converter->pktpfToKbps(
					this->carrier_return_remaining_capacity_pktpf[carrier_id]));
		}
	}
}

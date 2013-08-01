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


DamaCtrl::DamaCtrl():
	is_parent_init(false),
	converter(NULL),
	terminals(), // TODO not very useful, they are stocked in categories
	current_superframe_sf(0),
	frame_duration_ms(0),
	frames_per_superframe(0),
	cra_decrease(false),
	max_rbdc_kbps(0),
	rbdc_timeout_sf(0),
	min_vbdc_pkt(0),
	fca_kbps(0),
	enable_rbdc(false),
	enable_vbdc(false),
	available_bandplan_khz(0),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	fmt_simu(NULL),
	roll_off(0.0),
	stat_context()
{
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
                          vol_bytes_t packet_length_bytes,
                          bool cra_decrease,
                          rate_kbps_t max_rbdc_kbps,
                          time_sf_t rbdc_timeout_sf,
                          vol_pkt_t min_vbdc_pkt,
                          rate_kbps_t fca_kbps,
                          freq_khz_t available_bandplan_khz,
                          double roll_off,
                          TerminalCategories categories,
                          TerminalMapping affectation,
                          TerminalCategory *default_category,
                          const FmtSimulation *fmt_simu)
{
	this->frame_duration_ms = frame_duration_ms;
	this->frames_per_superframe = frames_per_superframe;
	this->cra_decrease = cra_decrease;
	this->max_rbdc_kbps = max_rbdc_kbps;
	this->rbdc_timeout_sf = rbdc_timeout_sf;
	this->min_vbdc_pkt = min_vbdc_pkt;
	this->available_bandplan_khz = available_bandplan_khz;
	this->fmt_simu = fmt_simu;

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

	this->roll_off = roll_off;

	// Compute bandplan
	if(!this->computeBandplan())
	{
		UTI_ERROR("Cannot compute band plan\n");
		return false;
	}

	this->is_parent_init = true;

	return true;
error:
	return false;
}

bool DamaCtrl::hereIsLogon(const LogonRequest &logon)
{
	tal_id_t tal_id = logon.mac;
	rate_kbps_t cra_kbps = logon.rt_bandwidth;

	UTI_DEBUG("New ST: #%u, with CRA: %u bits/sec\n", tal_id, cra_kbps);

	DamaTerminalList::iterator it;
	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		TerminalContext *terminal;
		TerminalMapping::const_iterator it;
		TerminalCategories::const_iterator category_it;
		TerminalCategory *category;

		// create the terminal
		if(!this->createTerminal(&terminal,
		                         tal_id,
		                         cra_kbps,
		                         this->max_rbdc_kbps,
		                         this->rbdc_timeout_sf,
		                         this->min_vbdc_pkt))
		{
			UTI_ERROR("Cannot create terminal context for ST #%d\n",
			          tal_id);
			return false;
		}

		// Add the new terminal to the list
		this->terminals.insert(
			std::pair<unsigned int, TerminalContext *>(tal_id, terminal));

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
		DC_RECORD_EVENT("LOGON st%d rt = %d", logon.mac,
		                logon.rt_bandwidth);
	}
	else
	{
		UTI_INFO("Duplicate logon received for ST #%d\n", tal_id);
	}

	return true;
}

bool DamaCtrl::hereIsLogoff(const LogoffRequest &logoff)
{
	DamaTerminalList::iterator it;
	TerminalContext *terminal;
	TerminalCategories::const_iterator category_it;
	TerminalCategory *category;
	tal_id_t tal_id = logoff.mac;

	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		UTI_DEBUG("No ST found for id %u\n", tal_id);
		return false;
	}
	terminal = (*it).second;
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

	// Update statistics
	this->stat_context.terminal_number = this->terminals.size();
	// statistics

	for(DamaTerminalList::iterator st = this->terminals.begin();
	    st != this->terminals.end(); ++st)
	{
		tal_id_t tal_id = st->first;
		TerminalContext *terminal = st->second;
		//uint16_t request;

		// ignore simulated ST in stats, there ID is > 100
		// TODO limitation caused by environment plane,
		//      remove if environment plane is rewritten
		// TODO create a stat that sum all simulated tal
		if(tal_id > BROADCAST_TAL_ID)
		{
			continue;
		}

		// TODO move in DamaCtrlRcs ?
		//request = terminal->getRequiredRbdc();
/*		request = terminal->getTotalRbdcRequestValue();
		if(request != 0)
		{
//			this->stat_context.rbdc_requests_number += terminal->getRbdcRequests().size();
			this->stat_context.rbdc_requests_sum_kbps += request;
		}

		//request = terminal->getRequiredVbdc();
		request = terminal->getVbdcRequest();
		if(request != 0)
		{
//			this->stat_context.vbdc_requests_number += terminal->getVbdcRequests().size();
			this->stat_context.vbdc_requests_sum_kb += request;
		}
		ENV_AGENT_Probe_PutInt(&EnvAgent,
		                       C_PROBE_GW_CRA_ST_ALLOCATION,
		                       it->first,
		                       terminal->getCra());
		ENV_AGENT_Probe_PutInt(&EnvAgent,
		                       C_PROBE_GW_RBDC_MAX_ST_ALLOCATION,
		                       it->first,
		                       terminal->getMaxRbdc());*/
		this->stat_context.total_max_rbdc_kbps += terminal->getMaxRbdc();

		this->stat_context.total_cra_kbps += terminal->getCra();
/*		ENV_AGENT_Probe_PutInt(&EnvAgent,
		                       C_PROBE_GW_RBDC_ST_ALLOCATION,
		                       St_id,
		                       (int) this->converter->
		                       ConvertFromCellsPerFrameToKbits((double) ThisSt->GetRbdc()));*/
// TODO
//		terminal->updateStatistics();
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
		UTI_ERROR("SF#%u: Error while computing RBDC allocation\n", this->current_superframe_sf);
		return false;
	}
	if(this->enable_vbdc && !this->runDamaVbdc())
	{
		UTI_ERROR("SF#%u: Error while computing RBDC allocation\n", this->current_superframe_sf);
		return false;
	}
	if(!this->runDamaFca())
	{
		UTI_ERROR("SF#%u: Error while computing RBDC allocation\n", this->current_superframe_sf);
		return false;
	}
	return true;
}

bool DamaCtrl::computeBandplan()
{
	TerminalCategories::const_iterator category_it;

	double weighted_sum_ksymps = 0.0;

	// compute weighted sum
	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    ++category_it)
	{
		TerminalCategory *category = (*category_it).second;

		// Compute weighted sum in ks/s since available bandplan is in kHz.
		weighted_sum_ksymps += category->getWeightedSum();
	}

	UTI_DEBUG_L3("Weigthed ratio sum: %f ksym/s\n", weighted_sum_ksymps);

	if(equals(weighted_sum_ksymps, 0.0))
	{
		UTI_ERROR("Weighted ratio sum is 0\n");
		goto error;
	}

	// compute carrier number per category
	for(category_it = this->categories.begin();
	    category_it !=this->categories.end();
		category_it++)
	{
		unsigned int carriers_number = 0;
		TerminalCategory *category = (*category_it).second;
		unsigned int ratio = category->getRatio();

		carriers_number = ceil(
		    (ratio / weighted_sum_ksymps) *
		    (this->available_bandplan_khz / (1 + this->roll_off)));
		UTI_DEBUG("Number of carriers for category %s: %d\n",
		          category->getLabel().c_str(), carriers_number);

		// set eh carrier numbers and capacity in carrier groups
		category->updateCarriersGroups(carriers_number,
		                               this->frame_duration_ms *
		                               this->frames_per_superframe);
	}

	return true;
error:
	return false;
}

/**
 * Set the file descriptor for storing events
 * @param event_stream  the file descriptor of an append only
 *                      opened stream used for event record
 */
// TODO ofstream
void DamaCtrl::setRecordFile(FILE * event_stream)
{
	this->event_file = event_stream;
	DC_RECORD_EVENT("%s", "# --------------------------------------\n");
}


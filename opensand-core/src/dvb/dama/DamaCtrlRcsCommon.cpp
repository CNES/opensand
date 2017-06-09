/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file DamaCtrlRcsCommon.cpp
 * @brief This library defines a generic DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieutoulouse.viveris.com>
 */


#include "DamaCtrlRcsCommon.h"
#include "OpenSandConf.h"

#include <opensand_output/Output.h>


using namespace std;

/**
 * Constructor
 */
DamaCtrlRcsCommon::DamaCtrlRcsCommon(spot_id_t spot): DamaCtrl(spot)
{
}


/**
 * Destructor
 */
DamaCtrlRcsCommon::~DamaCtrlRcsCommon()
{
}


bool DamaCtrlRcsCommon::init()
{
	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "Parent 'init()' method must be called first.\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool DamaCtrlRcsCommon::hereIsSAC(const Sac *sac)
{
	TerminalContextDamaRcs *terminal;
	tal_id_t tal_id = sac->getTerminalId();
	std::vector<cr_info_t> requests = sac->getRequests();

	// Checking if the station is registered
	// if we get GW terminal ID this is for physical layer parameters
	terminal = (TerminalContextDamaRcs *)this->getTerminalContext(tal_id);
	if(terminal == NULL && !OpenSandConf::isGw(tal_id))
	{
		LOG(this->log_sac, LEVEL_ERROR, 
		    "SF#%u: CR for an unknown st (logon_id=%u). "
		    "Discarded.\n" , this->current_superframe_sf, tal_id);
		goto error;
	}

	for(std::vector<cr_info_t>::iterator it = requests.begin();
	    it != requests.end(); ++it)
	{
		uint16_t xbdc;

		// retrieve the requested capacity
		xbdc = (*it).value;
		LOG(this->log_sac, LEVEL_INFO,
		    "SF#%u: ST%u requests %u %s\n",
		    this->current_superframe_sf, tal_id, xbdc,
		    ((*it).type == access_dama_vbdc) ?
		    "kbits in VBDC" : "kbits/s in RBDC");

		// take into account the new request
		switch((*it).type)
		{
			case access_dama_vbdc:
				this->enable_vbdc = true;
				terminal->setRequiredVbdc(xbdc);
				if(tal_id > BROADCAST_TAL_ID)
				{
					DC_RECORD_EVENT("CR st%u cr=%u type=%u",
					                tal_id, xbdc, access_dama_vbdc);
				}
				break;

			case access_dama_rbdc:
				this->enable_rbdc = true;
				if(tal_id > BROADCAST_TAL_ID)
				{
					DC_RECORD_EVENT("CR st%u cr=%u type=%u",
					                tal_id, xbdc, access_dama_rbdc);
				}
				
				// remove the CRA of the RBDC request
				// the CRA is not taken into acount on ST side
				xbdc =
					std::max(xbdc - terminal->getCra(), 0);
				terminal->setRequiredRbdc(xbdc);
				break;
		}
	}

	return true;

error:
	return false;
}

bool DamaCtrlRcsCommon::buildTTP(Ttp *ttp)
{
	TerminalCategories<TerminalCategoryDama>::const_iterator category_it;
	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    category_it++)
	{
		const std::vector<TerminalContext *> &terminals =
			(*category_it).second->getTerminals();
		
		LOG(this->log_ttp, LEVEL_DEBUG,
		    "SF#%u: Category %s has %zu terminals\n",
		    this->current_superframe_sf,
		    (*category_it).first.c_str(), terminals.size());
		for(unsigned int terminal_index = 0;
			terminal_index < terminals.size();
			terminal_index++)
		{
			TerminalContextDamaRcs *terminal =
					dynamic_cast<TerminalContextDamaRcs*>(terminals[terminal_index]);
			vol_kb_t total_allocation_kb = 0;

			// we need to do that else some CRA will be allocated and the terminal
			// will send data even if there is no MODCOD robust enough
			if(terminal->getFmtId() != 0)
			{
				total_allocation_kb += terminal->getTotalVolumeAllocation();
				total_allocation_kb += terminal->getTotalRateAllocation();
			}

			//FIXME: is the offset to be 0 ???
			if(!ttp->addTimePlan(0 /*FIXME: should it be the frame_counter of the bloc_ncc ?*/,
			                     terminal->getTerminalId(),
			                     0,
			                     total_allocation_kb,
			                     terminal->getFmtId(),
			                     0))
			{
				LOG(this->log_ttp, LEVEL_ERROR,
				    "SF#%u: cannot add TimePlan for terminal %u\n",
				    this->current_superframe_sf, terminal->getTerminalId());
				continue;
			}
		}
	}
	ttp->build();

	return true;
}

bool DamaCtrlRcsCommon::applyPepCommand(const PepRequest *request)
{
	TerminalContextDamaRcs *terminal;
	rate_kbps_t cra_kbps;
	rate_kbps_t max_rbdc_kbps;
	rate_kbps_t rbdc_kbps;

	// check that the ST is logged on
	terminal = dynamic_cast<TerminalContextDamaRcs *>(this->getTerminalContext(request->getStId()));
	if(terminal == NULL)
	{
		LOG(this->log_pep, LEVEL_ERROR, 
		    "SF#%u: ST%d is not logged on, ignore %s request\n",
		    this->current_superframe_sf, request->getStId(),
		    request->getType() == PEP_REQUEST_ALLOCATION ?
		    "allocation" : "release");
		goto abort;
	}

	// update CRA allocation ?
	cra_kbps = request->getCra();
	if(cra_kbps != 0)
	{
		// Output probes and stats
		this->gw_cra_alloc_kbps -= terminal->getCra();

		terminal->setCra(cra_kbps);
		LOG(this->log_pep, LEVEL_NOTICE,
		    "SF#%u: ST%u: update the CRA value to %u kbits/s\n",
		    this->current_superframe_sf,
		    request->getStId(), request->getCra());

		// Output probes and stats
		this->gw_cra_alloc_kbps += cra_kbps;
		this->probe_gw_cra_alloc->put(this->gw_cra_alloc_kbps);
		this->probes_st_cra_alloc[terminal->getTerminalId()]->put(cra_kbps);

	}

	// update RDBCmax threshold ?
	max_rbdc_kbps = request->getRbdcMax();
	if(max_rbdc_kbps != 0)
	{
		// Output probes and stats
		this->gw_rbdc_max_kbps -= terminal->getMaxRbdc();

		terminal->setMaxRbdc(max_rbdc_kbps);
		LOG(this->log_pep, LEVEL_NOTICE,
		    "SF#%u: ST%u: update RBDC std::max to %u kbits/s\n",
		    this->current_superframe_sf,
		    request->getStId(), request->getRbdcMax());

		// Output probes and stats
		this->gw_rbdc_max_kbps += max_rbdc_kbps;
		this->probe_gw_rbdc_max->put(this->gw_rbdc_max_kbps);
		this->probes_st_rbdc_max[terminal->getTerminalId()]->put(max_rbdc_kbps);
	}

	// inject one RDBC allocation ?
	rbdc_kbps = request->getRbdc();
	if(rbdc_kbps != 0)
	{
		// increase the RDBC timeout in order to be sure that RDBC
		// will not expire before the session is established
		terminal->updateRbdcTimeout(100);

		terminal->setRequiredRbdc(rbdc_kbps);
		LOG(this->log_pep, LEVEL_NOTICE,
		    "SF#%u: ST%u: inject RDBC request of %u kbits/s\n",
		    this->current_superframe_sf,
		    request->getStId(), request->getRbdc());

		// change back RDBC timeout
		terminal->updateRbdcTimeout(this->rbdc_timeout_sf);
	}

	return true;

abort:
	return false;
}

bool DamaCtrlRcsCommon::createTerminal(TerminalContextDama **terminal,
                                 tal_id_t tal_id,
                                 rate_kbps_t cra_kbps,
                                 rate_kbps_t max_rbdc_kbps,
                                 time_sf_t rbdc_timeout_sf,
                                 vol_kb_t max_vbdc_kb)
{
	*terminal = new TerminalContextDamaRcs(tal_id,
	                                       cra_kbps,
	                                       max_rbdc_kbps,
	                                       rbdc_timeout_sf,
	                                       max_vbdc_kb);
	if(!(*terminal))
	{
		LOG(this->log_logon, LEVEL_ERROR,
		    "SF#%u: cannot allocate terminal %u\n",
		    this->current_superframe_sf, tal_id);
		return false;
	}
	return true;
}


bool DamaCtrlRcsCommon::removeTerminal(TerminalContextDama **terminal)
{
	delete *terminal;
	*terminal = NULL;
	return true;
}

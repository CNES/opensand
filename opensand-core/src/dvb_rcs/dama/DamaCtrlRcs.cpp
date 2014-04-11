/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file DamaCtrlRcs.cpp
 * @brief This library defines a generic DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#include "DamaCtrlRcs.h"
#include "TerminalContextRcs.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <math.h>


using namespace std;

/**
 * Constructor
 */
DamaCtrlRcs::DamaCtrlRcs(): DamaCtrl()
{
}


/**
 * Destructor
 */
DamaCtrlRcs::~DamaCtrlRcs()
{
}


bool DamaCtrlRcs::init()
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

bool DamaCtrlRcs::createTerminal(TerminalContext **terminal,
                                 tal_id_t tal_id,
                                 rate_kbps_t cra_kbps,
                                 rate_kbps_t max_rbdc_kbps,
                                 time_sf_t rbdc_timeout_sf,
                                 vol_kb_t max_vbdc_kb)
{
	*terminal = new TerminalContextRcs(tal_id,
	                                   cra_kbps,
	                                   max_rbdc_kbps,
	                                   rbdc_timeout_sf,
	                                   max_vbdc_kb,
	                                   this->converter);
	if(!(*terminal))
	{
		LOG(this->log_logon, LEVEL_ERROR,
		    "SF#%u: cannot allocate terminal %u\n",
		    this->current_superframe_sf, tal_id);
		return false;
	}
	return true;
}

bool DamaCtrlRcs::removeTerminal(TerminalContext *terminal)
{
	delete terminal;
	return true;
}

bool DamaCtrlRcs::hereIsSAC(const Sac *sac)
{
	DamaTerminalList::iterator st;
	TerminalContextRcs *terminal;
	tal_id_t tal_id = sac->getTerminalId();
	std::vector<cr_info_t> requests = sac->getRequets();


	// Checking if the station is registered
	// if we get GW terminal ID this is for physical layer parameters
	st = this->terminals.find(tal_id);
	if(st == this->terminals.end() && tal_id != GW_TAL_ID)
	{
		LOG(this->log_sac, LEVEL_ERROR, 
		    "SF#%u: CR for an unknown st (logon_id=%u). "
		    "Discarded.\n" , this->current_superframe_sf, tal_id);
		goto error;
	}
	terminal = (TerminalContextRcs*) st->second; // Now st_context points to a valid context

	for(std::vector<cr_info_t>::iterator it = requests.begin();
	    it != requests.end(); ++it)
	{
		uint16_t request;
		uint16_t xbdc;

		// retrieve the requested capacity
		xbdc = (*it).value;
		LOG(this->log_sac, LEVEL_INFO,
		    "SF#%u: ST%u requests %u %s\n",
		    this->current_superframe_sf, tal_id, xbdc,
		    ((*it).type == cr_vbdc) ?
		    "slots in VBDC" : "kbits/s in RBDC");

		// take into account the new request
		switch((*it).type)
		{
			case cr_vbdc:
				this->enable_vbdc = true;
				terminal->setRequiredVbdc(xbdc);
				if(tal_id > BROADCAST_TAL_ID)
				{
					DC_RECORD_EVENT("CR ST%u value=%u type=%u",
					                tal_id, xbdc, cr_vbdc);
				}
				break;

			case cr_rbdc:
				this->enable_rbdc = true;
				if(this->cra_decrease)
				{
					// remove the CRA of the RBDC request
					xbdc =
						std::max(xbdc - terminal->getCra(), 0);
				}
				request = this->converter->kbpsToPktpf(xbdc);
				terminal->setRequiredRbdc(request);
				if(tal_id > BROADCAST_TAL_ID)
				{
					DC_RECORD_EVENT("CR ST%u value=%u type=%u",
					                tal_id, xbdc, cr_vbdc);
				}

				break;
		}
	}

	return true;

error:
	return false;
}


bool DamaCtrlRcs::buildTTP(Ttp *ttp)
{
	TerminalCategories::const_iterator category_it;
	for(category_it = this->categories.begin();
	    category_it != this->categories.end();
	    category_it++)
	{
		//const std::vector<TerminalContextRcs *> &terminals
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
			TerminalContextRcs *terminal = (TerminalContextRcs*) terminals[terminal_index];
			vol_pkt_t total_allocation_pkt = 0;

			total_allocation_pkt += terminal->getTotalVolumeAllocation();
			total_allocation_pkt += terminal->getTotalRateAllocation();

			//FIXME: is the offset to be 0 ???
			if(!ttp->addTimePlan(0 /*FIXME: should it be the frame_counter of the bloc_dvb_rcs_ncc ?*/,
			                     terminal->getTerminalId(),
			                     0,
			                     total_allocation_pkt,
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

// TODO check units here
//bool DamaCtrlRcs::applyPepCommand(const PepRequest &request)
bool DamaCtrlRcs::applyPepCommand(const PepRequest *request)
{
	DamaTerminalList::iterator it;
	TerminalContextRcs *terminal;
	rate_kbps_t cra_kbps;
	rate_kbps_t max_rbdc_kbps;
	rate_kbps_t rbdc_kbps;

	// check that the ST is logged on
	it = this->terminals.find(request->getStId());
	if(it == this->terminals.end())
	{
		LOG(this->log_pep, LEVEL_ERROR, 
		    "SF#%u: ST%d is not logged on, ignore %s request\n",
		    this->current_superframe_sf, request->getStId(),
		    request->getType() == PEP_REQUEST_ALLOCATION ?
		    "allocation" : "release");
		goto abort;
	}
	terminal = (TerminalContextRcs*)(it->second);

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
		terminal->setRbdcTimeout(100);

		terminal->setRequiredRbdc(this->converter->kbpsToPktpf(rbdc_kbps));
		LOG(this->log_pep, LEVEL_NOTICE,
		    "SF#%u: ST%u: inject RDBC request of %u kbits/s\n",
		    this->current_superframe_sf,
		    request->getStId(), request->getRbdc());

		// change back RDBC timeout
		terminal->setRbdcTimeout(this->rbdc_timeout_sf);
	}

	return true;

abort:
	return false;
}


void DamaCtrlRcs::updateFmt()
{
	DamaTerminalList::iterator terminal_it;

	for(DamaTerminalList::iterator terminal_it = this->terminals.begin();
	    terminal_it != this->terminals.end(); ++terminal_it)
    {
    	TerminalCategory *category;
		TerminalCategories::const_iterator category_it;
		TerminalContext *terminal = terminal_it->second;
		tal_id_t id = terminal->getTerminalId();
		vector<CarriersGroup *> carriers;
		unsigned int simulated_fmt;
		unsigned int available_fmt = 0; // not in the table

		// remove terminal from the terminal category
		category_it = this->categories.find(terminal->getCurrentCategory());
		if(category_it == this->categories.end())
		{
			LOG(this->log_fmt, LEVEL_ERROR,
			    "SF#%u: unable to find category associated with "
			    "terminal %u\n", this->current_superframe_sf, id);
			continue;
		}
		category = (*category_it).second;
		simulated_fmt = this->ret_fmt_simu->getCurrentModcodId(id);
		LOG(this->log_fmt, LEVEL_DEBUG,
		    "SF#%u: ST%u simulated FMT ID before affectation: %u\n",
		    this->current_superframe_sf, id, simulated_fmt);
		// get an available MODCOD id for this terminal among carriers
		carriers = category->getCarriersGroups();
		for(vector<CarriersGroup *>::const_iterator it = carriers.begin();
		    it != carriers.end(); ++it)
		{
			// FMT groups should only have one FMT id here, so get nearest should
			// return the FMT id of the carrier
			if((*it)->getNearestFmtId(simulated_fmt) == simulated_fmt)
			{
				LOG(this->log_fmt, LEVEL_DEBUG,
				    "SF#%u: ST%u will  served with the required "
				    "MODCOD (%u)\n", this->current_superframe_sf,
				    terminal->getTerminalId(), available_fmt);
				// we have a carrier with the corresponding MODCOD
				terminal->setCarrierId((*it)->getCarriersId());
				available_fmt = simulated_fmt;
				break;
			}
			// if we do not found the MODCOD value we need the closer supported value
			// MODCOD are classified from most to less robust
			if((*it)->getNearestFmtId(simulated_fmt) < simulated_fmt)
			{
				unsigned int fmt = (*it)->getNearestFmtId(simulated_fmt);
				// take the closest FMT id (i.e. the bigger value)
				available_fmt = std::max(available_fmt, fmt);
				terminal->setCarrierId((*it)->getCarriersId());
			}
		}

		if(available_fmt == 0)
		{
			LOG(this->log_fmt, LEVEL_WARNING,
			    "SF#%u: cannot serve terminal %u with simulated "
			    "MODCOD %u after affectation\n",
			    this->current_superframe_sf, id, simulated_fmt);
		}
		else
		{
			LOG(this->log_fmt, LEVEL_INFO,
			    "SF#%u: ST%u will be served with the MODCOD %u\n",
			    this->current_superframe_sf,
			    terminal->getTerminalId(), available_fmt);
		}
		// it will be 0 if the terminal cannot be served
		terminal->setFmtId(available_fmt);
	}
}


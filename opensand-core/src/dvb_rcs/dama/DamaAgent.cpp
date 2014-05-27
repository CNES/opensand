/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file    DamaAgent.cpp
 * @brief   This class defines the DAMA Agent interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */


#include "DamaAgent.h"

#include "OpenSandFrames.h"

#include <opensand_output/Output.h>

DamaAgent::DamaAgent():
	is_parent_init(false),
	packet_handler(),
	dvb_fifos(),
	group_id(0),
	current_superframe_sf(0),
	rbdc_enabled(false),
	vbdc_enabled(false),
	frame_duration_ms(0.0),
	cra_kbps(0.0),
	max_rbdc_kbps(0.0),
	rbdc_timeout_sf(0),
	max_vbdc_kb(0),
	msl_sf(0),
	cr_output_only(false)
{
}

DamaAgent::~DamaAgent()
{
}

bool DamaAgent::initParent(time_ms_t frame_duration_ms,
                           rate_kbps_t cra_kbps,
                           rate_kbps_t max_rbdc_kbps,
                           time_sf_t rbdc_timeout_sf,
                           vol_kb_t max_vbdc_kb,
                           time_sf_t msl_sf,
                           time_sf_t obr_period_sf,
                           bool cr_output_only,
                           const EncapPlugin::EncapPacketHandler *pkt_hdl,
                           const fifos_t &dvb_fifos)
{
	this->frame_duration_ms = frame_duration_ms;
	this->cra_kbps = cra_kbps;
	this->max_rbdc_kbps = max_rbdc_kbps;
	this->rbdc_timeout_sf = rbdc_timeout_sf;
	this->max_vbdc_kb = max_vbdc_kb;
	this->msl_sf = msl_sf;
	this->obr_period_sf = obr_period_sf;
	this->cr_output_only = cr_output_only;
	this->packet_handler = pkt_hdl;
	this->dvb_fifos = dvb_fifos;

	// Check if RBDC or VBDC CR are activated
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		cr_type_t cr_type = (*it).second->getCrType();
		switch(cr_type)
		{
			case cr_rbdc:
				this->rbdc_enabled = true;
				break;
			case cr_vbdc:
				this->vbdc_enabled = true;
				break;
			case cr_saloha:
			case cr_none:
				break;
			default:
				LOG(this->log_init, LEVEL_ERROR,
				    "Unknown CR type for FIFO %s: %d\n",
				    (*it).second->getName().c_str(), cr_type);
			goto error;
		}
	}

	this->is_parent_init = true;

	if (!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "the output probes and stats initialization have "
		    "failed\n");
		return false;
	}

	return true;

 error:
	return false;
}

bool DamaAgent::initOutput()
{

	// Output Log
	this->log_init = Output::registerLog(LEVEL_WARNING, "Dvb.init");
	this->log_frame_tick = Output::registerLog(LEVEL_WARNING,
	                                           "Dvb.DamaAgent.FrameTick");
	
	this->log_schedule = Output::registerLog(LEVEL_WARNING,
	                                           "Dvb.DamaAgent.Schedule");
	this->log_ttp = Output::registerLog(LEVEL_WARNING, "Dvb.TTP");
	this->log_sac = Output::registerLog(LEVEL_WARNING, "Dvb.SAC");
	this->log_request = Output::registerLog(LEVEL_WARNING,
	                                        "Dvb.DamaAgent.Request");

	// RBDC request size
	this->probe_st_rbdc_req_size = Output::registerProbe<int>(
		"Request.RBDC", "Kbps", true, SAMPLE_LAST);
	// VBDC request size
	this->probe_st_vbdc_req_size = Output::registerProbe<int>(
		"Request.VBDC", "Kbits", true, SAMPLE_LAST);
	// Total allocation
	this->probe_st_total_allocation = Output::registerProbe<int>(
		"Allocation.Total", "Kbps", true, SAMPLE_LAST);
	// Remaining allocation
	this->probe_st_remaining_allocation = Output::registerProbe<int>(
		"Allocation.Remaining", "Kbps", true, SAMPLE_LAST);

	return true;
}

bool DamaAgent::hereIsLogonResp(const LogonResponse *response)
{
	this->group_id = response->getGroupId();
	this->tal_id = response->getLogonId();
	return true;
}

bool DamaAgent::processOnFrameTick()
{
	return true;
}

bool DamaAgent::hereIsSOF(time_sf_t superframe_number_sf)
{
	this->current_superframe_sf = superframe_number_sf;
	return true;
}



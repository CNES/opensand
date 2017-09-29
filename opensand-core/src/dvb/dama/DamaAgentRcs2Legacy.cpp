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
 * @file    DamaAgentRcs2Legacy.cpp
 * @brief   Implementation of the DAMA agent for DVB-RCS2 emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 * @author  Aurelien Delrieu / Viveris Technologies
 *
 */


#include "DamaAgentRcs2Legacy.h"

#include <opensand_output/Output.h>

#include <algorithm>
#include <cmath>

// constants
const rate_kbps_t C_MAX_RBDC_IN_SAC = 16320.0; // 16320 kbits/s, limitation due
                                               // to CR value size in to SAC field
const vol_kb_t C_MAX_VBDC_IN_SAC = 4080;       // 4080 packets/ceils, limitation
                                               // due to CR value size in to SAC field

using std::max;
using std::min;

DamaAgentRcs2Legacy::DamaAgentRcs2Legacy(FmtDefinitionTable *ret_modcod_def):
	DamaAgentRcs2(ret_modcod_def),
	vbdc_credit_kb(0)
{
}

DamaAgentRcs2Legacy::~DamaAgentRcs2Legacy()
{
}

bool DamaAgentRcs2Legacy::hereIsSOF(time_sf_t superframe_number_sf)
{
	// Call parent method
	if(!DamaAgentRcs2::hereIsSOF(superframe_number_sf))
	 {
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u: cannot call DamaAgentRcs2::hereIsSOF()\n",
		    this->current_superframe_sf);
		return false;
	}

	this->rbdc_timer_sf++;
	// update dynamic allocation for next SF with allocation received
	// through TBTP during last SF
	this->dynamic_allocation_kb = this->allocated_kb;
	this->allocated_kb = 0;

	return true;
}

rate_kbps_t DamaAgentRcs2Legacy::computeRbdcRequest()
{
	rate_kbps_t rbdc_request_kbps;
	vol_b_t rbdc_length_b;
	vol_b_t rbdc_pkt_arrival_b;
	rate_kbps_t rbdc_req_in_previous_msl_kbps;
	double req_kbps = 0.0;

	/* get data length of outstanding packets in RBDC related MAC FIFOs */
	rbdc_length_b = this->getMacBufferLength(access_dama_rbdc);

	// Get data length of packets arrived in RBDC related IP FIFOs since
	// last RBDC request sent
	// NB: arrivals in MAC FIFOs must NOT be taken into account because these
	// packets represent only packets buffered because there is no
	// more available allocation, but their arrival has been taken into account
	// in IP fifos
	rbdc_pkt_arrival_b = this->getMacBufferArrivals(access_dama_rbdc);

	// get the sum of RBDC request during last MSL
	rbdc_req_in_previous_msl_kbps = this->rbdc_request_buffer->GetSum();

	// TODO original algo was rbdc_length - rbdc_arrivals but
	//      this does not work for first packet and I don't understand comment !
	//req_kbps = (int)((rbdc_length_b - rbdc_pkt_arrival_b) -
	req_kbps = (int)(rbdc_length_b -
	                 (this->rbdc_timer_sf * this->frame_duration_ms *
	                  rbdc_req_in_previous_msl_kbps)) /
	           (int)(this->frame_duration_ms * this->msl_sf);
	req_kbps = ceil(req_kbps);
	/* compute rate need: estimation of the need of bandwith for traffic  */
	if(this->rbdc_timer_sf != 0)
	{
		/* kbps = bpms */
		rbdc_request_kbps = (int)ceil(rbdc_pkt_arrival_b /
		                    (this->rbdc_timer_sf * this->frame_duration_ms)) +
		                    max(0, (int)req_kbps);
	}
	else
	{
		rbdc_request_kbps = max(0, (int)req_kbps);
	}

	LOG(this->log_request, LEVEL_DEBUG,
	    "SF#%u: RBDC Timer = %u, RBDC Length = %u bits"
	    ", RBDC packet arrival length = %u bits, previous RBDC request in "
	    "MSL = %u kbits/s, rate need = %u kbits/s\n",
	    this->current_superframe_sf,
	    this->rbdc_timer_sf, rbdc_length_b,
	    rbdc_pkt_arrival_b, rbdc_req_in_previous_msl_kbps,
	    rbdc_request_kbps);
  
	LOG(this->log_request, LEVEL_INFO,
	    "SF#%u: theoretical RBDC request = %u kbits/s",
	    this->current_superframe_sf,
	    rbdc_request_kbps);

	rbdc_request_kbps = min(rbdc_request_kbps, this->max_rbdc_kbps);
	LOG(this->log_request, LEVEL_DEBUG,
	    "updated RBDC request = %u kbits/s (in fonction of max "
	    "RBDC)\n", rbdc_request_kbps);

	/* reduce the request value to the maximum theorical value if required */
	rbdc_request_kbps = min(rbdc_request_kbps, C_MAX_RBDC_IN_SAC);

	LOG(this->log_request, LEVEL_DEBUG,
	    "SF#%u: updated RBDC request = %u kbits/s in "
	    "SAC\n", this->current_superframe_sf,
	    rbdc_request_kbps);

	return rbdc_request_kbps;
}

vol_kb_t DamaAgentRcs2Legacy::computeVbdcRequest()
{
	vol_kb_t vbdc_need_kb;
	vol_kb_t vbdc_request_kb;
	// TODO there is a problem with vbdc_credit ! it is not decreased !
	//      At the moment, set 0
	//      we may decrease it from allocated packets number
	//      or from the number of packets removed in fifo with
	//      getRemoved accessor and resetRemoved in FIFO
	//      Whatever, the VBDC algorithm is very bad !
	this->vbdc_credit_kb = 0;

	/* get number of outstanding packets in VBDC related MAC
	 * and IP FIFOs (in packets number) */
	vbdc_need_kb = ceil(this->getMacBufferLength(access_dama_vbdc) / 1000.);
	LOG(this->log_request, LEVEL_DEBUG,	
	    "SF#%u: MAC buffer length = %d kbits, VBDC credit = "
	    "%u kbits\n", this->current_superframe_sf,
	    vbdc_need_kb, this->vbdc_credit_kb);

	/* compute VBDC request: actual Vbdc request to be sent */
	vbdc_request_kb = max(0, (vbdc_need_kb - this->vbdc_credit_kb));
	LOG(this->log_request, LEVEL_DEBUG,
	    "SF#%u: theoretical VBDC request = %u kbits",
	    this->current_superframe_sf,
	    vbdc_request_kb);

	/* adjust request in function of max_vbdc value */
	vbdc_request_kb = min(vbdc_request_kb, this->max_vbdc_kb);

	// Ensure VBDC request value is not greater than SAC field
	vbdc_request_kb = min(vbdc_request_kb, C_MAX_VBDC_IN_SAC);
	LOG(this->log_request, LEVEL_DEBUG,
	    "updated VBDC request = %d kbits in fonction of "
	    "max VBDC and max VBDC in SAC\n", vbdc_request_kb);

	/* update VBDC Credit here */
	/* NB: the computed VBDC is always really sent if not null */
	this->vbdc_credit_kb += vbdc_request_kb;
	LOG(this->log_request, LEVEL_NOTICE,
	    "updated VBDC request = %d kbits in SAC, VBDC credit = "
	    "%u kbits\n", vbdc_request_kb, this->vbdc_credit_kb);

	return vbdc_request_kb;
}

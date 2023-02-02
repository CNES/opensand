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
 * @file    DamaAgentRcs2Legacy.cpp
 * @brief   Implementation of the DAMA agent for DVB-RCS2 emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 * @author  Aurelien Delrieu / Viveris Technologies
 */


#include "DamaAgentRcs2Legacy.h"

#include <opensand_output/Output.h>

#include <algorithm>
#include <cmath>


DamaAgentRcs2Legacy::DamaAgentRcs2Legacy(FmtDefinitionTable *ret_modcod_def):
	DamaAgentRcs2(ret_modcod_def),
	vbdc_credit_kb(0)
{
}

DamaAgentRcs2Legacy::~DamaAgentRcs2Legacy()
{
}

rate_kbps_t DamaAgentRcs2Legacy::computeRbdcRequest()
{
	rate_kbps_t rbdc_request_kbps;
	vol_b_t rbdc_length_b;
	vol_b_t rbdc_pkt_arrival_b;
	rate_kbps_t rbdc_req_in_previous_msl_kbps;

	/* get data length of outstanding packets in RBDC related MAC FIFOs */
	rbdc_length_b = this->getMacBufferLength(ReturnAccessType::dama_rbdc);

	// Get data length of packets arrived in RBDC related IP FIFOs since
	// last RBDC request sent
	// NB: arrivals in MAC FIFOs must NOT be taken into account because these
	// packets represent only packets buffered because there is no
	// more available allocation, but their arrival has been taken into account
	// in IP fifos
	rbdc_pkt_arrival_b = this->getMacBufferArrivals(ReturnAccessType::dama_rbdc);

	// get the sum of RBDC request during last MSL
	rbdc_req_in_previous_msl_kbps = this->rbdc_request_buffer->GetSum();

	// TODO original algo was rbdc_length - rbdc_arrivals but
	//      this does not work for first packet and I don't understand comment !
	rate_kbps_t req_kbps = 0;
	auto granted = this->rbdc_timer_sf * rbdc_req_in_previous_msl_kbps * this->frame_duration;
	auto rbdc_length = time_ms_t(rbdc_length_b);
	if (granted < rbdc_length)
	{
		req_kbps = (rbdc_length - granted) / (this->msl_sf * this->frame_duration);
	}
	/* compute rate need: estimation of the need of bandwith for traffic  */
	if(this->rbdc_timer_sf != 0)
	{
		/* kb/s = b/ms */
		rbdc_request_kbps = (time_ms_t(rbdc_pkt_arrival_b) / (this->rbdc_timer_sf * this->frame_duration)) + req_kbps;
	}
	else
	{
		rbdc_request_kbps = req_kbps;
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

	// reduce the request value to the maximum theorical value if required
	rbdc_request_kbps = this->checkRbdcRequest(rbdc_request_kbps);

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
	vbdc_need_kb = ceil(this->getMacBufferLength(ReturnAccessType::dama_vbdc) / 1000.);
	LOG(this->log_request, LEVEL_DEBUG,	
	    "SF#%u: MAC buffer length = %d kbits, VBDC credit = "
	    "%u kbits\n", this->current_superframe_sf,
	    vbdc_need_kb, this->vbdc_credit_kb);

	/* compute VBDC request: actual Vbdc request to be sent */
	vbdc_request_kb = std::max(0, (vbdc_need_kb - this->vbdc_credit_kb));
	LOG(this->log_request, LEVEL_DEBUG,
	    "SF#%u: theoretical VBDC request = %u kbits",
	    this->current_superframe_sf,
	    vbdc_request_kb);

	// Ensure VBDC request value is not greater than SAC field
	vbdc_request_kb = this->checkVbdcRequest(vbdc_request_kb);
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

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
 * @file    DamaAgentRcsLegacy.cpp
 * @brief   Implementation of the DAMA agent for DVB-S2 emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 *
 */


#include "DamaAgentRcsLegacy.h"

#include <opensand_output/Output.h>

#include <algorithm>
#include <cmath>

// constants
const rate_kbps_t C_MAX_RBDC_IN_SAC = 8160.0; // 8160 kbits/s, limitation due
                                              // to CR value size in to SAC field
const vol_pkt_t C_MAX_VBDC_IN_SAC = 4080;     // 4080 packets/ceils, limitation
                                              // due to CR value size in to SAC field

using std::max;
using std::min;

DamaAgentRcsLegacy::DamaAgentRcsLegacy():
	DamaAgentRcs(),
	vbdc_credit_pkt(0)
{
}

DamaAgentRcsLegacy::~DamaAgentRcsLegacy()
{
}

bool DamaAgentRcsLegacy::init()
{
	if(!DamaAgentRcs::init())
	{
		UTI_ERROR("Cannot call DamaAgentRcs::init()");
		return false;
	}

	return true;
}

bool DamaAgentRcsLegacy::hereIsSOF(time_sf_t superframe_number_sf)
{
	// Call parent method
	if(!DamaAgentRcs::hereIsSOF(superframe_number_sf))
	 {
		UTI_ERROR("SF#%u: cannot call DamaAgentRcs::hereIsSOF()\n",
		          this->current_superframe_sf);
		return false;
	}

	this->rbdc_timer_sf++;
	// update dynamic allocation for next SF with allocation received
	// through TBTP during last SF
	this->dynamic_allocation_pkt = this->allocated_pkt;
	this->allocated_pkt = 0;

	return true;
}

rate_kbps_t DamaAgentRcsLegacy::computeRbdcRequest()
{
	rate_kbps_t rbdc_request_kbps;
	rate_kbps_t rbdc_limit_kbps;
	vol_b_t rbdc_length_b;
	vol_b_t rbdc_pkt_arrival_b;
	rate_kbps_t rbdc_req_in_previous_msl_kbps;
	double req_kbps = 0.0;

	/* get number of outstanding packets in RBDC related MAC FIFOs */
	rbdc_length_b =
		this->converter->pktToBits(this->getMacBufferLength(cr_rbdc));

	// Get number of packets arrived in RBDC related IP FIFOs since
	// last RBDC request sent
	// NB: arrivals in MAC FIFOs must NOT be taken into account because these
	// packets represent only packets buffered because there is no
	// more available allocation, but their arrival has been taken into account
	// in IP fifos
	rbdc_pkt_arrival_b =
		this->converter->pktToBits(this->getMacBufferArrivals(cr_rbdc));

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

	UTI_DEBUG_L3("SF#%u: frame %u: RBDC Timer = %u, RBDC Length = %u bytes, "
	             "RBDC packet arrival = %u, previous RBDC request in "
	             "MSL = %u kb/s, rate need = %u kb/s\n",
	             this->current_superframe_sf, this->current_frame,
	             this->rbdc_timer_sf, rbdc_length_b,
	             rbdc_pkt_arrival_b, rbdc_req_in_previous_msl_kbps,
	             rbdc_request_kbps);

	UTI_DEBUG("SF#%u: frame %u: theoretical RBDC request = %u kbits/s",
	          this->current_superframe_sf, this->current_frame,
	          rbdc_request_kbps);

	/* adjust request in function of max RBDC and fixed allocation */
	if(!this->cra_in_cr)
	{
		rbdc_limit_kbps = this->max_rbdc_kbps + this->cra_kbps;
	}
	else
	{
		rbdc_limit_kbps = this->max_rbdc_kbps;
	}
	rbdc_request_kbps = min(rbdc_request_kbps, rbdc_limit_kbps);
	UTI_DEBUG_L3("updated RBDC request = %u kbits/s "
	             "(in fonction of max RBDC and CRA)\n", rbdc_request_kbps);

	/* reduce the request value to the maximum theorical value if required */
	rbdc_request_kbps = min(rbdc_request_kbps, C_MAX_RBDC_IN_SAC);

	UTI_DEBUG_L3("SF#%u: frame %u: updated RBDC request = %u kbits/s in SAC\n",
	             this->current_superframe_sf, this->current_frame,
	             rbdc_request_kbps);

	return rbdc_request_kbps;
}

vol_pkt_t DamaAgentRcsLegacy::computeVbdcRequest()
{
	vol_pkt_t vbdc_need_pkt;
	vol_pkt_t vbdc_request_pkt;
	vol_pkt_t max_vbdc_pkt = this->converter->kbitsToPkt(this->max_vbdc_kb);
	// TODO there is a problem with vbdc_credit ! it is not decreased !
	//      At the moment, set 0
	//      we may decrease it from allocated packets number
	//      or from the number of packets removed in fifo with
	//      getRemoved accessor and resetRemoved in FIFO
	//      Whatever, the VBDC algorithm is very bad !
	this->vbdc_credit_pkt = 0;

	/* get number of outstanding packets in VBDC related MAC
	 * and IP FIFOs (in packets number) */
	vbdc_need_pkt = this->getMacBufferLength(cr_vbdc);
	UTI_ERROR("SF#%u: frame %u: MAC buffer length = %d, VBDC credit = %u\n",
	             this->current_superframe_sf, this->current_frame,
	             vbdc_need_pkt, this->vbdc_credit_pkt);

	/* compute VBDC request: actual Vbdc request to be sent */
	vbdc_request_pkt = max(0, (vbdc_need_pkt - this->vbdc_credit_pkt));
	UTI_ERROR("SF#%u: frame %u: theoretical VBDC request = %u packets",
	             this->current_superframe_sf, this->current_frame,
	             vbdc_request_pkt);

	/* adjust request in function of max_vbdc value */
	vbdc_request_pkt = min(vbdc_request_pkt, max_vbdc_pkt);

	// Ensure VBDC request value is not greater than SAC field
	vbdc_request_pkt = min(vbdc_request_pkt, C_MAX_VBDC_IN_SAC);
	UTI_ERROR("updated VBDC request = %d packets in fonction of "
	             "max VBDC and max VBDC in SAC\n", vbdc_request_pkt);

	/* update VBDC Credit here */
	/* NB: the computed VBDC is always really sent if not null */
	this->vbdc_credit_pkt += vbdc_request_pkt;
	UTI_ERROR("updated VBDC request = %d packets in SAC, VBDC credit = %u\n",
	             vbdc_request_pkt, this->vbdc_credit_pkt);

	return vbdc_request_pkt;
}



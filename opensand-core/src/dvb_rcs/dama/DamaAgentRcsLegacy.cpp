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

#include "MacFifoElement.h"

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
	cra_in_cr(false),
	rbdc_timer_sf(0),
	vbdc_credit_pkt(0)
{
}

DamaAgentRcsLegacy::~DamaAgentRcsLegacy()
{
	delete this->ret_schedule;

	if(this->rbdc_request_buffer != NULL)
	{
		delete this->rbdc_request_buffer;
	}

	delete this->converter;
}

bool DamaAgentRcsLegacy::init()
{
	this->ret_schedule = new ReturnSchedulingRcs(this->packet_handler,
	                                             this->dvb_fifos);

	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_none)
		{
			this->cra_in_cr = true;
		}
	}

	if(this->rbdc_enabled)
	{
		// create circular buffer for saving last RBDC requests during the past
		// MSL duration with size = integer part of MSL / OBR period
		// (in frame number)
		// NB: if size = 0, only last req is saved and sum is always 0
		this->rbdc_request_buffer =
			new CircularBuffer((size_t) this->msl_sf / this->obr_period_sf);
		if(this->rbdc_request_buffer == NULL)
		{
			UTI_ERROR("Cannot create circular buffer to save "
			          "the last RBDC requests\n");
			goto error;
		}
	}

	// Initializes unit converter
	this->converter = new UnitConverter(this->packet_handler->getFixedLength(),
	                                    this->frame_duration_ms);

	return true;

 error:
	return false;
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

// a TTP reading function that handles MODCOD but not priority
bool DamaAgentRcsLegacy::hereIsTTP(Ttp &ttp)
{
	map<uint8_t, emu_tp_t> tp;

	/*
	if(this->group_id != ttp->group_id)
	{
		UTI_DEBUG_L3("SF#%u: TTP with different group_id (%d).\n",
		             this->current_superframe_sf, ttp->group_id);
		goto end;
	}*/

	if(!ttp.getTp(this->tal_id, tp))
	{
		return true;
	}

	for(map<uint8_t, emu_tp_t>::iterator it = tp.begin();
	    it != tp.end(); ++it)
	{
		time_pkt_t assign_pkt;

		assign_pkt = (*it).second.assignment_count;
		this->allocated_pkt += assign_pkt;
		UTI_DEBUG_L3("SF#%u: frame#%u: "
		             "offset:%u, assignment_count:%u, "
		             "fmt_id:%u priority:%u\n",
		             ttp.getSuperframeCount(),
		             (*it).first,
		             (*it).second.offset,
		             assign_pkt,
		             (*it).second.fmt_id,
		             (*it).second.priority);
	}

	UTI_DEBUG("SF#%u: allocated TS=%u\n",
	          ttp.getSuperframeCount(), this->allocated_pkt);
	return true;
}


bool DamaAgentRcsLegacy::processOnFrameTick()
{
	// Call parent method
	if(!DamaAgentRcs::processOnFrameTick())
	{
		UTI_ERROR("SF#%u: cannot call DamaAgentRcs::processOnFrameTick()\n",
		          this->current_superframe_sf);
		return false;
	}

	// TODO do we convert from pkt per sf or from pkt per frame ?????
	// TODO remove stat context and use probe directly
	// TODO move stats in updateStats
	this->stat_context.global_alloc_kbps =
		this->converter->pktpfToKbps(this->remaining_allocation_pktpf);

	return true;
}


bool DamaAgentRcsLegacy::buildCR(cr_type_t cr_type,
                                 CapacityRequest &capacity_request,
                                 bool &empty)
{
	bool send_rbdc_request = false;
	bool send_vbdc_request = false;
	rate_kbps_t rbdc_request_kbps = 0;
	vol_pkt_t vbdc_request_pkt = 0;
	empty = false;

	// Compute RBDC request if needed
	if(this->rbdc_enabled)
	{
		UTI_DEBUG("SF#%u: compute RBDC request\n",
		          this->current_superframe_sf);
		rbdc_request_kbps = this->computeRbdcRequest();

		// Send the request only if current RBDC timer > RBDC timeout / 2
		// or if CR is different from previous one
		if(rbdc_request_kbps > 0)
		{
// TODO do we keep that ? if not remove RBDC timeout from DAMA Agent ?
//      RBDC timeout is useful for inband request so
//      maybe we should keep it (but renaming it)
#ifdef OPTIMIZE
			if(rbdc_request_kbps != this->rbdc_request_buffer->GetPreviousValue()
			   || this->rbdc_timer_sf > (this->rbdc_timeout_sf / 2))

#endif
				send_rbdc_request = true;
#ifdef OPTIMIZE
			}
#endif
		}
		else
		{
			if(rbdc_request_kbps != this->rbdc_request_buffer->GetPreviousValue())
			{
				send_rbdc_request = true;
			}
		}
	}

	// Compute VBDC request if required
	if(this->vbdc_enabled)
	{
		UTI_DEBUG("SF#%u: Compute VBDC request\n", this->current_superframe_sf);
		vbdc_request_pkt = this->computeVbdcRequest();

		// Send the request only if it is not null
		if(vbdc_request_pkt > 0)
		{
			send_vbdc_request = true;
		}
	}

	// if no valid CR is built: skipping it
	if(!send_rbdc_request && !send_vbdc_request)
	{
		UTI_DEBUG_L3("SF#%u: RBDC CR = %d, VBDC CR = %d, no CR built.\n",
		             this->current_superframe_sf, rbdc_request_kbps,
		             vbdc_request_pkt);
		empty = true;
		goto end;
	}

	// set RBDC request (if any) in SAC
	if(send_rbdc_request)
	{
		capacity_request.addRequest(0, cr_rbdc, rbdc_request_kbps);

		// update variables used for next RBDC CR computation
		this->rbdc_timer_sf = 0;
		this->rbdc_request_buffer->Update(rbdc_request_kbps);

		// reset counter of arrival packets in MAC FIFOs related to RBDC
		for(fifos_t::const_iterator it = this->dvb_fifos.begin();
		    it != this->dvb_fifos.end(); ++it)
		{
			(*it).second->resetNew(cr_rbdc);
		}
	}

	// set VBDC request (if any) in SAC
	if(send_vbdc_request)
	{
		capacity_request.addRequest(0, cr_vbdc, vbdc_request_pkt);
	}

	this->stat_context.rbdc_request_kbps = rbdc_request_kbps;
	this->stat_context.vbdc_request_kb = this->converter->pktToKbits(vbdc_request_pkt);
	UTI_DEBUG("SF#%u: build CR with %u kb/s in RBDC and %u packets in VBDC",
	          this->current_superframe_sf, rbdc_request_kbps, vbdc_request_pkt);

 end:
	return true;
}

bool DamaAgentRcsLegacy::returnSchedule(list<DvbFrame *> *complete_dvb_frames)
{
	rate_kbps_t remaining_alloc_kbps;
	uint32_t remaining_alloc_pktpf = this->remaining_allocation_pktpf;

	UTI_DEBUG_L3("SF#%u: frame %u: allocation before scheduling %u\n",
	             this->current_superframe_sf, this->current_frame,
	             remaining_alloc_pktpf);
	if(!this->ret_schedule->schedule(this->current_superframe_sf,
	                                 this->current_frame,
	                                 0,
	                                 complete_dvb_frames,
	                                 remaining_alloc_pktpf))
	{
		UTI_ERROR("SF#%u: frame %u: Uplink Scheduling failed",
		          this->current_superframe_sf, this->current_frame);
		return false;
	}
	UTI_DEBUG_L3("SF#%u: frame %u: remaining allocation after scheduling %u\n",
	             this->current_superframe_sf, this->current_frame,
	             remaining_alloc_pktpf);
	this->remaining_allocation_pktpf = remaining_alloc_pktpf;

	remaining_alloc_kbps = this->converter->pktpfToKbps(this->remaining_allocation_pktpf);
	this->stat_context.unused_alloc_kbps = remaining_alloc_kbps;

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
	// TODO limits
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

vol_pkt_t DamaAgentRcsLegacy::getMacBufferLength(cr_type_t cr_type)
{
	vol_pkt_t nb_pkt_in_fifo; // absolute number of packets in fifo

	nb_pkt_in_fifo = 0;
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_type)
		{
			nb_pkt_in_fifo += (*it).second->getCurrentSize();
		}
	}

	return nb_pkt_in_fifo;
}


vol_pkt_t DamaAgentRcsLegacy::getMacBufferArrivals(cr_type_t cr_type)
{
	vol_pkt_t nb_pkt_input; // packets that filled the queue since last RBDC request

	nb_pkt_input = 0;
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_type)
		{
			nb_pkt_input += (*it).second->getNewSize();
		}
	}

	return nb_pkt_input;
}

void DamaAgentRcsLegacy::updateStatistics()
{
	//TODO
}

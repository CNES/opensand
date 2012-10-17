/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 *
 * TODO: I think the VBDC computation algorithm is messed-up and should be
 * re-validated.
 */


#include "DamaAgentRcsLegacy.h"

#include "MacFifoElement.h"

#include <algorithm>
#include <cmath>

#define DBG_PACKAGE PKG_DAMA_DA
#include "opensand_conf/uti_debug.h"

// constants
const rate_kbps_t C_MAX_RBDC_IN_SAC = 8160.0; // 8160 kbits/s, limitation due
                                              // to CR value size in to SAC field
const vol_pkt_t C_MAX_VBDC_IN_SAC = 4080;     // 4080 packets/ceils, limitation
                                              // due to CR value size in to SAC field


DamaAgentRcsLegacy::DamaAgentRcsLegacy(const EncapPlugin::EncapPacketHandler *pkt_hdl,
                                       const std::map<unsigned int, DvbFifo *> &dvb_fifos):
	DamaAgentRcs(pkt_hdl, dvb_fifos),
	cra_in_cr(false),
	rbdc_timer_sf(0),
	vbdc_credit_pkt(0),
	up_schedule(pkt_hdl, dvb_fifos)
{
}

DamaAgentRcsLegacy::~DamaAgentRcsLegacy()
{
	if(this->rbdc_request_buffer != NULL)
	{
		delete this->rbdc_request_buffer;
	}

	delete this->converter;
}

bool DamaAgentRcsLegacy::init()
{
	for(std::map<unsigned int, DvbFifo *>::const_iterator it = this->dvb_fifos.begin();
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
	this->converter = new DU_Converter(this->frame_duration_ms,
	                                   this->packet_handler->getFixedLength());

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

bool DamaAgentRcsLegacy::processOnFrameTick()
{
	// Call parent method
	if(!DamaAgentRcs::processOnFrameTick())
	{
		UTI_ERROR("SF#%u: cannot call DamaAgentRcs::processOnFrameTick()\n",
		          this->current_superframe_sf);
		return false;
	}

	this->stat_context.global_alloc_kbps = 
	   this->converter->ConvertFromCellsPerFrameToKbits(this->remaining_allocation_pktpsf);

	return true;
}


// TODO: implement these
bool DamaAgentRcsLegacy::buildCR(cr_type_t cr_type,
                                 CapacityRequest **capacity_request,
                                 bool &empty)
{
	UTI_ERROR("SF#%u: You should not be here!\n", this->current_superframe_sf);
	return false;
}

bool DamaAgentRcsLegacy::hereIsTTP(const Ttp &ttp)
{
	UTI_ERROR("SF#%u: You should not be here!\n", this->current_superframe_sf);
	return false;
}

/**  Wrappers **/
bool DamaAgentRcsLegacy::buildCR(cr_type_t cr_type,
                                 unsigned char *frame,
                                 size_t &length,
                                 bool &empty)
{
	T_DVB_SAC_CR *init_cr;
	bool send_rbdc_request = false;
	bool send_vbdc_request = false;
	rate_kbps_t rbdc_request_kbps = 0; 
	vol_pkt_t vbdc_request_pkt = 0;
	unsigned int cr_number = 0;
	empty = false;

	if(length < sizeof(T_DVB_SAC_CR))
	{
		UTI_ERROR("SF#%u: Buffer size too small to fit T_DVB_SAC_CR\n",
		           this->current_superframe_sf);
		goto error;
	}

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
		             this->current_superframe_sf, rbdc_request_kbps, vbdc_request_pkt);
		empty = true;
		goto end;
	}

	init_cr = (T_DVB_SAC_CR *) frame;
	init_cr->hdr.msg_length = sizeof(T_DVB_SAC_CR);
	init_cr->hdr.msg_type = MSG_TYPE_CR;

	if(!send_rbdc_request || !send_vbdc_request)
	{
		cr_number = 1;
	}
	else
	{
		cr_number = 2;
	}

	init_cr->cr_number = cr_number;
	cr_number--;

	// set RBDC request (if any) in SAC
	if(send_rbdc_request)
	{
		init_cr->cr[cr_number].route_id = 0;
		init_cr->cr[cr_number].type = cr_rbdc;
		init_cr->cr[cr_number].channel_id = 0;
		encode_request_value(&(init_cr->cr[cr_number]), rbdc_request_kbps);
		init_cr->cr[cr_number].group_id = this->group_id;
		init_cr->cr[cr_number].logon_id = this->tal_id;
		init_cr->cr[cr_number].M_and_C = 0;

		// update variables used for next RBDC CR computation
		this->rbdc_timer_sf = 0;
		this->rbdc_request_buffer->Update(rbdc_request_kbps);

		// reset counter of arrival packets/cells in MAC FIFOs related to RBDC
		for(std::map<unsigned int, DvbFifo *>::const_iterator it = this->dvb_fifos.begin();
		    it != this->dvb_fifos.end(); ++it)
		{
			(*it).second->resetNew(cr_rbdc);
		}

		cr_number--;
	}

	// set VBDC request (if any) in SAC
	if(send_vbdc_request)
	{
		init_cr->cr[cr_number].route_id = 0;
		init_cr->cr[cr_number].type = cr_vbdc;
		init_cr->cr[cr_number].channel_id = 0;
		encode_request_value(&(init_cr->cr[cr_number]), vbdc_request_pkt);
		init_cr->cr[cr_number].group_id = this->group_id;
		init_cr->cr[cr_number].logon_id = this->tal_id;
		init_cr->cr[cr_number].M_and_C = 0;
	}

	stat_context.rbdc_request_kbps = rbdc_request_kbps;
	stat_context.vbdc_request_pkt = vbdc_request_pkt;

 end:
	return true;
 error:
	return false;
}

bool DamaAgentRcsLegacy::uplinkSchedule(std::list<DvbFrame *> *complete_dvb_frames)
{
	if(!this->up_schedule.schedule(this->current_superframe_sf,
	                               this->current_frame,
	                               complete_dvb_frames,
	                               this->remaining_allocation_pktpsf))
	{
		UTI_ERROR("SF#%u: frame %u: Uplink Scheduling failed",
		          this->current_superframe_sf, this->current_frame);
		return false;
	}
	this->stat_context.unused_alloc_kbps =  // unused bandwith in kbits/s
	        this->converter->ConvertFromCellsPerFrameToKbits(this->remaining_allocation_pktpsf);

	return true;
}

rate_kbps_t DamaAgentRcsLegacy::computeRbdcRequest()
{
	rate_kbps_t rbdc_request_kbps;
	rate_kbps_t rbdc_limit_kbps;
	vol_b_t rbdc_length_b;
	vol_b_t rbdc_pkt_arrival_b;
	rate_kbps_t rbdc_req_in_previous_msl_kbps;

	/* get number of outstanding packets/cells in RBDC related MAC FIFOs */
	rbdc_length_b =
		this->converter->pktToBits(this->getMacBufferLength(cr_rbdc));

	// Get number of packets/cells arrived in RBDC related IP FIFOs since
	// last RBDC request sent
	// NB: arrivals in MAC FIFOs must NOT be taken into account because these
	// packets/cells represent only packets/cells buffered because there is no
	// more available allocation, but its arrival has been taken into account
	// in IP fifos
	rbdc_pkt_arrival_b =
		this->converter->pktToBits(this->getMacBufferArrivals(cr_rbdc));

	// get the sum of RBDC request during last MSL
	rbdc_req_in_previous_msl_kbps = this->rbdc_request_buffer->GetSum();

	/* compute rate need: estimation of the need of bandwith for traffic - in cells/sec */
	if(this->rbdc_timer_sf != 0.0)
	{
		/* kbps = bpms */
		rbdc_request_kbps =
		    rbdc_pkt_arrival_b / (this->rbdc_timer_sf * this->frame_duration_ms) +
		    std::max((unsigned int ) 0,
		             (rbdc_length_b - rbdc_pkt_arrival_b -
		             (this->rbdc_timer_sf * this->frame_duration_ms *
		              rbdc_req_in_previous_msl_kbps)) /
		             (this->frame_duration_ms * this->msl_sf));
	}
	else
	{
		rbdc_request_kbps =
		   std::max((unsigned int) 0,
		            (rbdc_length_b - rbdc_pkt_arrival_b) -
		            (this->rbdc_timer_sf * this->frame_duration_ms *
		             rbdc_req_in_previous_msl_kbps) /
		            (this->frame_duration_ms * this->msl_sf));
	}

	UTI_DEBUG_L3("SF#%u: frame %u: RBDC Timer = %u, RBDC Length = %u bytes, "
	             "RBDC cell arrival = %u, previous RBDC request in MSL = %u kb/s, "
	             "rate need = %u kb/s\n",
	             this->current_superframe_sf, this->current_frame,
	             this->rbdc_timer_sf, rbdc_length_b,
	             rbdc_pkt_arrival_b, rbdc_request_kbps, rbdc_request_kbps);

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
	rbdc_request_kbps = std::min(rbdc_request_kbps, rbdc_limit_kbps);
	UTI_DEBUG_L3("updated RBDC request = %u kbits/s "
	             "(in fonction of max RBDC and CRA)\n", rbdc_request_kbps);

	/* reduce the request value to the maximum theorical value if required */
	rbdc_request_kbps = std::min(rbdc_request_kbps, C_MAX_RBDC_IN_SAC);

	UTI_DEBUG_L3("SF#%u: frame %u: updated RBDC request = %u kbits/s in SAC\n",
	             this->current_superframe_sf, this->current_frame,
	             rbdc_request_kbps);

	return rbdc_request_kbps;
}

vol_pkt_t DamaAgentRcsLegacy::computeVbdcRequest()
{
	vol_pkt_t vbdc_need_pkt;
	vol_pkt_t vbdc_request_pkt;

	/* get number of outstanding packets/cells in VBDC related MAC
	 * and IP FIFOs (in packets/cells number) */
	vbdc_need_pkt = this->getMacBufferLength(cr_vbdc);
	UTI_DEBUG_L3("SF#%u: frame %u: MAC buffer length = %d, VBDC credit = %u\n",
	             this->current_superframe_sf, this->current_frame,
	             vbdc_need_pkt, this->vbdc_credit_pkt);

	/* compute VBDC request: actual Vbdc request to be sent */
	vbdc_request_pkt = std::max(0, (vbdc_need_pkt - this->vbdc_credit_pkt));
	UTI_DEBUG_L3("SF#%u: frame %u: theoretical VBDC request = %u packets/cells",
	             this->current_superframe_sf, this->current_frame,
	             vbdc_request_pkt);

	/* adjust request in function of max_vbdc value */
	vbdc_request_pkt = std::min(vbdc_request_pkt, this->max_vbdc_pkt);

	// Ensure VBDC request value is not greater than SAC field
	vbdc_request_pkt = std::min(vbdc_request_pkt, C_MAX_VBDC_IN_SAC);
	UTI_DEBUG_L3("updated VBDC request = %d packets/cells in fonction of "
	             "max VBDC and max VBDC in SAC\n", vbdc_request_pkt);

	/* update VBDC Credit here */
	/* NB: the computed VBDC is always really sent if not null */
	this->vbdc_credit_pkt += vbdc_request_pkt;
	UTI_DEBUG_L3("updated VBDC request = %d packets/cells in SAC, VBDC credit = %u\n",
	             vbdc_request_pkt, this->vbdc_credit_pkt);

	return vbdc_request_pkt;
}

vol_pkt_t DamaAgentRcsLegacy::getMacBufferLength(cr_type_t cr_type)
{
	vol_pkt_t nb_pkt_in_fifo; // absolute number of packets/cells in fifo

	nb_pkt_in_fifo = 0;
	for(std::map<unsigned int, DvbFifo *>::const_iterator it = this->dvb_fifos.begin();
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
	vol_pkt_t nb_pkt_input; // # packets/cells that filled the queue since last RBDC request

	nb_pkt_input = 0;
	for(std::map<unsigned int, DvbFifo *>::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_type)
		{
			nb_pkt_input += (*it).second->getNewSize();
		}
	}

	return nb_pkt_input;
}

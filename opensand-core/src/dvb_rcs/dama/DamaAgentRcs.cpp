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
 * @file    DamaAgentRcs.cpp
 * @brief   Implementation of the DAMA agent for DVB-RCS emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#define DBG_PACKAGE PKG_DAMA_DA
#include <opensand_conf/uti_debug.h>
#define DA_DBG_PREFIX "[RCS]"

#include "DamaAgentRcs.h"


DamaAgentRcs::DamaAgentRcs():
	DamaAgent(),
	allocated_pkt(0),
	dynamic_allocation_pkt(0),
	remaining_allocation_pktpf(0),
	cra_in_cr(false),
	rbdc_timer_sf(0)
{
}

DamaAgentRcs::~DamaAgentRcs()
{
	delete this->ret_schedule;

	if(this->rbdc_request_buffer != NULL)
	{
		delete this->rbdc_request_buffer;
	}

	delete this->converter;
}

bool DamaAgentRcs::init()
{
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
			return false;
		}
	}

	// Initializes unit converter
	this->converter = new UnitConverter(this->packet_handler->getFixedLength(),
	                                    this->frame_duration_ms);

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

	return true;
}


bool DamaAgentRcs::processOnFrameTick()
{
	// Call parent method
	if(!DamaAgent::processOnFrameTick())
	{
		UTI_ERROR("SF#%u: cannot call DamaAgent::processOnFrameTick()\n",
		          this->current_superframe_sf);
		return false;
	}

	this->current_frame++;
	this->remaining_allocation_pktpf = this->dynamic_allocation_pkt;

	return true;
}

bool DamaAgentRcs::returnSchedule(list<DvbFrame *> *complete_dvb_frames)
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

	// Update stats and probes
	this->probe_st_remaining_allocation->put(remaining_alloc_kbps);

	return true;
}

bool DamaAgentRcs::hereIsSOF(time_sf_t superframe_number_sf)
{
	// Call parent method
	if(!DamaAgent::hereIsSOF(superframe_number_sf))
	{
		UTI_ERROR("SF#%u: cannot call DamaAgent::hereIsSOF()\n",
		          this->current_superframe_sf);
		return false;
	}
	this->current_frame = 0;
	return true;
}

// a TTP reading function that handles MODCOD but not priority
bool DamaAgentRcs::hereIsTTP(Ttp *ttp)
{
	map<uint8_t, emu_tp_t> tp;

	if(this->group_id != ttp->getGroupId())
	{
		UTI_ERROR("SF#%u: TTP with different group_id (%d).\n",
		             this->current_superframe_sf, ttp->getGroupId());
		return true;
	}

	if(!ttp->getTp(this->tal_id, tp))
	{
		// Update stats and probes
		this->probe_st_total_allocation->put(0);
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
		             ttp->getSuperframeCount(),
		             (*it).first,
		             (*it).second.offset,
		             assign_pkt,
		             (*it).second.fmt_id,
		             (*it).second.priority);
	}

	// Update stats and probes
	this->probe_st_total_allocation->put(
		this->converter->pktpfToKbps(this->allocated_pkt));

	UTI_DEBUG("SF#%u: allocated TS=%u\n",
	          ttp->getSuperframeCount(), this->allocated_pkt);
	return true;
}

bool DamaAgentRcs::buildSAC(cr_type_t UNUSED(cr_type),
                            Sac *sac,
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
		this->probe_st_rbdc_req_size->put(0);
		this->probe_st_vbdc_req_size->put(0);
		goto end;
	}

	// set RBDC request (if any) in SAC
	if(send_rbdc_request)
	{
		sac->addRequest(0, cr_rbdc, rbdc_request_kbps);

		// update variables used for next RBDC CR computation
		this->rbdc_timer_sf = 0;
		this->rbdc_request_buffer->Update(rbdc_request_kbps);

		// reset counter of arrival packets in MAC FIFOs related to RBDC
		for(fifos_t::const_iterator it = this->dvb_fifos.begin();
		    it != this->dvb_fifos.end(); ++it)
		{
			(*it).second->resetNew(cr_rbdc);
		}

		// Update statistics
		this->probe_st_rbdc_req_size->put(rbdc_request_kbps);

	}
	else
	{
		this->probe_st_rbdc_req_size->put(0);
	}

	// set VBDC request (if any) in SAC
	if(send_vbdc_request)
	{
		sac->addRequest(0, cr_vbdc, vbdc_request_pkt);

		// Update statistics
		this->probe_st_vbdc_req_size->put(
			this->converter->pktToKbits(vbdc_request_pkt));

	}
	else
	{
		this->probe_st_vbdc_req_size->put(0);
	}

	UTI_DEBUG("SF#%u: build CR with %u kb/s in RBDC and %u packets in VBDC",
	          this->current_superframe_sf, rbdc_request_kbps, vbdc_request_pkt);

 end:
	return true;
}

vol_pkt_t DamaAgentRcs::getMacBufferLength(cr_type_t cr_type)
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


vol_pkt_t DamaAgentRcs::getMacBufferArrivals(cr_type_t cr_type)
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


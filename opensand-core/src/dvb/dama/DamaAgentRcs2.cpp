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
 * @file    DamaAgentRcs2.cpp
 * @brief   Implementation of the DAMA agent for DVB-RCS2 emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 * @author  Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "DamaAgentRcs2.h"

#include "UnitConverterFixedSymbolLength.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>

#include <algorithm>

// constants
const rate_kbps_t C_MAX_RBDC_IN_SAC = 16320.0; // 16320 kbits/s, limitation due
                                               // to CR value size in to SAC field
const vol_kb_t C_MAX_VBDC_IN_SAC = 4080;       // 4080 packets/ceils, limitation
                                               // due to CR value size in to SAC field


DamaAgentRcs2::DamaAgentRcs2(FmtDefinitionTable *ret_modcod_def):
	DamaAgent(),
	allocated_kb(0),
	dynamic_allocation_kb(0),
	remaining_allocation_b(0),
	rbdc_request_buffer(NULL),
	ret_schedule(NULL),
	rbdc_timer_sf(0),
	ret_modcod_def(ret_modcod_def),
	modcod_id(0)
{
}

DamaAgentRcs2::~DamaAgentRcs2()
{
	if(this->ret_schedule != NULL)
	{
		delete this->ret_schedule;
	}

	if(this->rbdc_request_buffer != NULL)
	{
		delete this->rbdc_request_buffer;
	}

	if(this->converter != NULL)
	{
		delete this->converter;
	}
}

bool DamaAgentRcs2::init()
{
	FmtDefinition *fmt_def;
	vol_sym_t length_sym = 0;

	if(this->rbdc_enabled)
	{
		// create circular buffer for saving last RBDC requests during the past
		// MSL duration with size = integer part of MSL / SYNC period
		// (in frame number)
		// NB: if size = 0, only last req is saved and sum is always 0
		this->rbdc_request_buffer =
			new CircularBuffer((size_t) this->msl_sf / this->sync_period_sf);
		if(this->rbdc_request_buffer == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Cannot create circular buffer to save "
			    "the last RBDC requests\n");
			return false;
		}
	}

	// Initializes unit converter
	if(!OpenSandModelConf::Get()->getRcs2BurstLength(length_sym))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get RCS2 burst length value");
		return NULL;
	}
	if(length_sym == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "invalid value '%u' value of RCS2 burst length", length_sym);
		return NULL;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "Burst length = %u sym\n", length_sym);
	
	this->converter = new UnitConverterFixedSymbolLength(this->frame_duration_ms, 
		0, length_sym);
	if(this->converter == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot create the unit converter\n");
		return false;
	}

	this->ret_schedule = new ReturnSchedulingRcs2(this->packet_handler, this->dvb_fifos);
	if(!this->ret_schedule)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot create the return link scheduling\n");
		return false;
	}

	this->modcod_id = this->ret_modcod_def->getMaxId();
	fmt_def = this->ret_modcod_def->getDefinition(this->modcod_id);
	if(fmt_def != NULL)
	{
		this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());
	}
	LOG(this->log_init, LEVEL_DEBUG,
	    "Default modcod id %u, modulation efficiency %u\n",
	    this->modcod_id, this->converter->getModulationEfficiency());

	this->probe_st_sent_modcod = Output::Get()->registerProbe<int>("Up_Return_modcod.Sent_modcod",
	                                                               "modcod index",
	                                                               true, SAMPLE_LAST);

	return true;
}

bool DamaAgentRcs2::processOnFrameTick()
{
	FmtDefinition *fmt_def;
	vol_b_t length_b;

	this->remaining_allocation_b = this->dynamic_allocation_kb * 1000;
	this->burst_length_b = this->converter->getPacketBitLength();

	fmt_def = this->ret_modcod_def->getDefinition(this->modcod_id);
	if(fmt_def == NULL)
	{
		LOG(this->log_schedule, LEVEL_WARNING,
		    "SF#%u: no MODCOD %u found",
		    this->current_superframe_sf,
		    this->modcod_id);
		return false;
	}

	length_b = this->burst_length_b;
	this->burst_length_b = fmt_def->removeFec(this->burst_length_b);
	LOG(this->log_schedule, LEVEL_DEBUG,
	    "SF#%u: burst length without FEC %u b, with FEC %u b",
	    this->current_superframe_sf,
	    this->burst_length_b,
	    length_b);
	return true;
}

bool DamaAgentRcs2::hereIsSOF(time_sf_t superframe_number_sf)
{
	// Call parent method
	if(!DamaAgent::hereIsSOF(superframe_number_sf))
	 {
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u: cannot call DamaAgent::hereIsSOF()\n",
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

// a TTP reading function that handles MODCOD but not priority and frame id
// only one TP is supported for MODCOD handling
bool DamaAgentRcs2::hereIsTTP(Ttp *ttp)
{
	rate_kbps_t alloc_kbps;
	fmt_id_t prev_modcod_id;
  std::map<uint8_t, emu_tp_t> tp;

	this->allocated_kb = 0;
	if(this->group_id != ttp->getGroupId())
	{
		LOG(this->log_ttp, LEVEL_ERROR,
		    "SF#%u: TTP with different group_id (%d).\n",
		    this->current_superframe_sf, ttp->getGroupId());
		return true;
	}

	if(!ttp->getTp(this->tal_id, tp))
	{
		// Update stats and probes
		this->probe_st_total_allocation->put(0);
		return true;
	}
	if(tp.size() > 1)
	{
		LOG(this->log_ttp, LEVEL_WARNING,
		    "Received more than one TP in TTP, "
		    "allocation will be correctly handled but not "
		    "modcod for physical layer emulation\n");
	}

	prev_modcod_id = this->modcod_id;
	this->allocated_kb = 0;
	for(std::map<uint8_t, emu_tp_t>::iterator it = tp.begin();
	    it != tp.end(); ++it)
	{
		vol_kb_t assign_kb;
		FmtDefinition *fmt_def;

		LOG(this->log_ttp, LEVEL_DEBUG,
		    "SF#%u: frame#%u: offset:%u, assignment_count:%u kb, "
		    "fmt_id:%u priority:%u\n", ttp->getSuperframeCount(),
		    (*it).first, (*it).second.offset, (*it).second.assignment_count,
		    (*it).second.fmt_id, (*it).second.priority);

		// we can directly assign here because we should have
		// received only one TTP
		this->modcod_id = (*it).second.fmt_id;
		if(prev_modcod_id != this->modcod_id)
		{
			// update the packet length in function of MODCOD
			LOG(this->log_ttp, LEVEL_DEBUG,
			    "SF#%u: modcod changed to %u\n",
			    ttp->getSuperframeCount(), this->modcod_id);
		}
		
		assign_kb = (*it).second.assignment_count;
		fmt_def = this->ret_modcod_def->getDefinition(this->modcod_id);
		if(fmt_def == NULL)
		{
			this->converter->setModulationEfficiency(0);
			continue;
		}
		this->converter->setModulationEfficiency(fmt_def->getModulationEfficiency());

		this->allocated_kb += assign_kb;
	}

	// Update stats and probes
	alloc_kbps = this->converter->pfToPs(this->allocated_kb);
	this->probe_st_total_allocation->put(alloc_kbps);

	LOG(this->log_ttp, LEVEL_INFO,
	    "SF#%u: allocated = %u kbits/s\n",
	    ttp->getSuperframeCount(), alloc_kbps);
	return true;
}

bool DamaAgentRcs2::returnSchedule(std::list<DvbFrame *> *complete_dvb_frames)
{
	uint32_t remaining_alloc_b = this->remaining_allocation_b;
	rate_kbps_t remaining_alloc_kbps;

	LOG(this->log_schedule, LEVEL_DEBUG,
	    "SF#%u: modulation efficiency %u, "
	    "burst length %u sym (%u b)\n",
	    this->current_superframe_sf,
	    this->converter->getModulationEfficiency(),
	    this->converter->getPacketSymbolLength(),
	    this->burst_length_b);

	this->ret_schedule->setMaxBurstLength(this->burst_length_b);
	
	remaining_alloc_kbps = this->converter->pfToPs(this->remaining_allocation_b / 1000);
	LOG(this->log_schedule, LEVEL_DEBUG,
	    "SF#%u: allocation before scheduling %u kbit/s\n",
	    this->current_superframe_sf,
	    remaining_alloc_kbps);

	LOG(this->log_schedule, LEVEL_DEBUG,
	    "SF#%u: capacity to send %u bursts of payload length %u bytes (%u bit)\n",
	    this->current_superframe_sf,
	    this->burst_length_b > 0 ? remaining_alloc_b / this->burst_length_b : 0,
	    this->burst_length_b >> 3,
	    this->burst_length_b);
	
	if(!this->ret_schedule->schedule(this->current_superframe_sf,
	                                 0,
	                                 complete_dvb_frames,
	                                 remaining_alloc_b))
	{
		LOG(this->log_schedule, LEVEL_ERROR,
		    "SF#%u: Uplink Scheduling failed",
		    this->current_superframe_sf);
		return false;
	}
	// add modcod id in frames
	for (auto&& dvb_frame: *complete_dvb_frames)
	{
		if(dvb_frame->getMessageType() == EmulatedMessageType::DvbBurst)
		{
			DvbRcsFrame *frame = *dvb_frame;
			frame->setModcodId(this->modcod_id);
		}
	}
	this->probe_st_sent_modcod->put(0);
	if(complete_dvb_frames->size() > 0)
	{
		// only set MODCOD id if there is data sent
		// as we are with SAMPLE_LAST we may miss some of these
		// when not sending a lot of trafic
		this->probe_st_sent_modcod->put(this->modcod_id);
	}

	this->remaining_allocation_b = remaining_alloc_b;
	remaining_alloc_kbps = this->converter->pfToPs(this->remaining_allocation_b / 1000);

	LOG(this->log_schedule, LEVEL_DEBUG,
	    "SF#%u: remaining allocation after scheduling "
	    "%u kbits/s\n", this->current_superframe_sf,
	    remaining_alloc_kbps);

	// Update stats and probes
	this->probe_st_remaining_allocation->put(remaining_alloc_kbps);

	return true;
}

bool DamaAgentRcs2::buildSAC(ReturnAccessType,
                            Sac *sac,
                            bool &empty)
{
	bool send_rbdc_request = false;
	bool send_vbdc_request = false;
	rate_kbps_t rbdc_request_kbps = 0;
	vol_kb_t vbdc_request_kb = 0;
	empty = false;

	// Compute RBDC request if needed
	if(this->rbdc_enabled)
	{
		rbdc_request_kbps = this->computeRbdcRequest();
		LOG(this->log_sac, LEVEL_INFO,
		    "SF#%u: Computed RBDC request = %u kb/s\n",
		    this->current_superframe_sf,
		    rbdc_request_kbps);

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
		vbdc_request_kb = this->computeVbdcRequest();
		LOG(this->log_sac, LEVEL_INFO,
		    "SF#%u: Computed VBDC request = %u kb\n",
		    this->current_superframe_sf,
		    vbdc_request_kb);

		// Send the request only if it is not null
		if(vbdc_request_kb > 0)
		{
			send_vbdc_request = true;
		}
	}

	// if no valid CR is built: skipping it
	if(!send_rbdc_request && !send_vbdc_request)
	{
		LOG(this->log_sac, LEVEL_DEBUG,
		    "SF#%u: RBDC CR = %d, VBDC CR = %d, no CR built.\n",
		    this->current_superframe_sf, rbdc_request_kbps,
		    vbdc_request_kb);
		empty = true;
		this->probe_st_rbdc_req_size->put(0);
		this->probe_st_vbdc_req_size->put(0);
		goto end;
	}

	// set RBDC request (if any) in SAC
	if(send_rbdc_request)
	{
		sac->addRequest(0, ReturnAccessType::dama_rbdc, rbdc_request_kbps);

		// update variables used for next RBDC CR computation
		this->rbdc_timer_sf = 0;
		this->rbdc_request_buffer->Update(rbdc_request_kbps);

		// reset counter of arrival packets in MAC FIFOs related to RBDC
		for(fifos_t::const_iterator it = this->dvb_fifos.begin();
		    it != this->dvb_fifos.end(); ++it)
		{
			(*it).second->resetNew(ReturnAccessType::dama_rbdc);
		}

		// Update statistics
		this->probe_st_rbdc_req_size->put(rbdc_request_kbps);

	}
	else
	{
		this->probe_st_rbdc_req_size->put(0);
		this->rbdc_request_buffer->Update(0);
	}

	// set VBDC request (if any) in SAC
	if(send_vbdc_request)
	{
		sac->addRequest(0, ReturnAccessType::dama_vbdc, vbdc_request_kb);

		// Update statistics
		this->probe_st_vbdc_req_size->put(vbdc_request_kb);

	}
	else
	{
		this->probe_st_vbdc_req_size->put(0);
	}

	LOG(this->log_sac, LEVEL_INFO,
	    "SF#%u: build CR with %u kb/s in RBDC and %u kb in "
	    "VBDC", this->current_superframe_sf, rbdc_request_kbps,
	    vbdc_request_kb);

 end:
	return true;
}

rate_kbps_t DamaAgentRcs2::checkRbdcRequest(rate_kbps_t request_kbps)
{
	return std::min(request_kbps, C_MAX_RBDC_IN_SAC);
}

vol_kb_t DamaAgentRcs2::checkVbdcRequest(vol_kb_t request_kb)
{
	return std::min(request_kb, C_MAX_VBDC_IN_SAC);
}

vol_b_t DamaAgentRcs2::getMacBufferLength(ReturnAccessType cr_type)
{
	vol_b_t nb_b_in_fifo; // absolute data length in fifo

	nb_b_in_fifo = 0;
	for(auto&& it: this->dvb_fifos)
	{
		if(it.second->getAccessType() == cr_type)
		{
			vol_bytes_t length = it.second->getCurrentDataLength();
			nb_b_in_fifo += (length << 3);
		}
	}

	return nb_b_in_fifo;
}


vol_b_t DamaAgentRcs2::getMacBufferArrivals(ReturnAccessType cr_type)
{
	vol_b_t nb_b_input; // data that filled the queue since last RBDC request

	nb_b_input = 0;
	for(auto&& it: this->dvb_fifos)
	{
		if(it.second->getAccessType() == cr_type)
		{
			vol_bytes_t length = it.second->getNewDataLength();
			nb_b_input += (length << 3);
		}
	}

	return nb_b_input;
}


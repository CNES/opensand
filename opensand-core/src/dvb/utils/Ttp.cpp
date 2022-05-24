/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file    Ttp.h
 * @brief   Generic TTP (Timeslot Time Plan)
 * @author  Julien Bernard / Viveris Technologies
 * @author  Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "Ttp.h"

#include "OpenSandFrames.h"

#include <opensand_output/Output.h>

#include <cstring>
#include <arpa/inet.h>



/// The maximum number of frames
#define NBR_MAX_FRAMES 1
/// The maximum number of TP per frame
#define NBR_MAX_TP BROADCAST_TAL_ID

std::shared_ptr<OutputLog> Ttp::ttp_log = nullptr;

Ttp::Ttp():
	DvbFrameTpl<T_DVB_TTP>()
{
};

Ttp::Ttp(group_id_t group_id, time_sf_t sf_id):
	DvbFrameTpl<T_DVB_TTP>()
{
	this->setMessageType(MSG_TYPE_TTP);
	this->setMessageLength(sizeof(T_DVB_TTP));
	this->setMaxSize(sizeof(T_DVB_TTP) +
	                 NBR_MAX_FRAMES * (sizeof(emu_frame_t) +
	                                   NBR_MAX_TP * sizeof(emu_tp_t))); 
	this->frame()->ttp.ttp_info.group_id = group_id;
	this->frame()->ttp.ttp_info.superframe_count = htons(sf_id);
}


bool Ttp::addTimePlan(time_frame_t frame_id,
                      tal_id_t tal_id,
                      int32_t offset,
                      uint16_t assignment_count,
                      fmt_id_t fmt_id,
                      uint8_t priority)
{
	emu_tp_t tp;

	tp.tal_id = htons(tal_id);
	tp.offset = htonl(offset);
	tp.assignment_count = htons(assignment_count);
	tp.fmt_id = fmt_id;
	tp.priority = priority;

	// create the entry for this frame id if it does not exist
	if(this->frames.find(frame_id) == this->frames.end())
	{
		time_plans_t time_plans;
		time_plans.push_back(tp);
		this->frames.insert(std::make_pair<uint8_t, time_plans_t>((uint8_t)frame_id, (time_plans_t)time_plans));
	}
	// add the TP to the list of TP for this frame ID
	else
	{
		this->frames[frame_id].push_back(tp);
	}
/* not compatible with simulated STs => remove if this is ok
	if(this->frames[frame_id].size() > BROADCAST_TAL_ID)
	{
		LOG(ttp_log, LEVEL_ERROR,
		    "Too many time plans for frame id %u\n", frame_id);
		this->frames[frame_id].pop_back();
		return false;
	}*/
	LOG(ttp_log, LEVEL_DEBUG,
	    "Add TP for ST%u at frame %u with offset=%u, "
	    "assignment_count=%u, fmt=%u, priority=%u\n",
	    tal_id, frame_id, offset, assignment_count,
	    fmt_id, priority);
	return true;
}

bool Ttp::build(void)
{
	frames_t::iterator frame_it;
	unsigned int frame_count = 0;
	time_plans_t::iterator tp_it;
	unsigned int tp_count;
	size_t ttp_length = 0;

	ttp_length = sizeof(T_DVB_TTP);
	// get the beginning of the frame
	for(frame_it = this->frames.begin(); frame_it != this->frames.end();
	    ++frame_it)
	{
    std::vector<emu_tp_t> tp_list = (*frame_it).second;
		emu_frame_t emu_frame;

		emu_frame.frame_info.frame_number = (*frame_it).first;
		ttp_length += sizeof(frame_info_t);
		tp_count = 0;
		this->data.append((unsigned char *)&emu_frame, sizeof(emu_frame_t));
		// get the first TP
		for(tp_it = tp_list.begin(); tp_it != tp_list.end(); ++tp_it)
		{
			this->data.append((unsigned char *)&(*tp_it), sizeof(emu_tp_t));
			ttp_length += sizeof(emu_tp_t);
			tp_count++;
		}
		this->frame()->ttp.frames[frame_count].frame_info.tp_loop_count = tp_count;
		frame_count++;
	}
	this->frame()->ttp.ttp_info.frame_loop_count = frame_count;
	// update message length
	// TODO we may use getPayloadLength, this should be the same value
	this->setMessageLength(ttp_length);

	return true;
}


bool Ttp::getTp(tal_id_t tal_id, std::map<uint8_t, emu_tp_t> &tps)
{
	size_t length = this->getMessageLength();
	emu_ttp_t *ttp;

	// we need this unsigned char * for arithmetical operations
	// on pointers as frame size is not constant
	unsigned char *frame_start;

	/* check that data contains DVB header, superframe_count and
	 * frame_loop_count */
	if(length < sizeof(T_DVB_TTP))
	{
		LOG(ttp_log, LEVEL_ERROR,
		    "Length is to small for a TTP\n");
		return false;
	}
	length -= sizeof(T_DVB_HDR);

	ttp = &(this->frame()->ttp);
	LOG(ttp_log, LEVEL_DEBUG,
	    "SF#%u: ttp->frame_loop_count=%u\n",
	    this->getSuperframeCount(),
	    ttp->ttp_info.frame_loop_count);

	length -= sizeof(ttp_info_t);
	frame_start = (unsigned char *)(&ttp->frames);
	for(unsigned int i = 0; i < ttp->ttp_info.frame_loop_count; i++)
	{
		emu_tp_t *tp;
		emu_frame_t *emu_frame = (emu_frame_t *)frame_start;

		if(length < sizeof(frame_info_t) + emu_frame->frame_info.tp_loop_count * sizeof(emu_tp_t))
		{
			LOG(ttp_log, LEVEL_ERROR,
			    "Length is too small for the given tp number\n");
			return false;
		}
		// update length
		length -= sizeof(frame_info_t);
		LOG(ttp_log, LEVEL_DEBUG,
		    "SF#%u: frame #%u tbtp_loop_count=%u\n",
		    this->getSuperframeCount(), i,
		    emu_frame->frame_info.tp_loop_count);
		// get the first TP
		// increase from 1 * sizeof(tp)
		tp = (emu_tp_t *)(&emu_frame->tp);
		for(unsigned int j = 0; j < emu_frame->frame_info.tp_loop_count; j++)
		{
			length -= sizeof(emu_tp_t);
			if(ntohs(tp->tal_id) != tal_id)
			{
				LOG(ttp_log, LEVEL_DEBUG,
				    "SF#%u: TP for ST%u ignored\n",
				    this->getSuperframeCount(),
				    ntohs(tp->tal_id));
				tp = tp + 1;
				continue;
			}
			tp->offset = ntohl(tp->offset);
			tp->assignment_count = ntohs(tp->assignment_count);
			tps[emu_frame->frame_info.frame_number] = *tp;
			LOG(ttp_log, LEVEL_DEBUG,
			    "SF#%u: frame#%u tbtp#%u: tal_id:%u, "
			    "offset:%u, assignment_count:%u, "
			    "fmt_id:%u priority:%u\n",
			    this->getSuperframeCount(), i, j,
			    tal_id, tp->offset, tp->assignment_count,
			    tp->fmt_id, tp->priority);
			// increase from 1 * sizeof(tp), we do not need to
			// use an unsigned char * for arithmetic operation here
			tp = tp + 1;
		}
		// go to next frame
		frame_start = frame_start + emu_frame->frame_info.tp_loop_count * sizeof(emu_tp_t);
	}

	return true;
}


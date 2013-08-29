/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 */

#include <opensand_conf/uti_debug.h>

#include "Ttp.h"

#include "OpenSandFrames.h"

#include <cstring>
#include <arpa/inet.h>


Ttp::Ttp(group_id_t group_id):
	group_id(group_id)
{}

bool Ttp::parse(const unsigned char *data, size_t length)
{
	emu_ttp_t *ttp;

	// we need this unsigned char * for arithmetical operations
	// on pointers as frame size is not constant
	unsigned char *frame_start;

	// clean TPs
	this->reset();
	/* check that data contains DVB header, superframe_count and
	 * frame_loop_count */
	if(length < sizeof(T_DVB_HDR) + sizeof(ttp_info_t))
	{
		UTI_ERROR("Length is to small for a TTP\n");
		return false;
	}
	length -= sizeof(T_DVB_HDR);

	ttp = &((T_DVB_TTP *)data)->ttp;
	this->group_id = ntohs(ttp->ttp_info.group_id);
	this->superframe_count = ntohs(ttp->ttp_info.superframe_count);
	UTI_DEBUG_L3("SF#%u: ttp->frame_loop_count=%u\n",
	             this->superframe_count, ttp->ttp_info.frame_loop_count);

	length -= sizeof(ttp_info_t);
	// fill the frames manually because the structure is allocated for
	// the maximum possible size
	frame_start = (unsigned char *)(&ttp->frames);
	for(unsigned int i = 0; i < ttp->ttp_info.frame_loop_count; i++)
	{
		emu_tp_t *tp;
		emu_frame_t *frame = (emu_frame_t *)frame_start;

		if(length < sizeof(frame_info_t) + frame->frame_info.tp_loop_count * sizeof(emu_tp_t))
		{
			UTI_ERROR("Length is too small for the given tp number\n");
			return false;
		}
		// update length
		length -= sizeof(frame_info_t);
		UTI_DEBUG_L3("SF#%u: frame #%u btp_loop_count=%u\n",
		             this->superframe_count, i, frame->frame_info.tp_loop_count);
		// get the first TP
		// increase from 1 * sizeof(tp)
		tp = (emu_tp_t *)(&frame->tp);
		for(unsigned int j = 0; j < frame->frame_info.tp_loop_count; j++)
		{
			length -= sizeof(emu_tp_t);
			tal_id_t tal_id = ntohs(tp->tal_id);
			tp->offset = ntohl(tp->offset);
			tp->assignment_count = ntohs(tp->assignment_count);
			// create the entry for this terminal ID if it does not exist
			if(this->tps.find(tal_id) == this->tps.end())
			{
				std::map<uint8_t, emu_tp_t> time_plans;
				time_plans[frame->frame_info.frame_number] = *tp;
				this->tps[tal_id] = time_plans;
			}
			// add the TP for this terminal at the position corresponding to
			// the frame ID
			else
			{
				this->tps[tal_id][frame->frame_info.frame_number] = *tp;
			}
			UTI_DEBUG_L3("SF#%u: frame#%u btp#%u: tal_id:%u, "
			             "offset:%u, assignment_count:%u, "
			             "fmt_id:%u priority:%u\n",
			             this->superframe_count, i, j,
			             tal_id,
			             tp->offset,
			             tp->assignment_count,
			             tp->fmt_id,
			             tp->priority);
			// increase from 1 * sizeof(tp), we do not need to
			// use an unsigned char * for arithmetic operation here
			tp = tp + 1;
		}
		// go to next frame
		frame_start = frame_start + frame->frame_info.tp_loop_count * sizeof(emu_tp_t);
	}

	return true;
}

bool Ttp::addTimePlan(time_frame_t frame_id,
                      tal_id_t tal_id,
                      int32_t offset,
                      uint16_t assignment_count,
                      uint8_t fmt_id,
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
		this->frames.insert(std::make_pair<uint8_t, time_plans_t>(frame_id, time_plans));
	}
	// add the TP to the list of TP for this frame ID
	else
	{
		this->frames[frame_id].push_back(tp);
	}
	if(this->frames[frame_id].size() > BROADCAST_TAL_ID)
	{
		UTI_ERROR("Too many time plans for frame id %u\n", frame_id);
		this->frames[frame_id].pop_back();
		return false;
	}
	UTI_DEBUG_L3("Add TP for ST%u at frame %u with offset=%u, "
	             "assignment_count=%u, FMT=%u, priority=%u\n",
	             tal_id, frame_id, offset, assignment_count,
	             fmt_id, priority);
	return true;
}

void Ttp::reset()
{
	this->frames.clear();
	this->tps.clear();
}

bool Ttp::build(time_sf_t superframe_nbr_sf, unsigned char *frame, size_t &length)
{
	T_DVB_TTP *dvb_ttp = (T_DVB_TTP *)frame;
	frames_t::iterator frame_it;
	unsigned int frame_count = 0;
	time_plans_t::iterator tp_it;
	unsigned int tp_count;
	size_t ttp_length = 0;
	// we need this unsigned char * for arithmetical operations
	// on pointers as frame size is not constant
	unsigned char *frame_start;

	dvb_ttp->hdr.msg_type = MSG_TYPE_TTP;

	dvb_ttp->ttp.ttp_info.group_id = htons(this->group_id);
	dvb_ttp->ttp.ttp_info.superframe_count = htons(superframe_nbr_sf);
	// we need the position for the frames beginning
	ttp_length += sizeof(ttp_info_t);
	// get the beginning of the frame
	frame_start = (unsigned char *)(&dvb_ttp->ttp.frames);
	for(frame_it = this->frames.begin(); frame_it != this->frames.end();
	    ++frame_it)
	{
		vector<emu_tp_t> tp_list = (*frame_it).second;
		emu_tp_t *tp;
		emu_frame_t *emu_frame = (emu_frame_t *)frame_start;

		frame_count++;
		emu_frame->frame_info.frame_number = (*frame_it).first;
		ttp_length += sizeof(frame_info_t);
		tp_count = 0;
		// get the first TP
		tp = (emu_tp_t *)(&emu_frame->tp);
		for(tp_it = tp_list.begin(); tp_it != tp_list.end(); ++tp_it)
		{
			emu_tp_t tp_orig = *tp_it;

			memcpy(tp, &tp_orig, sizeof(emu_tp_t));
			ttp_length += sizeof(emu_tp_t);
			tp_count++;
			// go to next TP
			// increase from 1 * sizeof(tp), we do not need to
			// use an unsigned char * for arithmetic operation here
			tp = tp + 1;
		}
		emu_frame->frame_info.tp_loop_count = tp_count;
		// go to next frame
		frame_start = frame_start + sizeof(emu_tp_t) * tp_count;
	}
	dvb_ttp->ttp.ttp_info.frame_loop_count = frame_count;
	// clean the frames list
	this->reset();
	// update message length
	// TODO when this will be handled in reception HTONL !!!!
	dvb_ttp->hdr.msg_length = sizeof(T_DVB_HDR) + ttp_length;
	length = sizeof(T_DVB_HDR) + ttp_length;

	return true;
}

bool Ttp::getTp(tal_id_t tal_id, std::map<uint8_t, emu_tp_t> &tp)
{
	if(this->tps.find(tal_id) == this->tps.end())
	{
		UTI_INFO("No TP for ST%u\n", tal_id);
		return false;
	}
	tp = this->tps[tal_id];

	return true;
}


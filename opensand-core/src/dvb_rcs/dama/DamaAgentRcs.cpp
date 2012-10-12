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
 */

#include "DamaAgentRcs.h"

#define DBG_PACKAGE PKG_DAMA_DA
#include "opensand_conf/uti_debug.h"
#define DA_DBG_PREFIX "[RCS]"

DamaAgentRcs::DamaAgentRcs(EncapPlugin::EncapPacketHandler *pkt_hdl):
	DamaAgent(pkt_hdl),
	allocated_pkt(0),
	dynamic_allocation_pkt(0),
	remaining_allocation_pktpsf(0)
{
}

bool DamaAgentRcs::hereIsTTP(unsigned char *buf, size_t len)
{
	T_DVB_TBTP *tbtp = (T_DVB_TBTP *) buf;
	T_DVB_FRAME *frame;
	T_DVB_BTP *btp;
	int i, j;

	if(tbtp->hdr.msg_type != MSG_TYPE_TBTP)
	{
		UTI_ERROR("SF#%u: Non TBTP msg type (%ld)\n",
		          this->current_superframe_sf, tbtp->hdr.msg_type);
		goto error;
	}

	if(this->group_id != tbtp->group_id)
	{
		UTI_DEBUG_L3("SF#%u: TBTP with different group_id (%d).\n",
		             this->current_superframe_sf, tbtp->group_id);
		goto end;
	}

	UTI_DEBUG_L3("SF#%u: tbtp->frame_loop_count (%d).\n",
	             this->current_superframe_sf, tbtp->frame_loop_count);

	frame = first_frame_ptr(tbtp);
	for(i = 0; i < tbtp->frame_loop_count; i++)
	{
		UTI_DEBUG_L3("SF#%u: frame#%d.\n", this->current_superframe_sf, i);
		btp = first_btp_ptr(frame);
		for(j = 0; j < frame->btp_loop_count; j++)
		{
			UTI_DEBUG_L3("SF#%u: btp#%d.\n", this->current_superframe_sf, j);
			if(this->tal_id == btp->logon_id)
			{
				// TODO check: assignment_count is in pktpsf but allocated_pkt
				// will be reinitialized each superframe so it can be a expressed
				// in pkt
				this->allocated_pkt += btp->assignment_count;
				UTI_DEBUG_L3("SF#%u:  assign=%ld\n",
				             this->current_superframe_sf,
				             btp->assignment_count);
			}
			else
			{
				UTI_DEBUG_L3("SF#%u: count:%ld, type:%d,"
				             "channelid:%d, logonid:%d,"
				             "mchannelflag:%d, startslot:%d.\n",
				             this->current_superframe_sf,
				             btp->assignment_count,
				             btp->assignment_type,
				             btp->channel_id,
				             btp->logon_id,
				             btp->multiple_channel_flag, btp->start_slot);
				UTI_DEBUG("SF#%u:\tBTP is not for this st (btp->logon_id=%d\n)",
				          this->current_superframe_sf, btp->logon_id);
			}
			btp = next_btp_ptr(btp);
		}
		frame = (T_DVB_FRAME *) btp; // Equiv to "frame=next_frame_ptr(frame)"
	}

	UTI_DEBUG("SF#%u: allocated TS=%u\n",
	          this->current_superframe_sf, this->allocated_pkt);
end:
	return true;

error:
	return false;
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
	this->remaining_allocation_pktpsf = this->dynamic_allocation_pkt;

	return true;
}

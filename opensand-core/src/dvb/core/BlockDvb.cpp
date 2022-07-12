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
 * @file BlockDvb.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "BlockDvb.h"
#include "BBFrame.h"
#include "Sac.h"
#include "Ttp.h"

#include "Plugin.h"
#include "DvbS2Std.h"
#include "EncapPlugin.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>


BlockDvb::BlockDvb(const std::string& name):
	Block(name)
{
	auto output = Output::Get();
	// register static logs
	BBFrame::bbframe_log = output->registerLog(LEVEL_WARNING, "Dvb.Net.BBFrame");
	Sac::sac_log = output->registerLog(LEVEL_WARNING, "Dvb.SAC");
	Ttp::ttp_log = output->registerLog(LEVEL_WARNING, "Dvb.TTP");
}


BlockDvb::~BlockDvb()
{
}


//****************************************************//
//                   DVB  UPWARD                      // 
//****************************************************//
BlockDvb::DvbUpward::DvbUpward(const std::string& name, dvb_specific specific):
	DvbChannel{},
	RtUpward{name},
	disable_control_plane{specific.disable_control_plane},
	disable_acm_loop{specific.disable_acm_loop}
{
}


BlockDvb::DvbUpward::~DvbUpward()
{
}


bool BlockDvb::DvbUpward::shareFrame(DvbFrame *frame)
{
	if (this->disable_control_plane)
	{
		if(!this->enqueueMessage((void **)&frame, sizeof(*frame), to_underlying(InternalMessageType::sig)))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Unable to transmit frame to upper layer\n");
			delete frame;
			return false;
		}
	}
	else
	{
		if(!this->shareMessage((void **)&frame, sizeof(*frame), to_underlying(InternalMessageType::sig)))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Unable to transmit frame to opposite channel\n");
			delete frame;
			return false;
		}
	}
	return true;
}


//****************************************************//
//                   DVB  DOWNWARD                    // 
//****************************************************//
BlockDvb::DvbDownward::DvbDownward(const std::string &name, dvb_specific specific):
	DvbChannel(),
	RtDownward(name),
	disable_control_plane{specific.disable_control_plane}
{
}


BlockDvb::DvbDownward::~DvbDownward()
{
}


bool BlockDvb::DvbDownward::initDown(void)
{
	// forward timer
	if(!OpenSandModelConf::Get()->getForwardFrameDuration(this->fwd_down_frame_duration_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'links': missing parameter 'forward frame duration'\n");
		return false;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "forward timer set to %u\n",
	    this->fwd_down_frame_duration_ms);

	return true;
}


bool BlockDvb::DvbDownward::sendBursts(std::list<DvbFrame *> *complete_frames,
                                       uint8_t carrier_id)
{
	bool status = true;

	// send all complete DVB-RCS frames
	LOG(this->log_send, LEVEL_DEBUG,
	    "send all %zu complete DVB frames...\n",
	    complete_frames->size());
	for(auto&& frame: *complete_frames)
	{
		// Send DVB frames to lower layer
		if(!this->sendDvbFrame(frame, carrier_id))
		{
			status = false;
			if(frame)
			{
				delete frame;
			}
			continue;
		}

		// DVB frame is now sent, so delete its content
		LOG(this->log_send, LEVEL_INFO,
		    "complete DVB frame sent to carrier %u\n", carrier_id);
	}
	// clear complete DVB frames
	complete_frames->clear();

	return status;
}


bool BlockDvb::DvbDownward::sendDvbFrame(DvbFrame *dvb_frame,
                                         uint8_t carrier_id)
{
	if(!dvb_frame)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "frame is %p\n", dvb_frame);
		return false;
	}

	dvb_frame->setCarrierId(carrier_id);

	if(dvb_frame->getTotalLength() <= 0)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "empty frame, header and payload are not present\n");
		return false;
	}

	// send the message to the lower layer
	// do not count carrier_id in len, this is the dvb_meta->hdr length
	if(!this->enqueueMessage((void **)&dvb_frame, 0, to_underlying(InternalMessageType::unknown)))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to send DVB frame to lower layer\n");
		delete dvb_frame;
		return false;
	}
	// TODO make a log_send_frame and a log_send_sig
	LOG(this->log_send, LEVEL_INFO,
	    "DVB frame sent to the lower layer\n");

	return true;
}


bool BlockDvb::DvbDownward::onRcvEncapPacket(std::unique_ptr<NetPacket> packet,
                                             DvbFifo *fifo,
                                             time_ms_t fifo_delay)
{
	return this->pushInFifo(fifo, std::move(packet), fifo_delay);
}

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

#include "Plugin.h"
#include "DvbS2Std.h"
#include "EncapPlugin.h"


BlockDvb::~BlockDvb()
{
}


//****************************************************//
//                   DVB  UPWARD                      // 
//****************************************************//
BlockDvb::DvbUpward::~DvbUpward()
{
}


//****************************************************//
//                   DVB  DOWNWARD                    // 
//****************************************************//
bool BlockDvb::DvbDownward::initDown(void)
{
	// forward timer
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               FWD_DOWN_CARRIER_DURATION,
	                   this->fwd_down_frame_duration_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, FWD_DOWN_CARRIER_DURATION);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "forward timer set to %u\n",
	    this->fwd_down_frame_duration_ms);

	return true;
error:
	return false;
}


bool BlockDvb::DvbDownward::sendBursts(list<DvbFrame *> *complete_frames,
                                       uint8_t carrier_id)
{
	list<DvbFrame *>::iterator frame_it;
	bool status = true;

	// send all complete DVB-RCS frames
	LOG(this->log_send, LEVEL_DEBUG,
	    "send all %zu complete DVB frames...\n",
	    complete_frames->size());
	for(frame_it = complete_frames->begin();
	    frame_it != complete_frames->end();
	    ++frame_it)
	{
		// Send DVB frames to lower layer
		if(!this->sendDvbFrame(*frame_it, carrier_id))
		{
			status = false;
			if(*frame_it)
			{
				delete *frame_it;
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
		goto error;
	}

	dvb_frame->setCarrierId(carrier_id);

	if(dvb_frame->getTotalLength() <= 0)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "empty frame, header and payload are not present\n");
		goto error;
	}

	// send the message to the lower layer
	// do not count carrier_id in len, this is the dvb_meta->hdr length
	if(!this->enqueueMessage((void **)(&dvb_frame)))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to send DVB frame to lower layer\n");
		goto release_dvb_frame;
	}
	// TODO make a log_send_frame and a log_send_sig
	LOG(this->log_send, LEVEL_INFO,
	    "DVB frame sent to the lower layer\n");

	return true;

release_dvb_frame:
	delete dvb_frame;
error:
	return false;
}


bool BlockDvb::DvbDownward::onRcvEncapPacket(NetPacket *packet,
                                             DvbFifo *fifo,
                                             time_ms_t fifo_delay)
{
	return this->pushInFifo(fifo, packet, fifo_delay);
}



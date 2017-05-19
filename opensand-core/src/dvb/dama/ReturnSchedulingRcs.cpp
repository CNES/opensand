/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file     ReturnScheduling.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS return link
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 */


#include "ReturnSchedulingRcs.h"
#include "MacFifoElement.h"
#include "OpenSandFrames.h"

#include <opensand_output/Output.h>

ReturnSchedulingRcs::ReturnSchedulingRcs(
			const EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos):
	ReturnSchedulingRcsCommon(packet_handler, fifos)
{
}

bool ReturnSchedulingRcs::macSchedule(const time_sf_t current_superframe_sf,
                                      list<DvbFrame *> *complete_dvb_frames,
                                      rate_pktpf_t &remaining_allocation_pktpf)
{
	unsigned int complete_frames_count;
	DvbRcsFrame *incomplete_dvb_frame = NULL;
	bool ret = true;
	rate_pktpf_t init_alloc_pktpf = remaining_allocation_pktpf;
	fifos_t::const_iterator fifo_it;

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: attempt to extract encap packets from MAC"
	    " FIFOs (remaining allocation = %d packets)\n",
	    current_superframe_sf,
	    remaining_allocation_pktpf);

	// create an incomplete DVB-RCS frame
	if(!this->allocateDvbRcsFrame(&incomplete_dvb_frame))
	{
		goto error;
	}

	// extract encap packets from MAC FIFOs while some UL capacity is available
	// (MAC fifos priorities are in MAC IDs order)
	// fifo are classified by priority value (map are ordered)
	complete_frames_count = 0;
	fifo_it = this->dvb_fifos.begin();
	while(fifo_it != this->dvb_fifos.end() &&
	      remaining_allocation_pktpf > 0)
	{
		NetPacket *encap_packet;
		MacFifoElement *elem;
		DvbFifo *fifo = (*fifo_it).second;

		if(fifo->getCurrentSize() <= 0)
		{
			// no data
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: ignore MAC FIFO %s: "
			    "no data (left) to schedule\n",
			    current_superframe_sf,
			    fifo->getName().c_str());
			// pass to next fifo
			++fifo_it;
		}
		else if(fifo->getAccessType() == access_saloha)
		{
			// not the good fifo
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: ignore MAC FIFO %s  "
			    "not the right access type (%d)\n",
			    current_superframe_sf,
			    fifo->getName().c_str(),
			    fifo->getAccessType());
			// pass to next fifo
			++fifo_it;
		}
		else
		{
			// FIFO with awaiting data
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: extract packet from "
			    "MAC FIFO %s: "
			    "%u awaiting packets (remaining "
			    "allocation = %d)\n",
			    current_superframe_sf,
			    fifo->getName().c_str(),
			    fifo->getCurrentSize(), remaining_allocation_pktpf);

			// extract next encap packet context from MAC fifo
			elem = fifo->pop();

			// delete elem context (keep only the packet)
			encap_packet = elem->getElem<NetPacket>();
			delete elem;

			// is there enough free space in the DVB frame
			// for the encapsulation packet ?
			if(encap_packet->getTotalLength() >
			   incomplete_dvb_frame->getFreeSpace())
			{
				LOG(this->log_scheduling, LEVEL_DEBUG,
				    "SF#%u: DVB frame #%u "
				    "is full, change for next one\n",
				    current_superframe_sf,
				    complete_frames_count + 1);

				complete_dvb_frames->push_back((DvbFrame *)incomplete_dvb_frame);

				// create another incomplete DVB-RCS frame
				if(!this->allocateDvbRcsFrame(&incomplete_dvb_frame))
				{
					goto error;
				}

				complete_frames_count++;

				// is there enough free space in the next DVB-RCS frame ?
				if(encap_packet->getTotalLength() >
				   incomplete_dvb_frame->getFreeSpace())
				{
					LOG(this->log_scheduling, LEVEL_ERROR,
					    "DVB-RCS frame #%u got no enough "
					    "free space, this should never "
					    "append\n", complete_frames_count + 1);
					delete encap_packet;
					continue;
				}
			}

			// add the encapsulation packet to the current DVB-RCS frame
			if(!incomplete_dvb_frame->addPacket(encap_packet))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: cannot add "
				    "extracted MAC packet in DVB frame #%u\n",
				    current_superframe_sf,
				    complete_frames_count + 1);
				delete encap_packet;
				ret = false;
				continue;
			}

			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: extracted packet added "
			    "to DVB frame #%u\n",
			    current_superframe_sf,
			    complete_frames_count + 1);
			delete encap_packet;

			// update allocation
			remaining_allocation_pktpf--;
		}
	}

	// add the incomplete DVB-RCS frame to the list of complete DVB-RCS frame
	// if it is not empty
	if(incomplete_dvb_frame != NULL)
	{
		if(incomplete_dvb_frame->getNumPackets() > 0)
		{
			complete_dvb_frames->push_back((DvbFrame *)incomplete_dvb_frame);

			// increment the counter of complete frames
			complete_frames_count++;
		}
		else
		{
			delete incomplete_dvb_frame;
		}
	}

	// print status
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: %d packets extracted from MAC FIFOs, "
	    "%u DVB frame(s) were built (remaining "
	    "allocation = %d packets)\n", current_superframe_sf,
	    init_alloc_pktpf - remaining_allocation_pktpf, 
	    complete_frames_count, remaining_allocation_pktpf);

	return ret;
error:
	return false;
}


bool ReturnSchedulingRcs::allocateDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame)
{
	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{ 
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS frame\n");
		goto error;
	}

	return true;

error:
	return false;
}


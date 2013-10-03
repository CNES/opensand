/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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


#define DBG_PACKAGE PKG_DAMA_DA
#include <opensand_conf/uti_debug.h>

#include "ReturnSchedulingRcs.h"
#include "MacFifoElement.h"
#include "OpenSandFrames.h"


ReturnSchedulingRcs::ReturnSchedulingRcs(
			const EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos):
	Scheduling(packet_handler, fifos)
{
	// set the number of PVC = the maximum PVC is (first PVC id is 1)
	this->max_pvc = 0;
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		this->max_pvc = std::max((*it).second->getPvc(), this->max_pvc);
	}
}


bool ReturnSchedulingRcs::schedule(const time_sf_t current_superframe_sf,
                                   const time_frame_t current_frame,
                                   clock_t current_time,
                                   list<DvbFrame *> *complete_dvb_frames,
                                   uint32_t &remaining_allocation)
{
	// for each PVC, schedule MAC Fifos
	for(unsigned int pvc_id = 1; pvc_id <= this->max_pvc; pvc_id++)
	{
		if(remaining_allocation > (unsigned int)pow(2.0, 8 * sizeof(rate_pktpf_t)))
		{
			UTI_INFO("Remaining allocation (%u) is too long and will be truncated\n",
			         remaining_allocation);
		}
		// extract and send encap packets from MAC FIFOs, in function of
		// UL allocation
		if(!this->macSchedule(pvc_id,
		                      current_superframe_sf,
		                      current_frame,
		                      complete_dvb_frames,
		                      (rate_pktpf_t &)remaining_allocation))
		{
			UTI_ERROR("SF#%u: MAC scheduling failed\n", current_superframe_sf);
			return false;
		}
	}

	return true;
}


bool ReturnSchedulingRcs::macSchedule(const unsigned int pvc,
                                      const time_sf_t current_superframe_sf,
                                      const time_frame_t current_frame,
                                      list<DvbFrame *> *complete_dvb_frames,
                                      rate_pktpf_t &remaining_allocation_pktpf)
{
	unsigned int complete_frames_count;
	DvbRcsFrame *incomplete_dvb_frame = NULL;
	bool ret = true;
	rate_pktpf_t init_alloc_pktpf = remaining_allocation_pktpf;
	fifos_t::const_iterator fifo_it;

	UTI_DEBUG("SF#%u: frame %u: attempt to extract encap packets from "
	          "MAC FIFOs for PVC %d (remaining allocation = %d packets)\n",
	          current_superframe_sf, current_frame,
	          pvc, remaining_allocation_pktpf);

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

		if(fifo->getPvc() != pvc)
		{
			// ignore FIFO with a different PVC
			UTI_DEBUG_L3("SF#%u: frame %u: ignore MAC FIFO "
			             "with ID %d: PVC is %d not %d\n",
			             current_superframe_sf, current_frame,
			             fifo->getPriority(),
			             fifo->getPvc(),
			             pvc);
			// pass to next fifo
			++fifo_it;
		}
		else if(fifo->getCurrentSize() <= 0)
		{
			// FIFO is on correct PVC but got no data
			UTI_DEBUG_L3("SF#%u: frame %u: ignore MAC FIFO "
			             "with ID %d: correct PVC %d but no data "
			             "(left) to schedule\n",
			             current_superframe_sf, current_frame,
			             fifo->getPriority(),
			             fifo->getPvc());
			// pass to next fifo
			++fifo_it;
		}
		else
		{
			// FIFO with correct PVC and awaiting data
			UTI_DEBUG_L3("SF#%u: frame %u: extract packet from "
			             "MAC FIFO with ID %d: correct PVC %d and "
			             "%u awaiting packets (remaining "
			             "allocation = %d)\n",
			             current_superframe_sf, current_frame,
			             fifo->getPriority(),
			             fifo->getPvc(),
			             fifo->getCurrentSize(),
			             remaining_allocation_pktpf);

			// extract next encap packet context from MAC fifo
			elem = fifo->pop();

			// delete elem context (keep only the packet)
			encap_packet = elem->getPacket();
			delete elem;

			// is there enough free space in the DVB frame
			// for the encapsulation packet ?
			if(encap_packet->getTotalLength() >
			   incomplete_dvb_frame->getFreeSpace())
			{
				UTI_DEBUG_L3("SF#%u: frame %u: DVB frame #%u "
				             "is full, change for next one\n",
				             current_superframe_sf, current_frame,
				             complete_frames_count + 1);

				complete_dvb_frames->push_back(incomplete_dvb_frame);

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
					UTI_ERROR("DVB-RCS frame #%u got no enough "
					          "free space, this should never "
					          "append\n", complete_frames_count + 1);
					delete encap_packet;
					continue;
				}
			}

			// add the encapsulation packet to the current DVB-RCS frame
			if(!incomplete_dvb_frame->addPacket(encap_packet))
			{
				UTI_ERROR("SF#%u: frame %u: cannot add "
				          "extracted MAC packet in "
				          "DVB frame #%u\n",
				          current_superframe_sf, current_frame,
				          complete_frames_count + 1);
				delete encap_packet;
				ret = false;
				continue;
			}

			UTI_DEBUG_L3("SF#%u: frame %u: extracted packet added "
			             "to DVB frame #%u\n",
			             current_superframe_sf, current_frame,
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
			complete_dvb_frames->push_back(incomplete_dvb_frame);

			// increment the counter of complete frames
			complete_frames_count++;
		}
		else
		{
			delete incomplete_dvb_frame;
		}
	}

	// print status
	UTI_DEBUG("SF#%u: frame %u: %d packets extracted from MAC FIFOs "
	          "for PVC %d, %u DVB frame(s) were built (remaining allocation "
	          "= %d packets)\n",
	          current_superframe_sf, current_frame,
	          init_alloc_pktpf - remaining_allocation_pktpf,
	          pvc, complete_frames_count,
	          remaining_allocation_pktpf);

	return ret;
error:
	return false;
}


bool ReturnSchedulingRcs::allocateDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame)
{
	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{
		UTI_ERROR("failed to create DVB-RCS frame\n");
		goto error;
	}

	// set the max size of the DVB-RCS frame, also set the type
	// of encapsulation packets the DVB-RCS frame will contain
	(*incomplete_dvb_frame)->setMaxSize(MSG_DVB_RCS_SIZE_MAX);
	(*incomplete_dvb_frame)->setEncapPacketEtherType(
						this->packet_handler->getEtherType());

	return true;

error:
	return false;
}


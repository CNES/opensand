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
 * @file     ReturnSchedulingRcs2.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS2 return link
 * @author   Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "ReturnSchedulingRcs2.h"
#include "MacFifoElement.h"
#include "OpenSandFrames.h"

#include <opensand_output/Output.h>

typedef enum
{
	state_next_fifo,      // Get the next fifo
	state_next_encap_pkt, // Get the next encapsulated packets
	state_add_chunk,      // Get the next chunk of encapsulated packets
	state_complete,       // Create a new incomplete packet
	state_end,            // End occurred
	state_error           // Error occurred
} sched_state_t;

ReturnSchedulingRcs2::ReturnSchedulingRcs2(
			const EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos):
	ReturnSchedulingRcsCommon(packet_handler, fifos)
{
}

bool ReturnSchedulingRcs2::macSchedule(const time_sf_t current_superframe_sf,
                                       list<DvbFrame *> *complete_dvb_frames,
                                       rate_pktpf_t &remaining_allocation_pktpf)
{
	int ret;
	unsigned int complete_frames_count;
	unsigned int sent_packets;
	DvbRcsFrame *incomplete_dvb_frame = NULL;
	//rate_pktpf_t init_alloc_pktpf = remaining_allocation_pktpf;
	fifos_t::const_iterator fifo_it;
	NetPacket *encap_packet = NULL;
	NetPacket *data = NULL;
	NetPacket *remaining_data = NULL;
	MacFifoElement *elem = NULL;
	DvbFifo *fifo = NULL;
	sched_state_t state;

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: attempt to extract encap packets from MAC"
	    " FIFOs (remaining allocation = %d packets)\n",
	    current_superframe_sf,
	    remaining_allocation_pktpf);

	// create an incomplete DVB-RCS frame
	if(!this->allocateDvbRcsFrame(&incomplete_dvb_frame))
	{
		return false;
	}

	// extract encap packets from MAC FIFOs while some UL capacity is available
	// (MAC fifos priorities are in MAC IDs order)
	// fifo are classified by priority value (map are ordered)
	complete_frames_count = 0;
	sent_packets = 0;
	fifo_it = this->dvb_fifos.begin();
	state = (0 < remaining_allocation_pktpf) ? state_next_encap_pkt : state_end;
	
	while (state != state_end && state != state_error)
	{
		switch (state)
		{
		case state_next_fifo:      // Get the next fifo
			// Pass to the next fifo
			++fifo_it;
			if(fifo_it == this->dvb_fifos.end())
			{
				state = state_end;
				break;
			}

			// Check the fifo
			fifo = (*fifo_it).second;
			if(fifo->getAccessType() == access_saloha)
			{
				// not the good fifo
				LOG(this->log_scheduling, LEVEL_DEBUG,
				    "SF#%u: ignore MAC FIFO %s  "
				    "not the right access type (%d)\n",
				    current_superframe_sf,
				    fifo->getName().c_str(),
				    fifo->getAccessType());
				state = state_next_fifo;
				break;
			}

			state = state_next_encap_pkt;
			break;

		case state_next_encap_pkt: // Get the next encapsulated packets
			// Check the fifo has data
			if(fifo->getCurrentSize() <= 0)
			{
				LOG(this->log_scheduling, LEVEL_DEBUG,
				    "SF#%u: ignore MAC FIFO %s: "
				    "no data (left) to schedule\n",
				    current_superframe_sf,
				    fifo->getName().c_str());
				state = state_next_fifo;
				break;
			}

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
			encap_packet = elem->getElem<NetPacket>();
			state = state_add_chunk;
			break;

		case state_add_chunk:     // Get the next chunk of encapsulated packets
			// Get part of data to send
			ret = this->packet_handler->getChunk(encap_packet,
			                                     incomplete_dvb_frame->getFreeSpace(),
			                                     &data, &remaining_data);
			if(!ret)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: error while processing packet "
				    "#%u\n", current_superframe_sf,
				    sent_packets + 1);
				delete elem;
				elem = NULL;
				delete encap_packet;
				encap_packet = NULL;
				state = state_error;
			}
			if(!data && !remaining_data)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: bad getChunk function "
				    "implementation, assert or skip packet #%u\n",
				    current_superframe_sf,
				    sent_packets + 1);
				assert(0);
				delete elem;
				elem = NULL;
				state = state_error;
				break;
			}

			// Store data to send
			state = state_complete;
			if(data)
			{
				if(!incomplete_dvb_frame->addPacket(data))
				{
					LOG(this->log_scheduling, LEVEL_ERROR,
					    "SF#%u: failed to add encapsulation "
					    "packet #%u->in BB frame with MODCOD ID %u "
					    "(packet length %zu, free space %zu",
					    current_superframe_sf,
					    sent_packets + 1,
					    incomplete_dvb_frame->getModcodId(),
					    data->getTotalLength(),
					    incomplete_dvb_frame->getFreeSpace());

					delete data;
					data = NULL;
					delete elem;
					elem = NULL;
					if(remaining_data)
					{
						delete remaining_data;
						remaining_data = NULL;
					}
					state = state_error;
					break;
				}

				// delete the NetPacket once it has been copied in the DVB-RCS2 Frame
				delete data;
				data = NULL;
				sent_packets++;
				state = state_add_chunk;
			}
			else
			{
				LOG(this->log_scheduling, LEVEL_INFO,
				    "SF#%u: not enough free space in DVB-RCS2 Frame "
				    "(%zu bytes) for %s packet (%zu bytes)\n",
				    current_superframe_sf,
				    incomplete_dvb_frame->getFreeSpace(),
				    this->packet_handler->getName().c_str(),
				    remaining_data->getTotalLength());
			}
			
			// Remaining data to send
			if(!remaining_data)
			{
				// destroy the element
				delete elem;
				elem = NULL;
				state = state_next_encap_pkt;
				break;
			}
			// replace the fifo first element with the remaining data
			elem->setElem(remaining_data);
			fifo->pushFront(elem);

			LOG(this->log_scheduling, LEVEL_INFO,
			    "SF#%u: packet fragmented, there is "
			    "still %zu bytes of data\n",
			    current_superframe_sf,
			    remaining_data->getTotalLength());
			break;

		case state_complete:       // Complete the DVB-RCS2 frame
			// Store DVB-RCS2 frame with completed frames
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: DVB frame #%u "
			    "is full, change for next one\n",
			    current_superframe_sf,
			    complete_frames_count + 1);
			complete_dvb_frames->push_back((DvbFrame *)incomplete_dvb_frame);
			complete_frames_count++;
			
			// update allocation
			remaining_allocation_pktpf--;
			
			// Check remaining allocation
			if(0 < remaining_allocation_pktpf)
			{
				fifo->pushFront(elem);
				incomplete_dvb_frame = NULL;
				state = state_end;
				break;
			}

			// Create a new incomplete DVB-RCS2 frame
			if(!this->allocateDvbRcsFrame(&incomplete_dvb_frame))
			{
				state = state_error;
				break;
			}

			state = state_add_chunk;
			break;

		default:
			state = state_error;
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: an unexpected error occurred during scheduling\n",
			    current_superframe_sf);
			break;
		}
	}

	// Check error
	if(state == state_error)
	{
		if(incomplete_dvb_frame != NULL)
		{
			delete incomplete_dvb_frame;
			incomplete_dvb_frame = NULL;
		}
		return false;
	}

	// Add the incomplete DVB-RCS2 frame to the list of complete DVB-RCS2 frame
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
			incomplete_dvb_frame = NULL;
		}
	}

	// Print status
	if(sent_packets != 0)
	{
		unsigned int cpt_frame = complete_dvb_frames->size();

		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: %u %s been scheduled and %u BB %s "
		    "completed\n", current_superframe_sf,
		    sent_packets,
		    (sent_packets > 1) ? "packets have" : "packet has",
		    cpt_frame,
		    (cpt_frame > 1) ? "frames were" : "frame was");
	}

	//LOG(this->log_scheduling, LEVEL_INFO,
	//    "SF#%u: %d packets extracted from MAC FIFOs, "
	//    "%u DVB frame(s) were built (remaining "
	//    "allocation = %d packets)\n", current_superframe_sf,
	//    init_alloc_pktpf - remaining_allocation_pktpf, 
	//    complete_frames_count, remaining_allocation_pktpf);

	return true;
}

bool ReturnSchedulingRcs2::allocateDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame)
{
	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{ 
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS2 frame\n");
		goto error;
	}

	// set the max size of the DVB-RCS2 frame, also set the type
	// of encapsulation packets the DVB-RCS2 frame will contain
	//(*incomplete_dvb_frame)->setMaxSize(MSG_DVB_RCS_SIZE_MAX);

	return true;

error:
	return false;
}


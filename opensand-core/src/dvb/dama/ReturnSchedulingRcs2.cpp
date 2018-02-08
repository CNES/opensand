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

#include <algorithm>

using std::max;

typedef enum
{
	state_next_fifo,      // Go to the next fifo
	state_get_fifo,       // Get the fifo
	state_next_encap_pkt, // Get the next encapsulated packets
	state_get_chunk,      // Get the next chunk of encapsulated packets
	state_add_data,       // Add data of the chunk of encapsulated packets
	state_finalize_frame, // Finalize frame
	state_end,            // End occurred
	state_error           // Error occurred
} sched_state_t;

ReturnSchedulingRcs2::ReturnSchedulingRcs2(
			EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos):
	ReturnSchedulingRcsCommon(packet_handler, fifos)
{
}

bool ReturnSchedulingRcs2::macSchedule(const time_sf_t current_superframe_sf,
                                       list<DvbFrame *> *complete_dvb_frames,
                                       vol_b_t &remaining_allocation_b)
{
	int ret;
	bool partial_encap;
	unsigned int complete_frames_count;
	unsigned int sent_packets;
	vol_b_t frame_length_b;
	DvbRcsFrame *incomplete_dvb_frame = NULL;
	fifos_t::const_iterator fifo_it;
	NetPacket *encap_packet = NULL;
	NetPacket *data = NULL;
	MacFifoElement *elem = NULL;
	DvbFifo *fifo = NULL;
	sched_state_t state;

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: attempt to extract encap packets from MAC"
	    " FIFOs (remaining allocation = %d kbits)\n",
	    current_superframe_sf,
	    remaining_allocation_b / 1000);

	// create an incomplete DVB-RCS frame
	if(!this->allocateDvbRcsFrame(&incomplete_dvb_frame))
	{
		return false;
	}
	//frame_length_b = incomplete_dvb_frame->getHeaderLength() << 3;
	frame_length_b = 0;

	// extract encap packets from MAC FIFOs while some UL capacity is available
	// (MAC fifos priorities are in MAC IDs order)
	// fifo are classified by priority value (map are ordered)
	complete_frames_count = 0;
	sent_packets = 0;
	fifo_it = this->dvb_fifos.begin();
	state = state_get_fifo;

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: %d DVB frames completed, remaining allocation %d kbits (%d bytes)",
	    current_superframe_sf,
	    complete_frames_count,
	    remaining_allocation_b / 1000,
	    remaining_allocation_b >> 3);
	LOG(this->log_scheduling, LEVEL_DEBUG,
		"SF#%u: DVB Frame filling (%d packets): used %d kbits (%d bytes), free %d kbits (%d bytes)",
		current_superframe_sf,
		incomplete_dvb_frame->getNumPackets(),
		frame_length_b / 1000,
		frame_length_b >> 3,
		(incomplete_dvb_frame->getFreeSpace() << 3) / 1000,
		incomplete_dvb_frame->getFreeSpace());

	while (state != state_end && state != state_error)
	{
		switch (state)
		{
		case state_next_fifo:      // Go to the next fifo

			// Pass to the next fifo
			++fifo_it;
			state = state_get_fifo;
			break;

		case state_get_fifo:        // Get the fifo

			// Check this is not the end of the fifos list
			if(fifo_it == this->dvb_fifos.end())
			{
				state = state_end;
				break;
			}

			// Check the fifo access
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
		
			// Check the encapsulated packets of the fifo
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
			    "allocation = %d kbits)\n",
			    current_superframe_sf,
			    fifo->getName().c_str(),
			    fifo->getCurrentSize(),
			    remaining_allocation_b / 1000);

			// extract next encap packet context from MAC fifo
			elem = fifo->pop();
			encap_packet = elem->getElem<NetPacket>();
			if(!encap_packet)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: error while getting packet (null) "
				    "#%u\n", current_superframe_sf,
				    sent_packets + 1);
				delete elem;
				elem = NULL;
				break;
			}

			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: Extracted packet: %d kbits (%d bytes)",
			    current_superframe_sf,
			    (encap_packet->getTotalLength() << 3) / 1000,
			    encap_packet->getTotalLength());

			state = state_get_chunk;
			break;

		case state_get_chunk:     // Get the next chunk of encapsulated packets

			// Encapsulate packet
			data = NULL;
			partial_encap = false;
			ret = this->packet_handler->encapNextPacket(encap_packet,
				incomplete_dvb_frame->getFreeSpace(),
				incomplete_dvb_frame->getPacketsCount() == 0,
				partial_encap,
				&data);
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
				state = state_next_encap_pkt;
				break;
			}
			if(!data && !partial_encap)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: bad getChunk function "
				    "implementation, assert or skip packet #%u\n",
				    current_superframe_sf,
				    sent_packets + 1);
				delete elem;
				elem = NULL;
				delete encap_packet;
				encap_packet = NULL;
				state = state_next_encap_pkt;
				break;
			}

			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: %s encapsulated packet length = %d kbits (%d bytes)",
			    current_superframe_sf,
			    partial_encap ? "Partial" : "Complete",
			    data ? (data->getTotalLength() << 3) / 1000 : 0,
			    data ? data->getTotalLength() : 0);

			// Check the frame allows data
			state = data ? state_add_data : state_finalize_frame;

			// Replace the fifo first element with the remaining data
			if(partial_encap)
			{
				// Re-insert packet
				fifo->pushFront(elem);
			}
			else
			{
				// Delete packet	
				delete elem;
				elem = NULL;
				delete encap_packet;
				encap_packet = NULL;
			}
			break;
			
		case state_add_data:       // Add data of the chunk of encapsulated packets

			// Add data to the frame
			if(!incomplete_dvb_frame->addPacket(data))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: failed to add encapsulation "
				    "packet #%u->in DVB-RCS2 frame with MODCOD ID %u "
				    "(packet length %zu, free space %zu)",
				    current_superframe_sf,
				    sent_packets + 1,
				    incomplete_dvb_frame->getModcodId(),
				    data->getTotalLength(),
				    incomplete_dvb_frame->getFreeSpace());

				delete data;
				data = NULL;

				state = state_error;
				break;
			}

			// Delete the NetPacket once it has been copied in the DVB-RCS2 Frame
			frame_length_b += data->getTotalLength() << 3;
			sent_packets++;

			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: DVB Frame filling (%d packets): used %d kbits (%d bytes), free %d kbits (%d bytes)",
				current_superframe_sf,
				incomplete_dvb_frame->getNumPackets(),
				frame_length_b / 1000,
				frame_length_b >> 3,
				(incomplete_dvb_frame->getFreeSpace() << 3) / 1000,
				incomplete_dvb_frame->getFreeSpace());

			delete data;
			data = NULL;

			// Check the frame is completed
			if(incomplete_dvb_frame->getFreeSpace() <= 0)
			{
				state = state_finalize_frame;
				break;
			}
			
			// Check there is enough remaining allocation
			if(remaining_allocation_b <= frame_length_b)
			{
				state = state_finalize_frame;
				break;
			}

			state = state_next_encap_pkt;
			break;

		case state_finalize_frame: // Finalize frame

			// is there any packets in the current DVB-RCS frame ?
			if(incomplete_dvb_frame->getNumPackets() <= 0)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "DVB-RCS2 frame #%u got no packets, "
				    "this should never append (free space %u bytes)\n",
				    complete_frames_count + 1,
				    incomplete_dvb_frame->getFreeSpace());

				frame_length_b = 0;
				
				state = state_error;
				break;
			}
			
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: DVB frame #%u "
			    "is full, change for next one\n",
			    current_superframe_sf,
			    complete_frames_count + 1);

			// Store DVB-RCS2 frame with completed frames
			complete_dvb_frames->push_back((DvbFrame *)incomplete_dvb_frame);
			complete_frames_count++;
			remaining_allocation_b = (vol_b_t)max((int)(remaining_allocation_b - frame_length_b), 0);
			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: %d DVB frames completed, remaining allocation %d kbits (%d bytes)",
				current_superframe_sf,
				complete_frames_count,
				remaining_allocation_b / 1000,
				remaining_allocation_b >> 3);

			// Check the remaining allocation
			if(remaining_allocation_b <= 0)
			{
				incomplete_dvb_frame = NULL;

				state = state_end;
				break;
			}

			// Create a new incomplete DVB-RCS2 frame
			if(!this->allocateDvbRcsFrame(&incomplete_dvb_frame))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: failed to create a new DVB frame\n",
				    current_superframe_sf);

				state = state_error;
				break;
			}
			//frame_length_b = incomplete_dvb_frame->getHeaderLength() << 3;
			frame_length_b = 0;
			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: DVB Frame filling (%d packets): used %d kbits (%d bytes), free %d kbits (%d bytes)",
				current_superframe_sf,
				incomplete_dvb_frame->getNumPackets(),
				frame_length_b / 1000,
				frame_length_b >> 3,
				(incomplete_dvb_frame->getFreeSpace() << 3) / 1000,
				incomplete_dvb_frame->getFreeSpace());

			state = state_next_encap_pkt;
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
		if(0 < incomplete_dvb_frame->getNumPackets())
		{
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: DVB frame #%u "
			    "is full, change for next one\n",
			    current_superframe_sf,
			    complete_frames_count + 1);

			// Store DVB-RCS2 frame with completed frames
			complete_dvb_frames->push_back((DvbFrame *)incomplete_dvb_frame);
			complete_frames_count++;
			remaining_allocation_b = (vol_b_t)max((int)(remaining_allocation_b - frame_length_b), 0);
			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: %d DVB frames completed, remaining allocation %d kbits (%d bytes)",
				current_superframe_sf,
				complete_frames_count,
				remaining_allocation_b / 1000,
				remaining_allocation_b >> 3);
		}
		else
		{
			delete incomplete_dvb_frame;
			incomplete_dvb_frame = NULL;
		}
	}

	// Print status
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: %d packets extracted from MAC FIFOs, "
	    "%u DVB frame(s) were built (remaining "
	    "allocation = %d kbits)\n", current_superframe_sf,
	    sent_packets, complete_frames_count,
	    remaining_allocation_b / 1000);

	return true;
}

bool ReturnSchedulingRcs2::allocateDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame)
{
	vol_bytes_t length_bytes;

	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{ 
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS2 frame\n");
		goto error;
	}

	// Get the max burst length
	length_bytes = this->max_burst_length_b >> 3;
	if(length_bytes <= 0)
	{
		delete (*incomplete_dvb_frame);
		*incomplete_dvb_frame = NULL;
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS2 frame: invalid burst length\n");
		goto error;
	}

	length_bytes += (*incomplete_dvb_frame)->getHeaderLength();
	//length_bytes += sizeof(T_DVB_PHY);
	if(MSG_DVB_RCS_SIZE_MAX < length_bytes)
	{
		length_bytes = MSG_DVB_RCS_SIZE_MAX;
	}

	// set the max size of the DVB-RCS2 frame, also set the type
	// of encapsulation packets the DVB-RCS2 frame will contain
	(*incomplete_dvb_frame)->setMaxSize(length_bytes);

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "new DVB-RCS2 frame with max length %u bytes (<= %u bytes), "
	    "payload length %u bytes, header length %u bytes\n",
	    (*incomplete_dvb_frame)->getMaxSize(),
	    MSG_DVB_RCS_SIZE_MAX,
	    (*incomplete_dvb_frame)->getFreeSpace(),
	    (*incomplete_dvb_frame)->getHeaderLength());
	return true;

error:
	return false;
}


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file     UplinkSchedulingRcs2.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS2 uplink on GW
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author   Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 */


#include "UplinkSchedulingRcs2.h"

#include "MacFifoElement.h"
#include "OpenSandFrames.h"
#include "FmtDefinitionTable.h"
#include "UnitConverterFixedSymbolLength.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

UplinkSchedulingRcs2::UplinkSchedulingRcs2(
			time_ms_t frame_duration_ms,
			EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos,
			const StFmtSimuList *const ret_sts,
			const FmtDefinitionTable *const ret_modcod_def,
			const TerminalCategoryDama *const category,
			tal_id_t gw_id):
	UplinkSchedulingRcsCommon(frame_duration_ms,
		packet_handler,
		fifos,
		ret_sts,
		ret_modcod_def,
		category,
		gw_id)
{
}

UnitConverter *UplinkSchedulingRcs2::generateUnitConverter() const
{
	vol_sym_t length_sym = 0;
	
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   RCS2_BURST_LENGTH, length_sym))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_BUFFER);
		return NULL;
	}
	if(length_sym == 0)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "invalid value '%u' value of '%s", length_sym, DELAY_BUFFER);
		return NULL;
	}
	LOG(this->log_scheduling, LEVEL_INFO,
	    "Burst length = %u sym\n", length_sym);
	
	return new UnitConverterFixedSymbolLength(this->frame_duration_ms,
		0, length_sym);

}

typedef enum
{
	state_next_encap_pkt, // Get the next encapsulated packets
	state_get_chunk,      // Get the next chunk of encapsulated packets
	state_add_data,       // Add data of the chunk of encapsulated packets
	state_finalize_frame, // Finalize frame
	state_end,            // End occurred
	state_error           // Error occurred
} sched_state_t;

bool UplinkSchedulingRcs2::scheduleEncapPackets(DvbFifo *fifo,
                                                const time_sf_t current_superframe_sf,
                                                clock_t current_time,
                                                std::list<DvbFrame *> *complete_dvb_frames,
                                                CarriersGroupDama *carriers,
                                                uint8_t modcod_id)
{
	int ret;
	bool partial_encap;
	unsigned int complete_frames_count;
	unsigned int sent_packets;
	vol_b_t frame_length_b;
	DvbRcsFrame *incomplete_dvb_frame = NULL;
	NetPacket *encap_packet = NULL;
	NetPacket *data = NULL;
	MacFifoElement *elem = NULL;
	sched_state_t state;
	vol_pkt_t remaining_allocation_pkt = carriers->getRemainingCapacity();
	vol_kb_t remaining_allocation_kb = this->converter->pktToKbits(remaining_allocation_pkt);

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: attempt to extract encap packets from MAC"
	    " FIFOs (remaining allocation = %d kbits)\n",
	    current_superframe_sf,
	    remaining_allocation_kb);

	// create an incomplete DVB-RCS frame
	if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame, modcod_id))
	{
		return false;
	}
	//frame_length_b = incomplete_dvb_frame->getHeaderLength() << 3;
	frame_length_b = 0;

	// extract encap packets from MAC FIFOs while some UL capacity is available
	// (MAC fifos priorities are in MAC IDs order)
	complete_frames_count = 0;
	sent_packets = 0;
	state = state_next_encap_pkt;

	LOG(this->log_scheduling, LEVEL_DEBUG, "[init] -------------------------------------------------");
	LOG(this->log_scheduling, LEVEL_DEBUG, "[init] next state = 'get next encap pkt'");
	
	while (state != state_end && state != state_error)
	{
		switch (state)
		{
		case state_next_encap_pkt: // Get the next encapsulated packets

			// Simulate the satellite delay
			if(fifo->getTickOut() > current_time)
			{
				LOG(this->log_scheduling, LEVEL_INFO,
					"SF#%u: packet is not scheduled for "
					"the moment, break\n", current_superframe_sf);
				// this is the first MAC FIFO element that is not ready yet,
				// there is no more work to do, break now
				state = state_end;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] next state = 'end'");
				break;
			}
		
			// Check the encapsulated packets of the fifo
			LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] "
			    "check fifo has encap packets (%u packets)", fifo->getCurrentSize());
			if(fifo->getCurrentSize() <= 0)
			{
				LOG(this->log_scheduling, LEVEL_DEBUG,
				    "SF#%u: ignore MAC FIFO %s: "
				    "no data (left) to schedule\n",
				    current_superframe_sf,
				    fifo->getName().c_str());
				state = state_end;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] next state = 'end'");
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
			    fifo->getCurrentSize(), remaining_allocation_kb);

			// extract next encap packet context from MAC fifo
			LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] get the first encap packet");
			elem = fifo->pop();
			encap_packet = elem->getElem<NetPacket>();
			if(!encap_packet)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: error while getting packet (null) "
				    "#%u\n", current_superframe_sf,
				    sent_packets + 1);
				LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] deleting 'elem'...");
				delete elem;
				elem = NULL;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] 'elem' deleted");
				LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] next state = 'next encap pkt'");
				break;
			}

			state = state_get_chunk;
			LOG(this->log_scheduling, LEVEL_DEBUG, "[next encap pkt] next state = 'get chunk'");
			break;

		case state_get_chunk:     // Get the next chunk of encapsulated packets

			// Get part of data to send
			LOG(this->log_scheduling, LEVEL_DEBUG, "[get chunk] try to get a chunk of the encap packet");
			// Encapsulate packet
			ret = this->packet_handler->encapNextPacket(encap_packet,
				incomplete_dvb_frame->getFreeSpace(),
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
				LOG(this->log_scheduling, LEVEL_DEBUG, "[get chunk] next state = 'next encap pkt'");
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
				LOG(this->log_scheduling, LEVEL_DEBUG, "[get chunk] next state = 'next encap pkt'");
				break;
			}

			// Check the frame allows data
			LOG(this->log_scheduling, LEVEL_DEBUG, "[get chunk] check frame allows data");
			if(data)
			{
				state = state_add_data;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[get chunk] next state = 'add data'");
			}
			else
			{
				state = state_finalize_frame;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[get chunk] next state = 'finalize frame'");
			}

			// Replace the fifo first element with the remaining data
			LOG(this->log_scheduling, LEVEL_DEBUG, "[get chunk] check there is remaining data");
			if(partial_encap)
			{
				// Re-insert packet
				fifo->pushFront(elem);

				LOG(this->log_scheduling, LEVEL_INFO,
				    "SF#%u: packet fragmented\n",
				    current_superframe_sf);
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
			LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] add data to the frame");
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

				LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] deleting 'data'...");
				delete data;
				data = NULL;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] 'data' deleted");

				state = state_error;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] next state = 'error'");
				break;
			}

			// Delete the NetPacket once it has been copied in the DVB-RCS2 Frame
			frame_length_b += data->getTotalLength() << 3;
			sent_packets++;
			LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] update frame length "
			    "to %u bits (< %u bits)",
			    frame_length_b, remaining_allocation_kb * 1000);
			
			LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] deleting 'data'...");
			delete data;
			data = NULL;
			LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] 'data' deleted");

			// Check the frame is completed
			LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] "
			   "check the frame is completed (free space %u bytes)",
			   incomplete_dvb_frame->getFreeSpace());
			if(incomplete_dvb_frame->getFreeSpace() <= 0)
			{
				state = state_finalize_frame;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] next state = 'finalize frame'");
				break;
			}
			
			// Check there is enough remaining allocation
			LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] "
			    "check the remaining allocation (%u bits)", remaining_allocation_kb * 1000 - frame_length_b);
			if(remaining_allocation_kb * 1000 <= frame_length_b)
			{
				state = state_finalize_frame;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] next state = 'finalize frame'");
				break;
			}

			state = state_next_encap_pkt;
			LOG(this->log_scheduling, LEVEL_DEBUG, "[add data] next state = 'next encap pkt'");
			break;

		case state_finalize_frame: // Finalize frame

			// is there any packets in the current DVB-RCS frame ?
			LOG(this->log_scheduling, LEVEL_DEBUG, "[finalize frame] "
			   "check there is packets in the frame %u", incomplete_dvb_frame->getNumPackets());
			if(incomplete_dvb_frame->getNumPackets() <= 0)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "DVB-RCS2 frame #%u got no packets, "
				    "this should never append (free space %u bytes)\n",
				    complete_frames_count + 1,
				    incomplete_dvb_frame->getFreeSpace());

				frame_length_b = 0;
				
				state = state_error;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[finalize frame] next state = 'error'");
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
			remaining_allocation_kb -= ceil(frame_length_b / 1000.);

			// Check the remaining allocation
			LOG(this->log_scheduling, LEVEL_DEBUG, "[finalize frame] "
			    "check the remaining allocation %u", remaining_allocation_kb);
			if(remaining_allocation_kb <= 0)
			{
				incomplete_dvb_frame = NULL;

				state = state_end;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[finalize frame] "
				    "next state = 'end'");
				break;
			}

			// Create a new incomplete DVB-RCS2 frame
			LOG(this->log_scheduling, LEVEL_DEBUG, "[finalize frame] "
			    "create a new DVB-RCS2 frame");
			if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame, modcod_id))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: failed to create a new DVB frame\n",
				    current_superframe_sf);

				state = state_error;
				LOG(this->log_scheduling, LEVEL_DEBUG, "[finalize frame] "
				    "next state = 'error'");
				break;
			}
			//frame_length_b = incomplete_dvb_frame->getHeaderLength() << 3;
			frame_length_b = 0;

			state = state_next_encap_pkt;
			LOG(this->log_scheduling, LEVEL_DEBUG, "[finalize frame] "
			    "next state = 'next encap pkt'");
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
			remaining_allocation_kb -= ceil(frame_length_b / 1000.);
		}
		else
		{
			delete incomplete_dvb_frame;
			incomplete_dvb_frame = NULL;
		}
	}

	remaining_allocation_pkt = this->converter->kbitsToPkt(remaining_allocation_kb);
	carriers->setRemainingCapacity(remaining_allocation_pkt);

	// Print status
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: %d packets extracted from MAC FIFOs, "
	    "%u DVB frame(s) were built (remaining "
	    "allocation = %d kbits)\n", current_superframe_sf,
	    sent_packets, complete_frames_count,
	    remaining_allocation_kb);

	return true;

}
/*
bool UplinkSchedulingRcs2::scheduleEncapPackets(DvbFifo *fifo,
                                                const time_sf_t current_superframe_sf,
                                                clock_t current_time,
                                                std::list<DvbFrame *> *complete_dvb_frames,
                                                CarriersGroupDama *carriers)
{
	unsigned int cpt_frame;
	unsigned int sent_packets;
	long max_to_send;
	DvbRcsFrame *incomplete_dvb_frame;
	vol_pkt_t remaining_capacity_pkt = carriers->getRemainingCapacity();
	
	// retrieve the number of packets waiting for transmission
	max_to_send = fifo->getCurrentSize();
	if(max_to_send <= 0)
	{
		// if there is nothing to send, return with success
		goto skip;
	}

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: send at most %ld encapsulation "
	    "packet(s)\n", current_superframe_sf,
	    max_to_send);

	// create an incomplete DVB-RCS frame
	if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame, modcod_id))
	//                                      carriers->getFmtIds().front()))
	{
		goto error;
	}

	// build DVB-RCS frames with packets extracted from the MAC FIFO
	cpt_frame = 0;
	sent_packets = 0;
	while(fifo->getCurrentSize() > 0 && remaining_capacity_pkt > 0)
	{
		MacFifoElement *elem;
		NetPacket *encap_packet;

		// simulate the satellite delay
		if(fifo->getTickOut() > current_time)
		{
			LOG(this->log_scheduling, LEVEL_INFO,
			    "SF#%u: packet is not scheduled for "
			    "the moment, break\n", current_superframe_sf);
			// this is the first MAC FIFO element that is not ready yet,
			// there is no more work to do, break now
			break;
		}

		elem = fifo->pop();

		// retrieve the encapsulation packet and delete element
		encap_packet = elem->getElem<NetPacket>();
		delete elem;

		// check the validity of the encapsulation packet
		if(encap_packet == NULL)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: invalid packet #%u\n",
			    current_superframe_sf,
			    sent_packets + 1);
			goto error;
		}

		// is there enough free space in the current DVB-RCS frame
		// for the encapsulation packet ?
		if(encap_packet->getTotalLength() >
		   incomplete_dvb_frame->getFreeSpace())
		{
			// no more room in the current DVB-RCS frame: the
			// encapsulation is of constant length so we can
			// not fragment the packet and we must complete the
			// DVB-RCS frame with padding. So:
			//  - add padding to the DVB-RCS frame (not done yet)
			//  - put the DVB-RCS frame in the list of complete frames
			//  - use the next DVB-RCS frame
			//  - put the encapsulation packet in this next DVB-RCS frame

			LOG(this->log_scheduling, LEVEL_INFO,
			    "SF#%u: DVB-RCS frame #%u does not "
			    "contain enough free space (%zu bytes) for the "
			    "encapsulation packet (%zu bytes), pad the "
			    "DVB-RCS frame and send it\n",
			    current_superframe_sf,
			    cpt_frame, incomplete_dvb_frame->getFreeSpace(),
			    encap_packet->getTotalLength());

			complete_dvb_frames->push_back((DvbFrame *)incomplete_dvb_frame);

			// create another incomplete DVB-RCS frame
			if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame,
			                                      carriers->getFmtIds().front()))
			{
				goto error;
			}

			// go to next frame
			cpt_frame++;

			// is there enough free space in the next DVB-RCS frame ?
			if(encap_packet->getTotalLength() >
			   incomplete_dvb_frame->getFreeSpace())
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: DVB-RCS frame #%u got no "
				    "enough free space, this should never append\n",
				    current_superframe_sf,
				    cpt_frame);
				delete encap_packet;
				goto error;
			}
		}

		// add the encapsulation packet to the current DVB-RCS frame
		if(!incomplete_dvb_frame->addPacket(encap_packet))
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: failed to add encapsulation "
			    "packet #%u in DVB-RCS frame #%u",
			    current_superframe_sf,
			    sent_packets + 1, cpt_frame);
			delete encap_packet;
			goto error;
		}
		sent_packets++;
		remaining_capacity_pkt--;
		delete encap_packet;
	}

	// add the incomplete DVB-RCS frame to the list of complete DVB-RCS frame
	// if it is not empty
	if(incomplete_dvb_frame != NULL)
	{
		if(incomplete_dvb_frame->getNumPackets() > 0)
		{
			complete_dvb_frames->push_back((DvbFrame *)incomplete_dvb_frame);

			// increment the counter of complete frames
			cpt_frame++;
		}
		else
		{
			delete incomplete_dvb_frame;
		}
	}

	carriers->setRemainingCapacity(remaining_capacity_pkt);

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: %u packet(s) have been scheduled in "
	    "%u DVB-RCS frames \n", current_superframe_sf, 
	    sent_packets, cpt_frame);

skip:
	return true;

error:
	return false;
}
*/

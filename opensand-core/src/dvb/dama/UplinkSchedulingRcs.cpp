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
 * @file     UplinkSchedulingRcs.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS uplink on GW
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author   Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 */


#include "UplinkSchedulingRcs.h"

#include "MacFifoElement.h"
#include "OpenSandFrames.h"
#include "FmtDefinitionTable.h"
#include "UnitConverterFixedBitLength.h"

#include <opensand_output/Output.h>


UplinkSchedulingRcs::UplinkSchedulingRcs(
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

UnitConverter *UplinkSchedulingRcs::generateUnitConverter() const
{
	vol_b_t length_b;
	
	length_b = this->packet_handler->getFixedLength() << 3;

	return new UnitConverterFixedBitLength(this->frame_duration_ms,
		0, length_b);
}

bool UplinkSchedulingRcs::scheduleEncapPackets(DvbFifo *fifo,
                                               const time_sf_t current_superframe_sf,
                                               clock_t current_time,
                                               std::list<DvbFrame *> *complete_dvb_frames,
                                               CarriersGroupDama *carriers,
                                               uint8_t modcod_id)
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
			                                      modcod_id))
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


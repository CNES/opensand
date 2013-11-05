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
 * @file     UplinkSchedulingRcs.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS uplink on GW
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 */


#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>

#include "UplinkSchedulingRcs.h"
#include "MacFifoElement.h"
#include "OpenSandFrames.h"
#include "FmtDefinitionTable.h"


UplinkSchedulingRcs::UplinkSchedulingRcs(
			const EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos,
			unsigned int frames_per_superframe,
            const FmtSimulation * const fmt_simu,
			const TerminalCategory *const category):
	Scheduling(packet_handler, fifos),
	frames_per_superframe(frames_per_superframe),
	fmt_simu(fmt_simu),
	category(category)
{
}


bool UplinkSchedulingRcs::schedule(const time_sf_t current_superframe_sf,
                                   const time_frame_t current_frame,
                                   clock_t current_time,
                                   list<DvbFrame *> *complete_dvb_frames,
                                   uint32_t &UNUSED(remaining_allocation))
{
	fifos_t::const_iterator fifo_it;
	vector<CarriersGroup *> carriers;
	vector<CarriersGroup *>::iterator carrier_it;
	carriers = this->category->getCarriersGroups();

	// TODO
	// at beginning we set the carrier capa to the total capa + the previous
	// non allocated capa then for each fifo we use this capacity
	// at the end the non allocated capa remain
	// BUT should we keep complete remaining capacity in all cases ?
	// there can remain more than capa for a timeslot if nothing was consumed

	// initialize carriers capacity
	for(carrier_it = carriers.begin();
	    carrier_it != carriers.end();
	    ++carrier_it)
	{
		vol_kb_t remaining_capacity_kb;
		rate_pktpf_t remaining_capacity_pktpf;
		const FmtDefinitionTable *modcod_def;

		modcod_def = this->fmt_simu->getRetModcodDefinitions();
		// we have only one MODCOD for each carrier so we can convert
		// directly from bauds to kbits
		remaining_capacity_kb =
			modcod_def->symToKbits((*carrier_it)->getFmtIds().front(),
			                       (*carrier_it)->getTotalCapacity());
		// as this function is called each superframe we can directly
		// convert number of packet to rate in packet per superframe
		// and dividing by the frame number per superframes we have
		// the rate in packet per frame
		remaining_capacity_pktpf =
			floor(((remaining_capacity_kb * 1000) /
			       (this->packet_handler->getFixedLength() * 8)) /
			      this->frames_per_superframe);

		// initialize remaining capacity with total capacity in
		// packet per superframe as it is the unit used in DAMA computations
		(*carrier_it)->setRemainingCapacity(remaining_capacity_pktpf);
		UTI_DEBUG("SF#%u: capacity before scheduling on GW uplink %u: "
		          "%u packet (per frame) (%u kb)",
		          current_superframe_sf,
		          (*carrier_it)->getCarriersId(),
		          remaining_capacity_pktpf,
		          remaining_capacity_kb / this->frames_per_superframe);
	}

	for(fifo_it = this->dvb_fifos.begin();
	    fifo_it != this->dvb_fifos.end(); ++fifo_it)
	{
		for(carrier_it = carriers.begin();
		    carrier_it != carriers.end();
		    ++carrier_it)
		{
			if(!this->scheduleEncapPackets((*fifo_it).second,
			                               current_superframe_sf,
			                               current_frame,
			                               current_time,
			                               complete_dvb_frames,
			                               (*carrier_it)))
			{
				return false;
			}
		}
	}
	return true;
}


bool UplinkSchedulingRcs::scheduleEncapPackets(DvbFifo *fifo,
                                               const time_sf_t current_superframe_sf,
                                               const time_frame_t current_frame,
                                               clock_t current_time,
                                               std::list<DvbFrame *> *complete_dvb_frames,
                                               CarriersGroup *carriers)
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

	UTI_DEBUG("SF#%u: frame %u: send at most %ld encapsulation packet(s)\n",
	          current_superframe_sf, current_frame, max_to_send);

	// create an incomplete DVB-RCS frame
	if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame))
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
			UTI_DEBUG("SF#%u: frame %u: packet is not scheduled for the moment, "
			          "break\n", current_superframe_sf, current_frame);
			// this is the first MAC FIFO element that is not ready yet,
			// there is no more work to do, break now
			break;
		}

		elem = fifo->pop();
		// examine the packet to be sent
		if(elem->getType() != 1)
		{
			UTI_ERROR("SF#%u: frame %u: MAC FIFO element does not contain NetPacket\n",
			          current_superframe_sf, current_frame);
			goto error;
		}

		// retrieve the encapsulation packet and delete element
		encap_packet = elem->getPacket();
		delete elem;

		// check the validity of the encapsulation packet
		if(encap_packet == NULL)
		{
			UTI_ERROR("SF#%u: frame %u: invalid packet #%u\n",
			          current_superframe_sf, current_frame, sent_packets + 1);
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

			UTI_DEBUG("SF#%u: frame %u: DVB-RCS frame #%u does not contain enough "
			          "free space (%u bytes) for the encapsulation "
			          "packet (%u bytes), pad the DVB-RCS frame "
			          "and send it\n", current_superframe_sf, current_frame,
			          cpt_frame, incomplete_dvb_frame->getFreeSpace(),
			          encap_packet->getTotalLength());

			complete_dvb_frames->push_back(incomplete_dvb_frame);

			// create another incomplete DVB-RCS frame
			if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame))
			{
				goto error;
			}

			// go to next frame
			cpt_frame++;

			// is there enough free space in the next DVB-RCS frame ?
			if(encap_packet->getTotalLength() >
			   incomplete_dvb_frame->getFreeSpace())
			{
				UTI_ERROR("SF#%u: frame %u: DVB-RCS frame #%u got no enough "
				          "free space, this should never append\n",
				          current_superframe_sf, current_frame, cpt_frame);
				delete encap_packet;
				goto error;
			}
		}

		// add the encapsulation packet to the current DVB-RCS frame
		if(!incomplete_dvb_frame->addPacket(encap_packet))
		{
			UTI_ERROR("SF#%u: frame %u: failed to add encapsulation packet #%u "
			          "in DVB-RCS frame #%u", current_superframe_sf, current_frame,
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
			complete_dvb_frames->push_back(incomplete_dvb_frame);

			// increment the counter of complete frames
			cpt_frame++;
		}
		else
		{
			delete incomplete_dvb_frame;
		}
	}

	carriers->setRemainingCapacity(remaining_capacity_pkt);

	UTI_DEBUG("SF#%u: frame %u: %u packet(s) have been scheduled in %u DVB-RCS "
	          "frames \n", current_superframe_sf, current_frame,
	          sent_packets, cpt_frame);

skip:
	return true;

error:
	return false;
}


bool UplinkSchedulingRcs::createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame)
{
	if(!this->packet_handler)
	{
		UTI_ERROR("packet handler is NULL\n");
		goto error;
	}

	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{
		UTI_ERROR("failed to create DVB-RCS frame\n");
		goto error;
	}

	// set the max size of the DVB-RCS frame, also set the type
	// of encapsulation packets the DVB-RCS frame will contain
	// we do not need to handle MODCOD here because the size to send is
	// managed by the allocation, the DVB frame is only an abstract
	// object to transport data
	(*incomplete_dvb_frame)->setMaxSize(MSG_DVB_RCS_SIZE_MAX);
	(*incomplete_dvb_frame)->setEncapPacketEtherType(
								this->packet_handler->getEtherType());

	return true;

error:
	return false;
}



/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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


#include "UplinkSchedulingRcs.h"

#include "MacFifoElement.h"
#include "OpenSandFrames.h"
#include "FmtDefinitionTable.h"

#include <opensand_output/Output.h>


UplinkSchedulingRcs::UplinkSchedulingRcs(
			const EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos,
			const map<tal_id_t, StFmtSimu *> *const ret_sts,
			const FmtDefinitionTable *const ret_modcod_def,
			const TerminalCategoryDama *const category,
			tal_id_t gw_id):
	Scheduling(packet_handler, fifos),
	gw_id(gw_id),
	ret_sts(ret_sts),
	ret_modcod_def(ret_modcod_def),
	category(category)
{

}


bool UplinkSchedulingRcs::schedule(const time_sf_t current_superframe_sf,
                                   clock_t current_time,
                                   list<DvbFrame *> *complete_dvb_frames,
                                   uint32_t &UNUSED(remaining_allocation))
{
	fifos_t::const_iterator fifo_it;
	vector<CarriersGroupDama *> carriers;
	vector<CarriersGroupDama *>::iterator carrier_it;
	carriers = this->category->getCarriersGroups();
	uint8_t desired_modcod = this->retrieveCurrentModcod();
	bool found_modcod = false;

	// FIXME we consider the band is not the same for GW and terminals (this
	//       is a good consideration...) but as we have only one band configuration
	//       we use the same parameters
	// initialize carriers capacity
	//
	for(carrier_it = carriers.begin();
	    carrier_it != carriers.end();
	    ++carrier_it)
	{
		vol_kb_t remaining_capacity_kb;
		rate_pktpf_t remaining_capacity_pktpf;
		uint8_t modcod_id;
		CarriersGroupDama *carriers = *carrier_it;

		// get best modcod ID according to carrier
		modcod_id = carriers->getNearestFmtId(desired_modcod);
		if(modcod_id == 0)
		{
			LOG(this->log_scheduling, LEVEL_NOTICE,
			    "cannot use any modcod (desired %u) "
			    "to send on on carrier %u\n", desired_modcod,
			    carriers->getCarriersId());
			// no available allocation on this carrier
			carriers->setRemainingCapacity(0);
			continue;
		}
		found_modcod = true;
		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "Available MODCOD for GW = %u\n", modcod_id);

		remaining_capacity_kb =
			this->ret_modcod_def->symToKbits(modcod_id,
			                      carriers->getTotalCapacity());
		// as this function is called each superframe we can directly
		// convert number of packet to rate in packet per superframe
		// and dividing by the frame number per superframes we have
		// the rate in packet per frame
		remaining_capacity_pktpf =
			floor(((remaining_capacity_kb * 1000) /
			       (this->packet_handler->getFixedLength() * 8)));

		// initialize remaining capacity with total capacity in
		// packet per superframe as it is the unit used in DAMA computations
		carriers->setRemainingCapacity(remaining_capacity_pktpf);
		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: capacity before scheduling on GW uplink %u: "
		    "%u packet (per frame) (%u kb)",
		    current_superframe_sf,
		    carriers->getCarriersId(),
		    remaining_capacity_pktpf,
		    remaining_capacity_kb);
	}
	if(!found_modcod)
	{
		LOG(this->log_scheduling, LEVEL_WARNING,
		    "No carrier found to use modcod %u\n",
		    desired_modcod);
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
			                               current_time,
			                               complete_dvb_frames,
			                               *carrier_it))
			{
				return false;
			}
		}
	}
	return true;
}


bool UplinkSchedulingRcs::scheduleEncapPackets(DvbFifo *fifo,
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
	if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame,
	                                      carriers->getFmtIds().front()))
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


bool UplinkSchedulingRcs::createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame,
                                                      uint8_t modcod_id)
{
	if(!this->packet_handler)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "packet handler is NULL\n");
		goto error;
	}

	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS frame\n");
		goto error;
	}

	// set the max size of the DVB-RCS frame, also set the type
	// of encapsulation packets the DVB-RCS frame will contain
	// we do not need to handle MODCOD here because the size to send is
	// managed by the allocation, the DVB frame is only an abstract
	// object to transport data
	(*incomplete_dvb_frame)->setMaxSize(MSG_DVB_RCS_SIZE_MAX);
	(*incomplete_dvb_frame)->setModcodId(modcod_id);

	return true;

error:
	return false;
}


uint8_t UplinkSchedulingRcs::getCurrentModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->ret_sts->find(id);
	if(st_iter != this->ret_sts->end())
	{
		return (*st_iter).second->getCurrentModcodId();
	}
	return 0;
}


uint8_t UplinkSchedulingRcs::retrieveCurrentModcod(void)
{
	uint8_t modcod_id = this->getCurrentModcodId(this->gw_id);
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "Simulated MODCOD for GW%u = %u\n", this->gw_id, modcod_id);

	return modcod_id;
}


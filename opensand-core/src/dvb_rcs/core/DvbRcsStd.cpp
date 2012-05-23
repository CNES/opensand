/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file DvbRcsStd.cpp
 * @brief DVB-RCS Transmission Standard
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "DvbRcsStd.h"
#include "assert.h"

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include "opensand_conf/uti_debug.h"



DvbRcsStd::DvbRcsStd(EncapPlugin::EncapPacketHandler *pkt_hdl):
	PhysicStd("DVB-RCS", pkt_hdl)
{
	/* these values are not used here */
	this->realModcod = 0;
	this->receivedModcod = this->realModcod;
	this->generic_switch = NULL;
}


DvbRcsStd::~DvbRcsStd()
{
	if(this->generic_switch != NULL)
	{
		delete this->generic_switch;
	}
}

int DvbRcsStd::scheduleEncapPackets(dvb_fifo *fifo,
                                    long current_time,
                                    std::list<DvbFrame *> *complete_dvb_frames)
{
	unsigned int cpt_frame;
	unsigned int sent_packets;
	MacFifoElement *elem;
	long max_to_send;
	DvbRcsFrame *incomplete_dvb_frame;

	// retrieve the number of packets waiting for transmission
	max_to_send = fifo->getCount();
	if(max_to_send <= 0)
	{
		// if there is nothing to send, return with success
		goto skip;
	}

	UTI_DEBUG("send at most %ld encapsulation packet(s)\n",
	          max_to_send);

	// create an incomplete DVB-RCS frame
	if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame))
	{
		goto error;
	}

	// build DVB-RCS frames with packets extracted from the MAC FIFO
	cpt_frame = 0;
	sent_packets = 0;
	while((elem = (MacFifoElement *) fifo->get()) != NULL)
	{
		NetPacket *encap_packet;

		// TODO remove ?
		// first examine the packet to be sent without removing it from the queue
		if(elem->getType() != 1)
		{
			UTI_ERROR("MAC FIFO element does not contain NetPacket\n");
			goto error;
		}
		// simulate the satellite delay
		if(elem->getTickOut() > current_time)
		{
			UTI_DEBUG("packet is not scheduled for the moment, "
			          "break\n");
			// this is the first MAC FIFO element that is not ready yet,
			// there is no more work to do, break now
			break;
		}

		// retrieve the encapsulation packet
		encap_packet = elem->getPacket();

		// remove the cell from the MAC FIFO and destroy it
		fifo->remove();
		delete elem;

		// check the validity of the encapsulation packet
		if(encap_packet == NULL)
		{
			UTI_ERROR("invalid packet #%u\n", sent_packets + 1);
			goto error;
		}

		// introduce error now only for satellite !!!!!
		// and do not use component_bloc
#if 0 /* TODO: enable the error generator again */
		if(((BlocDVBRcsSat *) this->component_bloc)->m_useErrorGenerator)
		{
			((BlocDVBRcsSat *) this->component_bloc)->errorGenerator(encap_packet);
		}
#endif

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

			UTI_DEBUG("DVB-RCS frame #%u does not contain enough "
			          "free space (%u bytes) for the encapsulation "
			          "packet (%u bytes), pad the DVB-RCS frame "
			          "and send it\n", cpt_frame,
			          incomplete_dvb_frame->getFreeSpace(),
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
				UTI_ERROR("DVB-RCS frame #%u got no enough "
				          "free space, this should never "
				          "append\n", cpt_frame);
				delete encap_packet;
				goto error;
			}
		}

		// add the encapsulation packet to the current DVB-RCS frame
		if(!incomplete_dvb_frame->addPacket(encap_packet))
		{
			UTI_ERROR("failed to add encapsulation packet #%u "
			          "in DVB-RCS frame #%u", sent_packets + 1,
			          cpt_frame);
			delete encap_packet;
			goto error;
		}
		sent_packets++;
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

	UTI_DEBUG("%u packet(s) have been scheduled in %u DVB-RCS "
	          "frames \n", sent_packets, cpt_frame);

skip:
	return 0;

error:
	return -1;
}


int DvbRcsStd::createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame)
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
	(*incomplete_dvb_frame)->setMaxSize(MSG_DVB_RCS_SIZE_MAX);
	(*incomplete_dvb_frame)->setEncapPacketEtherType(
								this->packet_handler->getEtherType());

	return 1;

error:
	return 0;
}


int DvbRcsStd::onRcvFrame(unsigned char *frame,
                          long length,
                          long type,
                          int mac_id,
                          NetBurst **burst)
{
	T_DVB_ENCAP_BURST *dvb_burst;  // DVB burst received from lower layer
	long i;                        // counter for packets
	size_t offset;
	size_t previous_length = 0;

	dvb_burst = (T_DVB_ENCAP_BURST *) frame;
	if(dvb_burst->qty_element <= 0)
	{
		UTI_DEBUG("skip DVB-RCS frame with no encapsulation packet\n");
		goto skip;
	}
	if(!this->packet_handler)
	{
		UTI_ERROR("packet handler is NULL\n");
		goto error;
	}
	if(packet_handler->getFixedLength() == 0)
	{
		UTI_ERROR("encapsulated packets length is not fixed on "
		          "a DVB-RCS emission link (packet type is %s)\n",
		          packet_handler->getName().c_str());
		return false;
	}

	if(type != MSG_TYPE_DVB_BURST)
	{
		UTI_ERROR("the message received is not a DVB burst\n");
		goto error;
	}
	UTI_DEBUG("%s burst received (%ld packet(s))\n",
	          this->packet_handler->getName().c_str(), dvb_burst->qty_element);

	// create an empty burst of encapsulation packets
	*burst = new NetBurst();
	if(*burst == NULL)
	{
		UTI_ERROR("failed to create a burst of packets\n");
		goto error;
	}

	// add packets received from lower layer
	// to the newly created burst
	offset = sizeof(T_DVB_ENCAP_BURST);
	for(i = 0; i < dvb_burst->qty_element; i++)
	{
		NetPacket *encap_packet; // one encapsulation packet
		size_t current_length;

		current_length = this->packet_handler->getLength(frame + offset + previous_length);
		// Use default values for QoS, source/destination tal_id
		encap_packet = this->packet_handler->build(frame + offset + previous_length,
		                                           current_length,
		                                           0x00, BROADCAST_TAL_ID, BROADCAST_TAL_ID);
		previous_length += current_length;
		if(encap_packet == NULL)
		{
			UTI_ERROR("cannot create one %s packet\n",
			          this->packet_handler->getName().c_str());
			goto release_burst;
		}

		// satellite part
		if(this->generic_switch != NULL)
		{
			uint8_t spot_id;

			// find the spot ID associated to the packet, it will
			// be used to put the cell in right fifo after the Encap SAT bloc
			spot_id = this->generic_switch->find(encap_packet);
			if(spot_id == 0)
			{
				UTI_ERROR("unable to find destination spot, drop the "
				          "packet\n");
				delete encap_packet;
				continue;
			}
			// associate the spot ID to the packet
			encap_packet->setDstSpot(spot_id);
		}


		// add the packet to the burst of packets
		(*burst)->add(encap_packet);
		UTI_DEBUG("%s packet (%d bytes) added to burst\n",
		          this->packet_handler->getName().c_str(),
		          encap_packet->getTotalLength());
	}

skip:
	// release buffer (data is now saved in NetPacket objects)
	g_memory_pool_dvb_rcs.release((char *) frame);
	return 0;

release_burst:
	delete burst;
error:
	g_memory_pool_dvb_rcs.release((char *) frame);
	return -1;
}

bool DvbRcsStd::setSwitch(GenericSwitch *generic_switch)
{
	if(generic_switch == NULL)
	{
		return false;
	}

	this->generic_switch = generic_switch;

	return true;
}


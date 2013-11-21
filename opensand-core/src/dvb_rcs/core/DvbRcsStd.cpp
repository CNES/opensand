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

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>

#include "DvbRcsStd.h"

#include <cassert>


DvbRcsStd::DvbRcsStd(const EncapPlugin::EncapPacketHandler * pkt_hdl):
	PhysicStd("DVB-RCS", pkt_hdl)
{
	this->generic_switch = NULL;
}


DvbRcsStd::~DvbRcsStd()
{
	if(this->generic_switch != NULL)
	{
		delete this->generic_switch;
	}
}


int DvbRcsStd::onRcvFrame(unsigned char *frame,
                          long UNUSED(length),
                          long type,
                          tal_id_t UNUSED(tal_id),
                          NetBurst **burst)
{
	T_DVB_ENCAP_BURST *dvb_burst;  // DVB burst received from lower layer
	long i;                        // counter for packets
	size_t offset;
	size_t previous_length = 0;

	if(type == MSG_TYPE_CORRUPTED)
	{
		// corrupted, nothing more to do
		UTI_DEBUG("The Frame was corrupted by physical layer, drop it\n");
		goto skip;
	}

	if(type != MSG_TYPE_DVB_BURST)
	{
		UTI_ERROR("the message received is not a DVB burst\n");
		goto error;
	}


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
	if(this->packet_handler->getFixedLength() == 0)
	{
		UTI_ERROR("encapsulated packets length is not fixed on "
		          "a DVB-RCS emission link (packet type is %s)\n",
		          this->packet_handler->getName().c_str());
		return false;
	}

	UTI_DEBUG("%s burst received (%u packet(s))\n",
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
		                                           0x00, BROADCAST_TAL_ID,
		                                           BROADCAST_TAL_ID);
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
	free(frame);
	return 0;

release_burst:
	delete burst;
error:
	free(frame);
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


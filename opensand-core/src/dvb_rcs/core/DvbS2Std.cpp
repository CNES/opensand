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
 * @file DvbS2Std.cpp
 * @brief DVB-S2 Transmission Standard
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>

#include "DvbS2Std.h"

#include <cassert>
#include <algorithm>

using std::list;


DvbS2Std::DvbS2Std(const EncapPlugin::EncapPacketHandler *const pkt_hdl):
	PhysicStd("DVB-S2", pkt_hdl),
	// use maximum MODCOD ID at startup in order to authorize any incoming trafic
	real_modcod(28), // TODO fmt_simu->getmaxFwdModcod()
	received_modcod(this->real_modcod)
{
}

DvbS2Std::~DvbS2Std()
{
}


// TODO factorize with DVB-RCS function ?
int DvbS2Std::onRcvFrame(unsigned char *frame,
                         long length,
                         long type,
                         tal_id_t tal_id,
                         NetBurst **burst)
{
	// TODO insted of cast in T_DVB_BBFRAME use this !
	BBFrame bbframe_burst(frame, length);
	long i;                       // counter for packets
	int real_mod;                 // real modcod of the receiver

	// Offset from beginning of frame to beginning of data
	size_t previous_length = 0;

	*burst = NULL;

	// sanity check
	if(frame == NULL)
	{
		UTI_ERROR("invalid frame received\n");
		goto error;
	}

	if(!this->packet_handler)
	{
		UTI_ERROR("packet handler is NULL\n");
		goto error;
	}

	// sanity check: this function only handle BB frames
	// keep corrupted for MODCOD updating
	if(type != MSG_TYPE_BBFRAME && type != MSG_TYPE_CORRUPTED)
	{
		UTI_ERROR("the message received is not a BB frame\n");
		goto error;
	}
	if(bbframe_burst.getEncapPacketEtherType() != this->packet_handler->getEtherType())
	{
		UTI_ERROR("Bad packet type (%d) in BB frame burst (expecting %d)\n",
		          bbframe_burst.getEncapPacketEtherType(),
		          this->packet_handler->getEtherType());
		goto error;
	}
	UTI_DEBUG("BB frame received (%d %s packet(s)\n",
	           bbframe_burst.getDataLength(),
	           this->packet_handler->getName().c_str());

	// retrieve the current real MODCOD of the receiver
	// (do this before any MODCOD update occurs)
	real_mod = this->real_modcod;

	// check if there is an update of the real MODCOD among all the real
	// MODCOD options located just after the header of the BB frame
	bbframe_burst.getRealModcod(tal_id, this->real_modcod);

	// used for terminal statistics
	this->received_modcod = bbframe_burst.getModcodId();

	if(type != MSG_TYPE_BBFRAME)
	{
		// corrupted, nothing more to do
		goto drop;
	}

	// is the ST able to decode the received BB frame ?
	if(this->received_modcod > real_mod)
	{
		// the BB frame is not robust enough to be decoded, drop it
		UTI_ERROR("received BB frame is encoded with MODCOD %d and "
		          "the real MODCOD of the BB frame (%d) is not "
		          "robust enough, so emulate a lost BB frame\n",
		          this->received_modcod, real_mod);
		goto drop;
	}

	if(bbframe_burst.getDataLength() <= 0)
	{
		UTI_DEBUG("skip BB frame with no encapsulation packet\n");
		goto skip;
	}

	// now we are sure that the BB frame is robust enough to be decoded,
	// so create an empty burst of encapsulation packets to store the
	// encapsulation packets we are about to extract from the BB frame
	*burst = new NetBurst();
	if(*burst == NULL)
	{
		UTI_ERROR("failed to create a burst of packets\n");
		goto error;
	}

	// add packets received from lower layer
	// to the newly created burst
	for(i = 0; i < bbframe_burst.getDataLength(); i++)
	{
		NetPacket *encap_packet;
		size_t current_length;

		current_length = this->packet_handler->getLength(
								bbframe_burst.getPayload().c_str() +
								previous_length);
		// Use default values for QoS, source/destination tal_id
		encap_packet = this->packet_handler->build(
								// TODO remove cast if build accepts const
								(unsigned char *)bbframe_burst.getPayload().c_str() +
								previous_length,
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

		// add the packet to the burst of packets
		(*burst)->add(encap_packet);
		UTI_DEBUG("%s packet (%d bytes) added to burst\n",
		          this->packet_handler->getName().c_str(),
		          encap_packet->getTotalLength());
	}

drop:
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



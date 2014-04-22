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


#include "DvbS2Std.h"

#include <opensand_output/Output.h>

#include <cassert>
#include <algorithm>

using std::list;


DvbS2Std::DvbS2Std(const EncapPlugin::EncapPacketHandler *const pkt_hdl):
	PhysicStd("DVB-S2", pkt_hdl),
	// use maximum MODCOD ID at startup in order to authorize any incoming trafic
	real_modcod(28), // TODO fmt_simu->getmaxFwdModcod()
	received_modcod(this->real_modcod)
{
	// Output Logs
	this->log_rcv_from_down = Output::registerLog(LEVEL_WARNING,
	                                              "Dvb.Upward.receive");
}

DvbS2Std::~DvbS2Std()
{
}


// TODO factorize with DVB-RCS function ?
bool DvbS2Std::onRcvFrame(DvbFrame *dvb_frame,
                          tal_id_t tal_id,
                          NetBurst **burst)
{
	BBFrame *bbframe_burst;
	long i;                       // counter for packets
	int real_mod;                 // real modcod of the receiver

	// Offset from beginning of frame to beginning of data
	size_t previous_length = 0;

	*burst = NULL;

	// sanity check
	if(dvb_frame == NULL)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "invalid frame received\n");
		goto error;
	}

	if(!this->packet_handler)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "packet handler is NULL\n");
		goto error;
	}

	// sanity check: this function only handle BB frames
	// keep corrupted for MODCOD updating
	if(dvb_frame->getMessageType() != MSG_TYPE_BBFRAME &&
	   dvb_frame->getMessageType() != MSG_TYPE_CORRUPTED)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "the message received is not a BB frame\n");
		goto error;
	}

	// TODO bbframe_burst = static_cast<BBFrame *>(dvb_frame);
	bbframe_burst = dvb_frame->operator BBFrame*();
	LOG(this->log_rcv_from_down, LEVEL_INFO,
	    "BB frame received (%d %s packet(s)\n",
	    bbframe_burst->getDataLength(),
	    this->packet_handler->getName().c_str());

	// retrieve the current real MODCOD of the receiver
	// (do this before any MODCOD update occurs)
	real_mod = this->real_modcod;

	// check if there is an update of the real MODCOD among all the real
	// MODCOD options located just after the header of the BB frame
	bbframe_burst->getRealModcod(tal_id, this->real_modcod);

	// used for terminal statistics
	this->received_modcod = bbframe_burst->getModcodId();

	if(bbframe_burst->getMessageType() == MSG_TYPE_CORRUPTED)
	{
		// corrupted, nothing more to do
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "The BBFrame was corrupted by physical layer, drop "
		    "it\n");
		goto drop;
	}

	// is the ST able to decode the received BB frame ?
	if(this->received_modcod > real_mod)
	{
		// the BB frame is not robust enough to be decoded, drop it
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "received BB frame is encoded with MODCOD %d and "
		    "the real MODCOD of the BB frame (%d) is not "
		    "robust enough, so emulate a lost BB frame\n",
		    this->received_modcod, real_mod);
		goto drop;
	}

	if(bbframe_burst->getDataLength() <= 0)
	{
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "skip BB frame with no encapsulation packet\n");
		goto skip;
	}

	// now we are sure that the BB frame is robust enough to be decoded,
	// so create an empty burst of encapsulation packets to store the
	// encapsulation packets we are about to extract from the BB frame
	*burst = new NetBurst();
	if(*burst == NULL)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "failed to create a burst of packets\n");
		goto error;
	}

	// add packets received from lower layer
	// to the newly created burst
	for(i = 0; i < bbframe_burst->getDataLength(); i++)
	{
		NetPacket *encap_packet;
		size_t current_length;

		current_length = this->packet_handler->getLength(
								bbframe_burst->getPayload(previous_length).c_str());
		// Use default values for QoS, source/destination tal_id
		encap_packet = this->packet_handler->build(
								bbframe_burst->getPayload(previous_length),
								current_length,
								0x00, BROADCAST_TAL_ID,
								BROADCAST_TAL_ID);
		previous_length += current_length;
		if(encap_packet == NULL)
		{
			LOG(this->log_rcv_from_down, LEVEL_ERROR,
			    "cannot create one %s packet\n",
			    this->packet_handler->getName().c_str());
			goto release_burst;
		}

		// add the packet to the burst of packets
		(*burst)->add(encap_packet);
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "%s packet (%zu bytes) added to burst\n",
		    this->packet_handler->getName().c_str(),
		    encap_packet->getTotalLength());
	}

drop:
skip:
	// release buffer (data is now saved in NetPacket objects)
	delete dvb_frame;
	return true;

release_burst:
	delete burst;
error:
	delete dvb_frame;
	return false;
}



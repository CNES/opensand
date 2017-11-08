/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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

#include <opensand_output/Output.h>

#include <cassert>


DvbRcsStd::DvbRcsStd(EncapPlugin::EncapPacketHandler *pkt_hdl):
	PhysicStd("DVB-RCS", pkt_hdl),
	has_fixed_length(true)
{
	this->generic_switch = NULL;

	this->log_rcv_from_down = Output::registerLog(LEVEL_WARNING,
	                                              "Dvb.Upward.receive");
}

DvbRcsStd::DvbRcsStd(string type, bool has_fixed_length,
		EncapPlugin::EncapPacketHandler *pkt_hdl):
	PhysicStd(type, pkt_hdl),
	has_fixed_length(has_fixed_length)
{
	this->generic_switch = NULL;

	this->log_rcv_from_down = Output::registerLog(LEVEL_WARNING,
	                                              "Dvb.Upward.receive");
}

DvbRcs2Std::DvbRcs2Std(EncapPlugin::EncapPacketHandler *pkt_hdl):
	DvbRcsStd("DVB-RCS2", false, pkt_hdl)
{
}

DvbRcsStd::~DvbRcsStd()
{
	if(this->generic_switch != NULL)
	{
		delete this->generic_switch;
	}
}


bool DvbRcsStd::onRcvFrame(DvbFrame *dvb_frame,
                           tal_id_t UNUSED(tal_id),
                           NetBurst **burst)
{
	DvbRcsFrame *dvb_rcs_frame;

	vector<NetPacket *> decap_packets;
	bool partial_decap = false;

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

	// sanity check: this function only handle DVB-RCS frame
	if(dvb_frame->getMessageType() != MSG_TYPE_DVB_BURST)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "the message received is not a DVB burst\n");
		goto error;
	}
	// TODO dvb_frame = static_cast<DvbRcsFrame *>(dvb_frame);
	dvb_rcs_frame = dvb_frame->operator DvbRcsFrame*();

	if(dvb_rcs_frame->isCorrupted())
	{
		// corrupted, nothing more to do

		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "The Frame was corrupted by physical layer, "
		    "drop it\n");
		goto skip;
	}

	if(dvb_rcs_frame->getNumPackets() <= 0)
	{
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "skip DVB-RCS frame with no encapsulation packet\n");
		goto skip;
	}
	if(this->has_fixed_length && this->packet_handler->getFixedLength() == 0)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "encapsulated packets length is not fixed on "
		    "a DVB-RCS emission link (packet type is %s)\n",
		    this->packet_handler->getName().c_str());
		return false;
	}

	LOG(this->log_rcv_from_down, LEVEL_INFO,
	    "%s burst received (%u packet(s))\n",
	    this->packet_handler->getName().c_str(),
	    dvb_rcs_frame->getNumPackets());

	// get encapsulated packets received from lower layer
	if(!this->packet_handler->getEncapsulatedPackets(dvb_rcs_frame,
		partial_decap,
		decap_packets,
		dvb_rcs_frame->getNumPackets()))
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "cannot create one %s packet\n",
		    this->packet_handler->getName().c_str());
		goto error;
	}

	// create an empty burst of encapsulation packets
	*burst = new NetBurst();
	if(*burst == NULL)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "failed to create a burst of packets\n");
		goto error;
	}

	// add packets to the newly created burst
	for(vector<NetPacket *>::iterator it = decap_packets.begin();
		it != decap_packets.end();
		++it)
	{
		// satellite part
		if(this->generic_switch)
		{
			uint8_t spot_id;

			// find the spot ID associated to the packet, it will
			// be used to put the cell in right fifo after the Encap SAT bloc
			spot_id = this->generic_switch->find(*it);
			if(spot_id == 0)
			{
				LOG(this->log_rcv_from_down, LEVEL_ERROR,
				    "unable to find destination spot, drop the "
				    "packet\n");
				delete (*it);
				continue;
			}
			// associate the spot ID to the packet
			(*it)->setSpot(spot_id);
		}

		// add the packet to the burst of packets
		(*burst)->add(*it);
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "%s packet (%zu bytes) added to burst\n",
		    this->packet_handler->getName().c_str(),
		    (*it)->getTotalLength());
	}

skip:
	// release buffer (data is now saved in NetPacket objects)
	delete dvb_frame;
	return true;

error:
	delete dvb_frame;
	return false;
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


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
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "DvbRcsStd.h"

#include <opensand_output/Output.h>

#include <cassert>


DvbRcsStd::DvbRcsStd(EncapPlugin::EncapPacketHandler *pkt_hdl):
	PhysicStd("DVB-RCS", pkt_hdl),
	has_fixed_length(true)
{
	this->log_rcv_from_down = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Upward.receive");
}

DvbRcsStd::DvbRcsStd(std::string type, bool has_fixed_length,
		EncapPlugin::EncapPacketHandler *pkt_hdl):
	PhysicStd(type, pkt_hdl),
	has_fixed_length(has_fixed_length)
{
	this->log_rcv_from_down = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Upward.receive");
}

DvbRcs2Std::DvbRcs2Std(EncapPlugin::EncapPacketHandler *pkt_hdl):
	DvbRcsStd("DVB-RCS2", false, pkt_hdl)
{
}

DvbRcsStd::~DvbRcsStd()
{
}


bool DvbRcsStd::onRcvFrame(DvbFrame *dvb_frame,
                           tal_id_t,
                           NetBurst **burst)
{

	std::vector<std::unique_ptr<NetPacket>> decap_packets;
	bool partial_decap = false;

	// sanity check
	if(dvb_frame == nullptr)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "invalid frame received\n");
		delete dvb_frame;
		return false;
	}

	if(!this->packet_handler)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "packet handler is NULL\n");
		delete dvb_frame;
		return false;
	}

	// sanity check: this function only handle DVB-RCS frame
	if(dvb_frame->getMessageType() != EmulatedMessageType::DvbBurst)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "the message received is not a DVB burst\n");
		delete dvb_frame;
		return false;
	}

	DvbRcsFrame *dvb_rcs_frame_ptr = *dvb_frame;
	std::unique_ptr<DvbRcsFrame> dvb_rcs_frame{dvb_rcs_frame_ptr};

	if(dvb_rcs_frame->isCorrupted())
	{
		// corrupted, nothing more to do

		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "The Frame was corrupted by physical layer, "
		    "drop it\n");
		return true;
	}

	auto packets_count = dvb_rcs_frame->getNumPackets();
	if(packets_count <= 0)
	{
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "skip DVB-RCS frame with no encapsulation packet\n");
		return true;
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
	    packets_count);

	// get encapsulated packets received from lower layer
	if(!this->packet_handler->getEncapsulatedPackets(std::move(dvb_rcs_frame),
	                                                 partial_decap,
	                                                 decap_packets,
	                                                 packets_count))
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "cannot create one %s packet\n",
		    this->packet_handler->getName().c_str());
		return false;
	}

	try
	{
		// create an empty burst of encapsulation packets
		*burst = new NetBurst();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "failed to create a burst of packets\n");
		return false;
	}

	// add packets to the newly created burst
	for (auto&& packet : decap_packets)
	{
		// add the packet to the burst of packets
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "%s packet (%zu bytes) added to burst\n",
		    this->packet_handler->getName().c_str(),
		    packet->getTotalLength());
		(*burst)->add(std::move(packet));
	}

	return true;
}

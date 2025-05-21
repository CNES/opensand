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
 * @file DvbS2Std.cpp
 * @brief DVB-S2 Transmission Standard
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include <algorithm>

#include <opensand_output/Output.h>

#include "DvbS2Std.h"
#include "NetBurst.h"
#include "OpenSandModelConf.h"


DvbS2Std::DvbS2Std(std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl):
	PhysicStd("DVB-S2", pkt_hdl),
	// use maximum MODCOD ID at startup in order to authorize any incoming trafic
	real_modcod(28), // TODO fmt_simu->getmaxFwdModcod()
	received_modcod(this->real_modcod),
	is_scpc(false)
{
	// Output Logs
	this->log_rcv_from_down = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Upward.receive");
}


DvbScpcStd::DvbScpcStd(std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl):
	DvbS2Std("SCPC", pkt_hdl)
{
	this->is_scpc = true;
}


DvbS2Std::DvbS2Std(std::string type,
                   std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl):
	PhysicStd(type, pkt_hdl),
	// use maximum MODCOD ID at startup in order to authorize any incoming trafic
	real_modcod(28), // TODO fmt_simu->getmaxFwdModcod()
	received_modcod(this->real_modcod),
	is_scpc(false)
{
	// Output Logs
	this->log_rcv_from_down = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Upward.receive");
}

DvbS2Std::~DvbS2Std()
{
}


// TODO factorize with DVB-RCS function ?
bool DvbS2Std::onRcvFrame(Rt::Ptr<DvbFrame> dvb_frame,
                          tal_id_t tal_id,
                          Rt::Ptr<NetBurst> &burst)
{
	auto Conf = OpenSandModelConf::Get();
	int real_mod = 0;     // real modcod of the receiver

	std::vector<Rt::Ptr<NetPacket>> decap_packets;
	bool partial_decap = false;

	// sanity check
	if(dvb_frame == nullptr)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "invalid frame received\n");
		return false;
	}

	if(!this->packet_handler)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "packet handler is NULL\n");
		return false;
	}

	// sanity check: this function only handle BB frames
	// keep corrupted for MODCOD updating
	if(dvb_frame->getMessageType() != EmulatedMessageType::BbFrame)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "the message received is not a BB frame\n");
		return false;
	}

	auto bbframe_burst = dvb_frame_upcast<BBFrame>(std::move(dvb_frame));
	auto burst_length = bbframe_burst->getDataLength();
	LOG(this->log_rcv_from_down, LEVEL_INFO,
	    "BB frame received (%d %s packet(s))\n",
	    burst_length,
	    this->packet_handler->getName().c_str());

	// TODO This is not used on GW in SCPC mode as we do not use MODCOD options
	if(!Conf->isGw(tal_id) && !this->is_scpc)
	{
		// retrieve the current real MODCOD of the receiver
		// (do this before any MODCOD update occurs)
		real_mod = this->real_modcod;
	}

	// used for terminal statistics
	this->received_modcod = bbframe_burst->getModcodId();

	if(bbframe_burst->isCorrupted())
	{
		// corrupted, nothing more to do
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "The BBFrame was corrupted by physical layer, drop "
		    "it\n");
		return true;
	}

	// is the ST able to decode the received BB frame ?
	// TODO This is not used on GW in SCPC mode as we do not use MODCOD options
	if(!Conf->isGw(tal_id) && !this->is_scpc &&
	   this->getRequiredEsN0(this->received_modcod) > this->getRequiredEsN0(real_mod))
	{
		// the BB frame is not robust enough to be decoded, drop it
		// TODO Error but the frame is maybe not for this st
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "the terminal is able to decode MODCOD %d (SNR %f), "
		    "the received BB frame is encoded with MODCOD %d (SNR %f) "
		    "that is not robust enough, so emulate a lost BB frame\n",
		    real_mod, this->getRequiredEsN0(real_mod),
		    this->received_modcod, this->getRequiredEsN0(this->received_modcod));
		return true;
	}

	if(burst_length <= 0)
	{
		LOG(this->log_rcv_from_down, LEVEL_INFO,
		    "skip BB frame with no encapsulation packet\n");
		return true;
	}

	// get encapsulated packets received from lower layer
	if(!this->packet_handler->getEncapsulatedPackets(std::move(bbframe_burst),
	                                                 partial_decap,
	                                                 decap_packets,
	                                                 burst_length))
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "cannot create one %s packet\n",
		    this->packet_handler->getName().c_str());
		return false;
	}

	// now we are sure that the BB frame is robust enough and has been
	// decoded, create an empty burst of encapsulation packets to store the
	// encapsulation packets we have extracted from the BB frame
	try
	{
		burst = Rt::make_ptr<NetBurst>();
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
		burst->add(std::move(packet));
	}

	return true;
}

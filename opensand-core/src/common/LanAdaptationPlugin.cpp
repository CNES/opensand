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
 * @file LanAdaptationPlugin.h
 * @brief Generic LAN adaptation plugin
 */

#include <opensand_output/Output.h>

#include "LanAdaptationPlugin.h"
#include "NetBurst.h"
#include "NetContainer.h"
#include "SarpTable.h"
#include "Except.h"


LanAdaptationPlugin::LanAdaptationPlugin(NET_PROTO ether_type):
		StackPlugin(ether_type)
{
}

bool LanAdaptationPlugin::init()
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING,
	                                       "LanAdaptation.%s",
	                                       this->getName().c_str());
	return true;
}


LanAdaptationPlugin::LanAdaptationPacketHandler::LanAdaptationPacketHandler(LanAdaptationPlugin &pl):
		StackPacketHandler(pl)
{
}

bool LanAdaptationPlugin::LanAdaptationPacketHandler::init()
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING,
	                                       "LanAdaptation.%s",
	                                       this->getName().c_str());
	return true;
}


std::size_t LanAdaptationPlugin::LanAdaptationPacketHandler::getMinLength() const
{
	throw NotImplementedError("LanAdaptationPlugin::LanAdaptationPacketHandler::getMinLength");
}


bool LanAdaptationPlugin::LanAdaptationPacketHandler::encapNextPacket(Rt::Ptr<NetPacket>,
                                                                      std::size_t,
                                                                      bool,
                                                                      Rt::Ptr<NetPacket> &,
                                                                      Rt::Ptr<NetPacket> &)
{
	throw NotImplementedError("LanAdaptationPlugin::LanAdaptationPacketHandler::encapNextPacket");
}


bool LanAdaptationPlugin::LanAdaptationPacketHandler::getEncapsulatedPackets(Rt::Ptr<NetContainer>,
                                                                             bool &,
                                                                             std::vector<Rt::Ptr<NetPacket>> &,
                                                                             unsigned int)
{
	throw NotImplementedError("LanAdaptationPlugin::LanAdaptationPacketHandler::getEncapsulatedPackets");
}


LanAdaptationPlugin::LanAdaptationContext::LanAdaptationContext(LanAdaptationPlugin &pl):
		StackContext(pl),
		handle_net_packet(false)
{
}

bool LanAdaptationPlugin::LanAdaptationContext::init()
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING,
	                                       "LanAdaptation.%s",
	                                       this->getName().c_str());
	return true;
}

bool LanAdaptationPlugin::LanAdaptationContext::initLanAdaptationContext(tal_id_t tal_id,
                                                                         std::shared_ptr<PacketSwitch> packet_switch)
{
	this->tal_id = tal_id;
	this->packet_switch = packet_switch;
	return true;
}

bool LanAdaptationPlugin::LanAdaptationContext::setUpperPacketHandler(StackPlugin::StackPacketHandler *pkt_hdl)
{
	if(!pkt_hdl && this->handle_net_packet)
	{
		this->current_upper = nullptr;
		return true;
	}
	return StackPlugin::StackContext::setUpperPacketHandler(pkt_hdl);
}

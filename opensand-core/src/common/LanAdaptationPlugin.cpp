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

#include "LanAdaptationPlugin.h"
#include "NetBurst.h"
#include "NetContainer.h"
#include "SarpTable.h"

#include <opensand_output/Output.h>

#include <cassert>


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
	assert(0);
}


bool LanAdaptationPlugin::LanAdaptationPacketHandler::encapNextPacket(std::unique_ptr<NetPacket>,
                                                                      std::size_t,
                                                                      bool,
                                                                      std::unique_ptr<NetPacket> &,
                                                                      std::unique_ptr<NetPacket> &)
{
	assert(0);
}


bool LanAdaptationPlugin::LanAdaptationPacketHandler::getEncapsulatedPackets(std::unique_ptr<NetContainer>,
                                                                             bool &,
                                                                             std::vector<std::unique_ptr<NetPacket>> &,
                                                                             unsigned int)
{
	assert(0);
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
                                                                         tal_id_t gw_id,
                                                                         PacketSwitch *packet_switch)
{
	this->tal_id = tal_id;
	this->gw_id = gw_id;
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

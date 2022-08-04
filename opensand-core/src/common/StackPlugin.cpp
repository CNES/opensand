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
 * @file StackPlugin.cpp
 * @brief Generic plugin for stack elements
 * @author Mathias ETTINGER <mathias.ettinger@viveris.ffrr>
 */

#include <algorithm>

#include "StackPlugin.h"
#include "NetBurst.h"
#include "NetPacket.h"


StackPlugin::StackPacketHandler::StackPacketHandler(StackPlugin &pl):
	plugin{pl}
{
}


StackPlugin::StackPacketHandler::~StackPacketHandler()
{
}


NET_PROTO StackPlugin::StackPacketHandler::getEtherType() const
{
	return plugin.ether_type;
}


std::string StackPlugin::StackPacketHandler::getName() const
{
	return plugin.name;
}


StackPlugin::StackContext::StackContext(StackPlugin &pl):
	current_upper{nullptr},
	plugin{pl}
{
}


StackPlugin::StackContext::~StackContext()
{
}


NetBurst *StackPlugin::StackContext::encapsulate(NetBurst *burst)
{
	std::map<long, int> time_contexts;
	return this->encapsulate(burst, time_contexts);
}


std::vector<std::string> StackPlugin::StackContext::getAvailableUpperProto() const
{
	return plugin.upper;
}


NET_PROTO StackPlugin::StackContext::getEtherType() const
{
	return plugin.ether_type;
}


bool StackPlugin::StackContext::setUpperPacketHandler(StackPlugin::StackPacketHandler *pkt_hdl)
{
	if (pkt_hdl == nullptr)
	{
		this->current_upper = nullptr;
		return false;
	}

	auto iter = std::find(plugin.upper.begin(), plugin.upper.end(), pkt_hdl->getName());
	this->current_upper = pkt_hdl;
	return iter != plugin.upper.end();
}


void StackPlugin::StackContext::updateStats(unsigned int)
{
}


std::string StackPlugin::StackContext::getName() const
{
	return plugin.name;
}


std::unique_ptr<NetPacket> StackPlugin::StackContext::createPacket(const Data &data,
                                                                   std::size_t data_length,
                                                                   uint8_t qos,
                                                                   uint8_t src_tal_id,
                                                                   uint8_t dst_tal_id)
{
	return plugin.packet_handler->build(data, data_length, qos, src_tal_id, dst_tal_id);
}


StackPlugin::StackPlugin(NET_PROTO ether_type):
	OpenSandPlugin{},
	ether_type{ether_type}
{
}


StackPlugin::~StackPlugin()
{
	delete this->context;
	delete this->packet_handler;
}


StackPlugin::StackContext *StackPlugin::getContext() const
{
	return this->context;
}


StackPlugin::StackPacketHandler *StackPlugin::getPacketHandler() const
{
	return this->packet_handler;
}


std::string StackPlugin::getName() const
{
  return this->name;
}

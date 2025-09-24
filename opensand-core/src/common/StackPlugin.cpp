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


StackPlugin::StackContext::StackContext(StackPlugin &pl): plugin{pl}
{
}


Rt::Ptr<NetBurst> StackPlugin::StackContext::encapsulate(Rt::Ptr<NetBurst> burst)
{
	std::map<long, int> time_contexts;
	return this->encapsulate(std::move(burst), time_contexts);
}


std::vector<std::string> StackPlugin::StackContext::getAvailableUpperProto() const
{
	return plugin.upper;
}


NET_PROTO StackPlugin::StackContext::getEtherType() const
{
	return plugin.ether_type;
}


void StackPlugin::StackContext::updateStats(const time_ms_t &)
{
}


std::string StackPlugin::StackContext::getName() const
{
	return plugin.name;
}


StackPlugin::StackPlugin(NET_PROTO ether_type):
	OpenSandPlugin{},
	ether_type{ether_type}
{
}


std::shared_ptr<StackPlugin::StackContext> StackPlugin::getContext() const
{
	return this->context;
}


std::string StackPlugin::getName() const
{
	return this->name;
}

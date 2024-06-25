/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
 * Copyright © 2019 TAS
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
 * @file SimpleEncapPlugin.cpp
 * @brief Generic encapsulation / deencapsulation plugin
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include <opensand_output/Output.h>

#include "SimpleEncapPlugin.h"
#include "NetBurst.h"
#include "NetContainer.h"
#include "NetPacket.h"

SimpleEncapPlugin::SimpleEncapPlugin(NET_PROTO ether_type, const std::string& name) : OpenSandPlugin(), dst_tal_id(BROADCAST_TAL_ID), ether_type{ether_type}
{
}


std::string SimpleEncapPlugin::getName() const
{
	return this->name;
}


bool SimpleEncapPlugin::init()
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING, "Encap." + this->getName());
	return true;
}




NET_PROTO SimpleEncapPlugin::getEtherType() const
{
	return this->ether_type;
}

void SimpleEncapPlugin::setFilterTalId(uint8_t tal_id)
{
	// TODO LOG(this->log,LEVEL_INFO,"Setting filter tal id to %u",tal_id);
	this->dst_tal_id = tal_id;
}
/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file NetPacket.cpp
 * @brief Network-layer packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "NetPacket.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


mgl_memory_pool NetPacket::mempool(230, 100000, "net_packet");


NetPacket::NetPacket(Data data): _data(data), _name("unknown")
{
	this->_type = 0;
}

NetPacket::NetPacket(unsigned char *data, unsigned int length):
	_data(), _name("unknown")
{
	_data.append(data, length);

	this->_type = 0;
}

NetPacket::NetPacket(): _data(), _name("unknown")
{
	this->_type = 0;
}

NetPacket::~NetPacket()
{
}

void NetPacket::addTrace(std::string name_function)
{
	mempool.add_function(name_function, (char *) this);
}

std::string NetPacket::name()
{
	return this->_name;
}

uint16_t NetPacket::type()
{
	return this->_type;
}

Data NetPacket::data()
{
	if(!this->isValid())
	{
		UTI_ERROR("invalid packet\n");
		return Data();
	}

	return this->_data;
}

void NetPacket::setType(uint16_t type)
{
	return;
}


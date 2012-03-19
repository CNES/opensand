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
 * @file RohcPacket.cpp
 * @brief ROHC packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "RohcPacket.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


RohcPacket::RohcPacket(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "ROHC";
	this->_type = NET_PROTO_ROHC;
	this->_data.reserve(1500);
}

RohcPacket::RohcPacket(Data data): NetPacket(data)
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "ROHC";
	this->_type = NET_PROTO_ROHC;
	this->_data.reserve(1500);
}

RohcPacket::RohcPacket(): NetPacket()
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "ROHC";
	this->_type = NET_PROTO_ROHC;
	this->_data.reserve(1500);
}

RohcPacket::~RohcPacket()
{
}

int RohcPacket::qos()
{
	return this->_qos;
}

void RohcPacket::setQos(int qos)
{
	this->_qos = qos;
}

unsigned long RohcPacket::macId()
{
	return this->_macId;
}

void RohcPacket::setMacId(unsigned long macId)
{
	this->_macId = macId;
}

long RohcPacket::talId()
{
	return this->_talId;
}

void RohcPacket::setTalId(long talId)
{
	this->_talId = talId;
}

bool RohcPacket::isValid()
{
	// always return true, rohc library will test packet validity
	return true;
}

uint16_t RohcPacket::totalLength()
{
	return this->_data.length();
}

uint16_t RohcPacket::payloadLength()
{
	return this->totalLength();
}

Data RohcPacket::payload()
{
	return this->_data;
}

// static
NetPacket * RohcPacket::create(Data data)
{
	return new RohcPacket(data);
}

void RohcPacket::setType(uint16_t type)
{
	this->_type = type;
}

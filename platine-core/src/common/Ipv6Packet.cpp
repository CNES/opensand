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
 * @file Ipv6Packet.cpp
 * @brief IPv6 packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "Ipv6Packet.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


Ipv6Packet::Ipv6Packet(Data data): IpPacket(data)
{
	this->_name = "IPv6";
	this->_type = NET_PROTO_IPV6;
}

Ipv6Packet::Ipv6Packet(unsigned char *data, unsigned int length):
	IpPacket(data, length)
{
	this->_name = "IPv6";
	this->_type = NET_PROTO_IPV6;
}

Ipv6Packet::Ipv6Packet(): IpPacket()
{
	this->_name = "IPv6";
	this->_type = NET_PROTO_IPV6;
}

Ipv6Packet::~Ipv6Packet()
{
}

bool Ipv6Packet::isValid()
{
	return (this->_data.length() >= Ipv6Packet::headerLength());
}

uint16_t Ipv6Packet::totalLength()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::totalLength] invalid IPv6 packet\n");
		return 0;
	}

	return (uint16_t) (this->payloadLength() + Ipv6Packet::headerLength());
}

uint16_t Ipv6Packet::payloadLength()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::payloadLength] invalid IPv6 packet\n");
		return 0;
	}

	return (uint16_t) (((this->_data.at(4) & 0xff) << 8)
	                 + ((this->_data.at(5) & 0xff) << 0));
}

IpAddress * Ipv6Packet::srcAddr()
{
	if(this->_srcAddr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv6Packet::srcAddr] invalid IPv6 packet\n");
			return NULL;
		}

		this->_srcAddr =
			new Ipv6Address(this->_data.at( 8), this->_data.at( 9),
			                this->_data.at(10), this->_data.at(11),
			                this->_data.at(12), this->_data.at(13),
			                this->_data.at(14), this->_data.at(15),
			                this->_data.at(16), this->_data.at(17),
			                this->_data.at(18), this->_data.at(19),
			                this->_data.at(20), this->_data.at(21),
			                this->_data.at(22), this->_data.at(23));
	}

	return this->_srcAddr;
}

IpAddress *Ipv6Packet::destAddr()
{
	if(this->_destAddr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv6Packet::destAddr] invalid IPv6 packet\n");
			return NULL;
		}

		this->_destAddr =
			new Ipv6Address(this->_data.at(24), this->_data.at(25),
			                this->_data.at(26), this->_data.at(27),
			                this->_data.at(28), this->_data.at(29),
			                this->_data.at(30), this->_data.at(31),
			                this->_data.at(32), this->_data.at(33),
			                this->_data.at(34), this->_data.at(35),
			                this->_data.at(36), this->_data.at(37),
			                this->_data.at(38), this->_data.at(39));
	}

	return this->_destAddr;
}

// static
unsigned int Ipv6Packet::headerLength()
{
	return 40;
}

uint8_t Ipv6Packet::trafficClass()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::trafficClass] invalid IPv6 packet\n");
		return 0;
	}

	return (uint8_t) (((this->_data.at(0) & 0x0f) << 4)
	                + ((this->_data.at(1) & 0xf0) >> 4));
}

// static
NetPacket *Ipv6Packet::create(Data data)
{
	return new Ipv6Packet(data);
}

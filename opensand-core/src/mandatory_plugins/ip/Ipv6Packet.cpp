/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file Ipv6Packet.cpp
 * @brief IPv6 packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "Ipv6Packet.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"


Ipv6Packet::Ipv6Packet(Data data): IpPacket(data)
{
	this->name = "IPv6";
	this->type = NET_PROTO_IPV6;
	this->header_length = 40;
}

Ipv6Packet::Ipv6Packet(unsigned char *data, unsigned int length):
	IpPacket(data, length)
{
	this->name = "IPv6";
	this->type = NET_PROTO_IPV6;
	this->header_length = 40;
}

Ipv6Packet::Ipv6Packet(): IpPacket()
{
	this->name = "IPv6";
	this->type = NET_PROTO_IPV6;
	this->header_length = 40;
}

Ipv6Packet::~Ipv6Packet()
{
}

bool Ipv6Packet::isValid()
{
	return (this->data.length() >= Ipv6Packet::headerLength());
}

uint16_t Ipv6Packet::getTotalLength()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::totalLength] invalid IPv6 packet\n");
		return 0;
	}

	return (uint16_t) (this->getPayloadLength() + Ipv6Packet::headerLength());
}

uint16_t Ipv6Packet::getPayloadLength()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::payloadLength] invalid IPv6 packet\n");
		return 0;
	}

	return (uint16_t) (((this->data.at(4) & 0xff) << 8)
	                 + ((this->data.at(5) & 0xff) << 0));
}

IpAddress * Ipv6Packet::srcAddr()
{
	if(this->src_addr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv6Packet::srcAddr] invalid IPv6 packet\n");
			return NULL;
		}

		this->src_addr =
			new Ipv6Address(this->data.at( 8), this->data.at( 9),
			                this->data.at(10), this->data.at(11),
			                this->data.at(12), this->data.at(13),
			                this->data.at(14), this->data.at(15),
			                this->data.at(16), this->data.at(17),
			                this->data.at(18), this->data.at(19),
			                this->data.at(20), this->data.at(21),
			                this->data.at(22), this->data.at(23));
	}

	return this->src_addr;
}

IpAddress *Ipv6Packet::dstAddr()
{
	if(this->dst_addr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv6Packet::dstAddr] invalid IPv6 packet\n");
			return NULL;
		}

		this->dst_addr =
			new Ipv6Address(this->data.at(24), this->data.at(25),
			                this->data.at(26), this->data.at(27),
			                this->data.at(28), this->data.at(29),
			                this->data.at(30), this->data.at(31),
			                this->data.at(32), this->data.at(33),
			                this->data.at(34), this->data.at(35),
			                this->data.at(36), this->data.at(37),
			                this->data.at(38), this->data.at(39));
	}

	return this->dst_addr;
}

// static
unsigned int Ipv6Packet::headerLength()
{
	return 40;
}

uint8_t Ipv6Packet::diffServField()
{
	uint8_t diffServField;

	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::diffServField] invalid IPv6 packet\n");
		return 0;
	}

	diffServField = (uint8_t) (((this->data.at(0) & 0x0f) << 4)
	                + ((this->data.at(1) & 0xf0) >> 4));
	return diffServField;
}

uint8_t Ipv6Packet::diffServCodePoint()
{
	uint8_t diffServCodePoint;

	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::diffServCodePoint] invalid IPv6 packet\n");
		return 0;
	}

	diffServCodePoint = (uint8_t) (((this->data.at(0) & 0x0f) << 4)
	                + ((this->data.at(1) & 0xc0) >> 4));
	return diffServCodePoint;
}

uint8_t Ipv6Packet::explicitCongestionNotification()
{
	uint8_t explicitCongestionNotification;

	if(!this->isValid())
	{
		UTI_ERROR("[Ipv6Packet::explicitCongestionNoticiation] invalid IPv6 packet\n");
		return 0;
	}

	explicitCongestionNotification = (uint8_t) ((this->data.at(1) & 0x03) >> 4);
	return explicitCongestionNotification;
}

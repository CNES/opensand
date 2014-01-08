/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file Ipv4Packet.cpp
 * @brief IPv4 packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "Ipv4Packet.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"


Ipv4Packet::Ipv4Packet(const unsigned char *data,
                       size_t length):
	IpPacket(data, length)
{
	this->name = "IPv4";
	this->type = NET_PROTO_IPV4;

	this->validity_checked = false;
	this->validity_result = false;
	this->header_length = 20;
}

Ipv4Packet::Ipv4Packet(const Data &data): IpPacket(data)
{
	this->name = "IPv4";
	this->type = NET_PROTO_IPV4;

	this->validity_checked = false;
	this->validity_result = false;
	this->header_length = 20;
}

Ipv4Packet::Ipv4Packet(const Data &data,
                       size_t length):
	IpPacket(data, length)
{
	this->name = "IPv4";
	this->type = NET_PROTO_IPV4;

	this->validity_checked = false;
	this->validity_result = false;
	this->header_length = 20;
}

Ipv4Packet::Ipv4Packet(): IpPacket()
{
	this->name = "IPv4";
	this->type = NET_PROTO_IPV4;

	this->validity_checked = false;
	this->validity_result = false;
	this->header_length = 20;
}

Ipv4Packet::~Ipv4Packet()
{
}

bool Ipv4Packet::isValid() const
{
	bool is_valid = false;
	uint16_t crc;
	uint16_t cur_crc;

	if(this->validity_checked)
		goto skip;

	// IPv4 packet length must be at least 20 byte long
	if(this->data.length() < 4 * 5)
	{
		UTI_ERROR("IP packet is to small\n");
		goto invalid;
	}

	// calculate the CRC
	crc = this->calcCrc();
	cur_crc = this->crc();

	if(crc != cur_crc)
	{
		UTI_ERROR("[Ipv4Packet::isValid] CRC = %08x, should be %08x\n",
		          cur_crc, crc);
		goto invalid;
	}

	is_valid = true;

invalid:
	// cache the result
	this->validity_result = is_valid;
	this->validity_checked = true;

skip:
	return this->validity_result;
}

uint16_t Ipv4Packet::calcCrc() const
{
	size_t nbytes;
	uint16_t *data;
	uint32_t sum;

	data = (uint16_t *) this->data.c_str();
	nbytes = this->ihl() * 4;
	sum = 0;

	while(nbytes >= sizeof(uint16_t))
	{
		sum += *(data++);
		nbytes -= sizeof(uint16_t);
	}
	// Header Checksum field must be 0 for checksum,
	sum -= this->crc();

	sum = (sum >> 16) + (sum & 0xffff);
	sum = (sum >> 16) + (sum & 0xffff);

	return ((uint16_t) ~ sum);
}

uint16_t Ipv4Packet::crc() const
{
	if(this->data.length() < 4 * 5)
	{
		UTI_ERROR("[Ipv4Packet::crc] invalid IPv4 packet\n");
		return 0;
	}

	return (uint16_t) (((this->data.at(10) & 0xff) << 8)
	                 + ((this->data.at(11) & 0xff) << 0));
}

size_t Ipv4Packet::getTotalLength() const
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::totalLength] invalid IPv4 packet\n");
		return 0;
	}

	return (((this->data.at(2) & 0xff) << 8)
	        + ((this->data.at(3) & 0xff) << 0));
}

uint8_t Ipv4Packet::ihl() const
{
	if(this->data.length() < 4 * 5)
	{
		UTI_ERROR("[Ipv4Packet::ihl] invalid IPv4 packet\n");
		return 0;
	}

	return (uint8_t) (this->data.at(0) & 0x0f);
}

size_t Ipv4Packet::getPayloadLength() const
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::payloadLength] invalid IPv4 packet\n");
		return 0;
	}

	return (this->getTotalLength() - this->ihl() * 4);
}

IpAddress *Ipv4Packet::srcAddr()
{
	if(this->src_addr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv4Packet::srcAddr] invalid IPv4 packet\n");
			return NULL;
		}

		this->src_addr =
			new Ipv4Address(this->data.at(12), this->data.at(13),
			                this->data.at(14), this->data.at(15));
	}

	return this->src_addr;
}

IpAddress *Ipv4Packet::dstAddr()
{
	if(this->dst_addr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv4Packet::dstAddr] invalid IPv4 packet\n");
			return NULL;
		}

		this->dst_addr =
			new Ipv4Address(this->data.at(16), this->data.at(17),
			                this->data.at(18), this->data.at(19));
	}

	return this->dst_addr;
}

uint8_t Ipv4Packet::diffServField() const
{
	uint8_t diffServField;
	
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::diffServField] invalid IPv4 packet\n");
		return 0;
	}

	diffServField = (uint8_t) this->data.at(1);
	return diffServField;
}

uint8_t Ipv4Packet::diffServCodePoint() const
{
	uint8_t diffServCodePoint;

	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::diffServCodePoint] invalid IPv4 packet\n");
		return 0;
	}

	diffServCodePoint = (uint8_t) (this->data.at(1) & 0xfc);
	return diffServCodePoint;
}

uint8_t Ipv4Packet::explicitCongestionNotification() const
{
	uint8_t explicitCongestionNotification;

	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::explicitCongestionNotification] invalid IPv4 packet\n");
		return 0;
	}

	explicitCongestionNotification = (uint8_t) (this->data.at(1) & 0x03);
	return explicitCongestionNotification;
}

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
 * @file IpPacket.cpp
 * @brief Generic IP packet, either IPv4 or IPv6
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "IpPacket.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"


IpPacket::IpPacket(Data data): NetPacket(data)
{
	this->data.reserve(1500);

	this->src_addr = NULL;
	this->dst_addr = NULL;
}

IpPacket::IpPacket(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->data.reserve(1500);

	this->src_addr = NULL;
	this->dst_addr = NULL;
}

IpPacket::IpPacket(): NetPacket()
{
	this->data.reserve(1500);

	this->src_addr = NULL;
	this->dst_addr = NULL;
}

IpPacket::~IpPacket()
{
	if(this->src_addr != NULL)
		delete this->src_addr;
	if(this->dst_addr != NULL)
		delete this->dst_addr;
}

void IpPacket::setQos(int qos)
{
	this->qos = qos;
}

void IpPacket::setSrcTalId(long tal_id)
{
	this->src_tal_id = tal_id;
}

void IpPacket::setDstTalId(long tal_id)
{
	this->dst_tal_id = tal_id;
}

Data IpPacket::getPayload()
{
	uint16_t payload_len, header_len;

	if(!this->isValid())
	{
		UTI_ERROR("[IpPacket::payload] invalid IP packet\n");
		return Data();
	}

	payload_len = this->getPayloadLength();
	header_len = this->getTotalLength() - payload_len;

	if(header_len <= 0 || payload_len <= 0)
	{
		UTI_ERROR("[IpPacket::payload] IP packet has a 0 length payload\n");
		return Data();
	}

	return this->data.substr(header_len, payload_len);
}

// static
int IpPacket::version(Data data)
{
	if(data.length() < 4 * 5)
	{
		UTI_ERROR("[IpPacket::version(data)] invalid IP packet\n");
		return 0;
	}

	return ((data.at(0) & 0xf0) >> 4);
}

// static
int IpPacket::version(unsigned char *data, unsigned int length)
{
	if(length < 4 * 5)
	{
		UTI_ERROR("[IpPacket::version(data, length)] invalid IP packet\n");
		return 0;
	}

	return ((data[0] & 0xf0) >> 4);
}

int IpPacket::version()
{
	if(!this->isValid())
	{
		UTI_ERROR("[IpPacket::version] invalid IP packet\n");
		return 0;
	}

	return IpPacket::version(this->data);
}

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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

#include <opensand_output/Output.h>

OutputLog *IpPacket::ip_log = NULL;

IpPacket::IpPacket(const unsigned char *data, size_t length):
	NetPacket(data, length)
{
	this->data.reserve(1500);

	this->src_addr = NULL;
	this->dst_addr = NULL;
}


IpPacket::IpPacket(const Data &data): NetPacket(data)
{
	this->data.reserve(1500);

	this->src_addr = NULL;
	this->dst_addr = NULL;
}

IpPacket::IpPacket(const Data &data, size_t length):
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

Data IpPacket::getPayload() const
{
	size_t payload_len, header_len;

	if(!this->isValid())
	{
		LOG(ip_log, LEVEL_ERROR,
		    "invalid IP packet\n");
		return Data();
	}

	payload_len = this->getPayloadLength();
	header_len = this->getTotalLength() - payload_len;

	if(header_len <= 0 || payload_len <= 0)
	{
		LOG(ip_log, LEVEL_ERROR,
		    "IP packet has a 0 length payload\n");
		return Data();
	}

	return this->data.substr(header_len, payload_len);
}

// static
int IpPacket::version(Data data)
{
	if(data.length() < 4 * 5)
	{
		LOG(ip_log, LEVEL_ERROR,
		    "invalid IP packet\n");
		return 0;
	}

	return ((data.at(0) & 0xf0) >> 4);
}

// static
/*int IpPacket::version(const unsigned char *data, unsigned int length)
{
	if(length < 4 * 5)
	{
		LOG(ip_log, LEVEL_ERROR,
		    "invalid IP packet\n");
		return 0;
	}

	return ((data[0] & 0xf0) >> 4);
}*/

int IpPacket::version() const
{
	if(!this->isValid())
	{
		LOG(ip_log, LEVEL_ERROR,
		    "invalid IP packet\n");
		return 0;
	}

	return IpPacket::version(this->data);
}

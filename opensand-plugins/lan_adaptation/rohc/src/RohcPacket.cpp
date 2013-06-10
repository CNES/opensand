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
 * @file RohcPacket.cpp
 * @brief ROHC packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "RohcPacket.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"


RohcPacket::RohcPacket(unsigned char *data, unsigned int length, uint16_t type):
	NetPacket(data, length)
{
	this->name = "ROHC";
	this->type = type;
	this->data.reserve(1500);
}

RohcPacket::RohcPacket(Data data, uint16_t type): NetPacket(data)
{
	this->name = "ROHC";
	this->type = type;
	this->data.reserve(1500);
}

RohcPacket::RohcPacket(uint16_t type): NetPacket()
{
	this->name = "ROHC";
	this->type = type;
	this->data.reserve(1500);
}

RohcPacket::~RohcPacket()
{
}

uint8_t RohcPacket::getQos()
{
	return this->qos;
}

uint8_t RohcPacket::getSrcTalId()
{
	return this->src_tal_id;
}

uint8_t RohcPacket::getDstTalId()
{
	return this->dst_tal_id;
}

uint16_t RohcPacket::getTotalLength()
{
	return this->data.length();
}

uint16_t RohcPacket::getPayloadLength()
{
	return this->getTotalLength();
}

Data RohcPacket::getPayload()
{
	return this->data;
}

void RohcPacket::setSrcTalId(uint8_t tal_id)
{
	this->src_tal_id = tal_id;
}

void RohcPacket::setDstTalId(uint8_t tal_id)
{
	this->dst_tal_id = tal_id;
}

void RohcPacket::setQos(uint8_t qos)
{
	this->qos = qos;
}


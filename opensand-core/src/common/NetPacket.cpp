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
 * @file NetPacket.cpp
 * @brief Network-layer packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "NetPacket.h"


NetPacket::NetPacket(const unsigned char *data, std::size_t length):
		NetContainer{data, length},
		type{NET_PROTO::ERROR},
		qos{},
		src_tal_id{},
		dst_tal_id{}
{
	this->name = "NetPacket";
}


NetPacket::NetPacket(const Rt::Data &data):
		NetContainer{data},
		type{NET_PROTO::ERROR},
		qos{},
		src_tal_id{},
		dst_tal_id{}
{
	this->name = "NetPacket";
}


NetPacket::NetPacket(const Rt::Data &data, std::size_t length):
		NetContainer{data, length},
		type{NET_PROTO::ERROR},
		qos{},
		src_tal_id{},
		dst_tal_id{}
{
	this->name = "NetPacket";
}


NetPacket::NetPacket(const NetPacket &pkt):
		NetContainer{pkt.getData(), pkt.getTotalLength()},
		type{pkt.getType()},
		qos{pkt.getQos()},
		src_tal_id{pkt.getSrcTalId()},
		dst_tal_id{pkt.getDstTalId()}
{
	this->name = pkt.getName();
	this->spot = pkt.getSpot();
}


NetPacket::NetPacket():
		NetContainer{},
		type{NET_PROTO::ERROR},
		qos{},
		src_tal_id{},
		dst_tal_id{}
{
	this->name = "NetPacket";
}


NetPacket::NetPacket(const Rt::Data &data,
                     std::size_t length,
                     std::string name,
                     NET_PROTO type,
                     uint8_t qos,
                     uint8_t src_tal_id,
                     uint8_t dst_tal_id,
                     std::size_t header_length):
	NetContainer{data, length},
	type{type},
	qos{qos},
	src_tal_id{src_tal_id},
	dst_tal_id{dst_tal_id}
{
	this->name = name;
	this->header_length = header_length;
}


NetPacket::~NetPacket()
{
}


NET_PROTO NetPacket::getType() const
{
	return this->type;
}


void NetPacket::setQos(uint8_t qos)
{
	this->qos = qos;
}


uint8_t NetPacket::getQos() const
{
	return this->qos;
}


void NetPacket::setSrcTalId(uint8_t tal_id)
{
	this->src_tal_id = tal_id;
}


uint8_t NetPacket::getSrcTalId() const
{
	return this->src_tal_id;
}


void NetPacket::setDstTalId(uint8_t tal_id)
{
	this->dst_tal_id = tal_id;
}


uint8_t NetPacket::getDstTalId() const
{
	return this->dst_tal_id;
}

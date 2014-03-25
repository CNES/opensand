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
 * @file MpegPacket.cpp
 * @brief MPEG-2 TS packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "MpegPacket.h"

#include <opensand_output/Output.h>


MpegPacket::MpegPacket(const unsigned char *data, size_t length):
	NetPacket(data, length)
{
	this->name = "MPEG2-TS";
	this->type = NET_PROTO_MPEG;
	this->data.reserve(TS_PACKETSIZE);
	this->header_length = TS_HEADERSIZE;
	this->src_tal_id = this->getSrcTalId();
	this->dst_tal_id = this->getDstTalId();
	this->qos = this->getQos();
}

MpegPacket::MpegPacket(const Data &data): NetPacket(data)
{
	this->name = "MPEG2-TS";
	this->type = NET_PROTO_MPEG;
	this->data.reserve(TS_PACKETSIZE);
	this->header_length = TS_HEADERSIZE;
	this->src_tal_id = this->getSrcTalId();
	this->dst_tal_id = this->getDstTalId();
	this->qos = this->getQos();
}

MpegPacket::MpegPacket(const Data &data, size_t length):
	NetPacket(data, length)
{
	this->name = "MPEG2-TS";
	this->type = NET_PROTO_MPEG;
	this->data.reserve(TS_PACKETSIZE);
	this->header_length = TS_HEADERSIZE;
	this->src_tal_id = this->getSrcTalId();
	this->dst_tal_id = this->getDstTalId();
	this->qos = this->getQos();
}

MpegPacket::MpegPacket(): NetPacket()
{
	this->name = "MPEG2-TS";
	this->type = NET_PROTO_MPEG;
	this->data.reserve(TS_PACKETSIZE);
	this->header_length = TS_HEADERSIZE;
}

MpegPacket::~MpegPacket()
{
}

uint8_t MpegPacket::getQos()
{
	return (this->getPid() & 0x07);
}

uint8_t MpegPacket::getSrcTalId()
{
	return (this->getPid() >> 3) & 0x1F;
}

uint8_t MpegPacket::getDstTalId()
{
	return (this->getPid() >> 8) & 0x1F;
}

bool MpegPacket::isValid() const
{
	/* check length */
	if(this->getTotalLength() != TS_PACKETSIZE)
	{
		DFLTLOG(LEVEL_ERROR,
		        "bad length (%zu bytes)\n",
		        this->getTotalLength());
		goto bad;
	}

	/* check Synchonization byte */
	if(this->sync() != 0x47)
	{
		DFLTLOG(LEVEL_ERROR,
		        "bad sync byte (0x%02x)\n",
		        this->sync());
		goto bad;
	}

	/* check the Transport Error Indicator (TEI) bit */
	if(this->tei())
	{
		DFLTLOG(LEVEL_ERROR,
		        "TEI is on\n");
		goto bad;
	}

	/* check Transport Scrambling Control (TSC) bits */
	if(this->tsc() != 0)
	{
		DFLTLOG(LEVEL_ERROR,
		        "TSC is on\n");
		goto bad;
	}

	/* check Payload Pointer validity (if present) */
	if(this->pusi() && this->pp() >= (TS_DATASIZE - 1))
	{
		DFLTLOG(LEVEL_ERROR,
		        "bad payload pointer (PUSI set and PP = 0x%02x)\n",
		        this->pp());
		goto bad;
	}

	return true;

bad:
	return false;
}

uint8_t MpegPacket::sync() const
{
	return this->data.at(0) & 0xff;
}

bool MpegPacket::tei() const
{
	return (this->data.at(1) & 0x80) != 0;
}

bool MpegPacket::pusi() const
{
	return (this->data.at(1) & 0x40) != 0;
}

bool MpegPacket::tp() const
{
	return (this->data.at(1) & 0x20) != 0;
}

uint16_t MpegPacket::getPid() const
{
	return (uint16_t) (((this->data.at(1) & 0x1f) << 8) +
	                   ((this->data.at(2) & 0xff) << 0));
}

uint8_t MpegPacket::tsc() const
{
	return (this->data.at(3) & 0xC0);
}

uint8_t MpegPacket::cc() const
{
	return (this->data.at(3) & 0x0f);
}

uint8_t MpegPacket::pp() const
{
	return (this->data.at(4) & 0xff);
}

// static
uint16_t MpegPacket::getPidFromPacket(NetPacket *packet)
{
	uint8_t qos = packet->getQos();
	uint8_t src_tal_id = packet->getSrcTalId();
	uint8_t dst_tal_id = packet->getDstTalId();

	uint16_t pid = 0;

	return pid | ((dst_tal_id & 0x1F) << 8)
	           | ((src_tal_id & 0x1F) << 3)
	           | (qos & 0x07);
}

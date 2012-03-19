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
 * @file MpegPacket.cpp
 * @brief MPEG-2 TS packet
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "MpegPacket.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


MpegPacket::MpegPacket(unsigned char *data, unsigned int length):
                       NetPacket(data, length)
{
	this->_name = "MPEG2-TS";
	this->_type = NET_PROTO_MPEG;
	this->_data.reserve(TS_PACKETSIZE);
}

MpegPacket::MpegPacket(Data data): NetPacket(data)
{
	this->_name = "MPEG2-TS";
	this->_type = NET_PROTO_MPEG;
	this->_data.reserve(TS_PACKETSIZE);
}

MpegPacket::MpegPacket(): NetPacket()
{
	this->_name = "MPEG2-TS";
	this->_type = NET_PROTO_MPEG;
	this->_data.reserve(TS_PACKETSIZE);
}

MpegPacket::~MpegPacket()
{
}

int MpegPacket::qos()
{
	return (this->pid() & 0x07);
}

void MpegPacket::setQos(int qos)
{
	uint16_t pid;

	if((qos & 0x07) != qos)
	{
		UTI_ERROR("Be careful, you have set a QoS priority greater than 7, "
		          "this can not stand in 3 bits of MPEG2-TS packet !!!\n");
	}

	pid = this->pid();
	pid &= ~0x07;
	pid |= qos & 0x07;

	this->setPid(pid);
}

unsigned long MpegPacket::macId()
{
	return ((this->pid() >> 6) & 0x7f);
}

void MpegPacket::setMacId(unsigned long macId)
{
	uint16_t pid;

	if((macId & 0x7f) != macId)
	{
		UTI_ERROR("Be careful, you have set a MAC ID greater than 0x7f, "
		          "this can not stand in 7 bits of PID field of MPEG2-TS "
		          "packet !!!\n");
	}

	pid = this->pid();
	pid &= ~(0x7f << 6);
	pid |= (macId & 0x7f) << 6;

	this->setPid(pid);
}

long MpegPacket::talId()
{
	return (this->pid() >> 3) & 0x07;
}

void MpegPacket::setTalId(long talId)
{
	uint16_t pid;

	if((talId & 0x07) != talId)
	{
		UTI_ERROR("Be careful, you have set a TAL ID greater than 7, "
		          "this can not stand in 3 bits of MPEG2-TS packet !!!\n");
	}

	pid = this->pid();
	pid &= ~(0x07 << 3);
	pid |= (talId & 0x07) << 3;

	this->setPid(pid);
}

bool MpegPacket::isValid()
{
	const char FUNCNAME[] = "[MpegPacket::isValid]";

	/* check length */
	if(this->totalLength() != TS_PACKETSIZE)
	{
		UTI_ERROR("%s bad length (%d bytes)\n",
		          FUNCNAME, this->totalLength());
		goto bad;
	}

	/* check Synchonization byte */
	if(this->sync() != 0x47)
	{
		UTI_ERROR("%s bad sync byte (0x%02x)\n",
		          FUNCNAME, this->sync());
		goto bad;
	}

	/* check the Transport Error Indicator (TEI) bit */
	if(this->tei())
	{
		UTI_ERROR("%s TEI is on\n", FUNCNAME);
		goto bad;
	}

	/* check Transport Scrambling Control (TSC) bits */
	if(this->tsc() != 0)
	{
		UTI_ERROR("%s TSC is on\n", FUNCNAME);
		goto bad;
	}

	/* check Payload Pointer validity (if present) */
	if(this->pusi() && this->pp() >= (TS_DATASIZE - 1))
	{
		UTI_ERROR("%s bad payload pointer (PUSI set and PP = 0x%02x)\n",
		          FUNCNAME, this->pp());
		goto bad;
	}

	return true;

bad:
	return false;
}

uint16_t MpegPacket::totalLength()
{
	return this->_data.length();
}

uint16_t MpegPacket::payloadLength()
{
	return (this->totalLength() - TS_HEADERSIZE);
}

Data MpegPacket::payload()
{
	return Data(this->_data, TS_HEADERSIZE,
	            this->payloadLength());
}

uint8_t MpegPacket::sync()
{
	return this->_data.at(0) & 0xff;
}

bool MpegPacket::tei()
{
	return (this->_data.at(1) & 0x80) != 0;
}

bool MpegPacket::pusi()
{
	return (this->_data.at(1) & 0x40) != 0;
}

bool MpegPacket::tp()
{
	return (this->_data.at(1) & 0x20) != 0;
}

uint16_t MpegPacket::pid()
{
	return (uint16_t) (((this->_data.at(1) & 0x1f) << 8) +
	                   ((this->_data.at(2) & 0xff) << 0));
}

void MpegPacket::setPid(uint16_t pid)
{
	this->_data.at(1) = (this->_data.at(1) & (~0x1f)) |
	                    ((pid >> 8) & 0x1f);
	this->_data.at(2) = pid & 0xff;
}

uint8_t MpegPacket::tsc()
{
	return (this->_data.at(3) & 0xC0);
}

uint8_t MpegPacket::cc()
{
	return (this->_data.at(3) & 0x0f);
}

uint8_t MpegPacket::pp()
{
	return (this->_data.at(4) & 0xff);
}

// static
unsigned int MpegPacket::length()
{
	return TS_PACKETSIZE;
}

// static
NetPacket * MpegPacket::create(Data data)
{
	return new MpegPacket(data);
}

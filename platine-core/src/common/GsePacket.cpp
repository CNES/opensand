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
 * @file GsePacket.cpp
 * @brief GSE packet
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "GsePacket.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"

/// the length of a GSE header in a not fragmented packet
#define GSE_HDR_LENGTH_NO_REFRAG 10

GsePacket::GsePacket(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "GSE";
	this->_type = NET_PROTO_GSE;
	this->_data.reserve(4096);
}

GsePacket::GsePacket(Data data): NetPacket(data)
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "GSE";
	this->_type = NET_PROTO_GSE;
	this->_data.reserve(4096);
}

GsePacket::GsePacket(): NetPacket()
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "GSE";
	this->_type = NET_PROTO_GSE;
	this->_data.reserve(4096);
}

GsePacket::~GsePacket()
{
}

int GsePacket::qos()
{
	return this->_qos;
}

void GsePacket::setQos(int qos)
{
	this->_qos = qos;
}

unsigned long GsePacket::macId()
{
	return this->_macId;
}

void GsePacket::setMacId(unsigned long macId)
{
	this->_macId = macId;
}

long GsePacket::talId()
{
	gse_status_t status;
	uint8_t label[6];
	long tal_id;

	if(this->_talId != -1)
	{
		return this->_talId;
	}
	// TODO check that packet can contain the tal_id else error: tal_id was not initialized !
	status = gse_get_label((unsigned char *)(this->_data.c_str()), label);
	if(status == GSE_STATUS_OK)
	{
		tal_id = ((label[1] & 0x1f) << 8) | (label[2] & 0xff);
		return tal_id;
	}
	else
		return -1;
}

void GsePacket::setTalId(long talId)
{
	this->_talId = talId;
}

bool GsePacket::isValid()
{
	// always return true, GSE library will test
	// packet validity when necessary
	return true;
}

uint16_t GsePacket::totalLength()
{
	return this->_data.length();
}

uint16_t GsePacket::payloadLength()
{
	return this->totalLength() - GSE_HDR_LENGTH_NO_REFRAG;
	// TODO: the header size could be variable (in the case of a refragmentation)
	// so we have to implement a function returning the right size
}

Data GsePacket::payload()
{
	return this->_data;
}

// static
NetPacket *GsePacket::create(Data data)
{
	return new GsePacket(data);
}

// static
uint16_t GsePacket::length(unsigned char *data, unsigned int offset)
{
	uint16_t length;

	gse_get_gse_length(data + offset, &length);
	// Add 2 bits for S, E and LT fields
	length += 2;
	return length;
}

uint8_t GsePacket::start_indicator()
{
	uint8_t s;

	gse_get_start_indicator((unsigned char *)(this->_data.c_str()), &s);
	return s;
}

uint8_t GsePacket::end_indicator()
{
	uint8_t e;

	gse_get_end_indicator((unsigned char *)(this->_data.c_str()), &e);
	return e;
}

uint8_t GsePacket::fragId()
{
	uint8_t frag_id;

	gse_get_frag_id((unsigned char *)(this->_data.c_str()), &frag_id);
	return frag_id;
}

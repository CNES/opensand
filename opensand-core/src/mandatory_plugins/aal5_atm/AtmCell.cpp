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
 * @file AtmCell.cpp
 * @brief ATM cell
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "AtmCell.h"

#include <opensand_output/Output.h>


AtmCell::AtmCell(const unsigned char *data, size_t length):
	NetPacket(data, length)
{
	this->name = "ATM";
	this->type = NET_PROTO_ATM;
	this->data.reserve(53);
	this->header_length = 5;
}

AtmCell::AtmCell(const Data &data): NetPacket(data)
{
	this->name = "ATM";
	this->type = NET_PROTO_ATM;
	this->data.reserve(53);
	this->header_length = 5;
}

AtmCell::AtmCell(const Data &data, size_t length):
	NetPacket(data, length)
{
	this->name = "ATM";
	this->type = NET_PROTO_ATM;
	this->data.reserve(53);
	this->header_length = 5;
}

AtmCell::AtmCell(): NetPacket()
{
	this->name = "ATM";
	this->type = NET_PROTO_ATM;
	this->data.reserve(53);
	this->header_length = 5;
}

AtmCell::~AtmCell()
{
}

uint8_t AtmCell::getQos()
{
	return (this->getVci() & 0x07);
}

uint8_t AtmCell::getSrcTalId()
{
	return ((this->getVci() >> 3) & 0x1F);
}

uint8_t AtmCell::getDstTalId()
{
	return (this->getVpi() & 0x1F);
}

bool AtmCell::isValid()
{
	bool is_ok = true;

	if(this->getTotalLength() != AtmCell::getLength())
	{
		DFLTLOG(LEVEL_WARNING,
		        "total length (%u) != ATM cell length (%u)\n",
		        this->getTotalLength(), AtmCell::getLength());
		is_ok = false;
	}

	return is_ok;
}

uint16_t AtmCell::getTotalLength()
{
	return this->data.length();
}

uint16_t AtmCell::getPayloadLength()
{
	return this->getTotalLength() - this->header_length;
}

Data AtmCell::getPayload()
{
	if(!this->isValid())
	{
		DFLTLOG(LEVEL_ERROR,
		        "invalid ATM cell\n");
		return Data();
	}

	return Data(this->data, 5, this->getPayloadLength());
}

// UNI VPI field (8 bits)
uint8_t AtmCell::getVpi()
{
	if(!this->isValid())
	{
		DFLTLOG(LEVEL_ERROR,
		        "invalid ATM cell\n");
		return 0;
	}

	return (uint8_t) (((this->data.at(0) & 0x0f) << 4)
	                + ((this->data.at(1) & 0xf0) >> 4));
}

// VCI field (16 bits)
uint16_t AtmCell::getVci()
{
	if(!this->isValid())
	{
		DFLTLOG(LEVEL_ERROR,
		        "invalid ATM cell\n");
		return 0;
	}

	return (uint16_t) (((this->data.at(1) & 0x0f) << 12)
	                 + ((this->data.at(2) & 0xff) << 4)
	                 + ((this->data.at(3) & 0xf0) >> 4));
}

// PTI (3 bits)
uint8_t AtmCell::getPt()
{
	if(!this->isValid())
	{
		DFLTLOG(LEVEL_ERROR,
		        "invalid ATM cell\n");
		return 0;
	}

	return (uint8_t) ((this->data.at(3) & 0x0e) >> 1);
}

// is this ATM cell the last one of the AAL5 frame ?
bool AtmCell::isLastCell()
{
	return (this->getPt() & 0x01);
}

void AtmCell::setGfc(uint8_t gfc)
{
	this->data.replace(0, 1, 1,
		((gfc << 4) & 0xf0) + (this->data.at(0) & 0x0f));
}

void AtmCell::setVpi(uint8_t vpi)
{
	this->data.replace(0, 1, 1,
		(this->data.at(0) & 0xf0) + ((vpi >> 4) & 0x0f));
	this->data.replace(1, 1, 1,
		((vpi << 4) & 0xf0) + (this->data.at(1) & 0x0f));
}

void AtmCell::setVci(uint16_t vci)
{
	this->data.replace(1, 1, 1,
		(this->data.at(1) & 0xf0) + ((vci >> 12) & 0x0f));
	this->data.replace(2, 1, 1, (vci >> 4) & 0xff);
	this->data.replace(3, 1, 1,
		((vci << 4) & 0xf0) + (this->data.at(3) & 0x0f));
}

void AtmCell::setPt(uint8_t pt)
{
	this->data.replace(3, 1, 1,
		(this->data.at(3) & 0xf1) + ((pt << 1) & 0x0e));
}

void AtmCell::setClp(uint8_t clp)
{
	this->data.replace(3, 1, 1,
		(this->data.at(3) & 0xfe) + (clp & 0x01));
}

void AtmCell::setIsLastCell(bool is_last_cell)
{
	uint8_t mask = is_last_cell ? 0x01 : 0x00;

	this->setPt((this->getPt() & 0xfe) | mask);
}

// static
AtmCell *AtmCell::create(uint8_t gfc, uint8_t vpi, uint16_t vci,
                         uint8_t pt, uint8_t clp, bool is_last_cell,
                         Data payload)
{
	AtmCell *atm_cell = NULL;

	Data data;
	data.resize(data.length() + 5, '\0'); // 5 bytes for ATM header
	data.append(payload); // add the 48 byte payload

	atm_cell = new AtmCell(data);

	if(atm_cell == NULL)
		return NULL;

	atm_cell->setGfc(gfc);
	atm_cell->setVpi(vpi);
	atm_cell->setVci(vci);
	atm_cell->setPt(pt);
	atm_cell->setIsLastCell(is_last_cell);
	atm_cell->setClp(clp);
// TODO: ATM checksum ?
// atm_cell->setHec();

	if(!atm_cell->isValid())
	{
		delete atm_cell;
		return NULL;
	}

	return atm_cell;
}

// static
unsigned int AtmCell::getLength()
{
	return 53;
}


// static
uint16_t AtmCell::getVci(NetPacket *packet)
{
	uint8_t qos = packet->getQos();
	uint8_t src_tal_id = packet->getSrcTalId();
	uint16_t vci = 0;

	return vci | (src_tal_id & 0x1F) << 3 | (qos & 0x07);
}

uint8_t AtmCell::getVpi(NetPacket *packet)
{
	uint8_t dst_tal_id = packet->getDstTalId();
	uint8_t vpi = 0;

	return vpi | (dst_tal_id & 0x1F);
}

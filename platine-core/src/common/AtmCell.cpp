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
 * @file AtmCell.cpp
 * @brief ATM cell
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "AtmCell.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


AtmCell::AtmCell(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->_name = "ATM";
	this->_type = NET_PROTO_ATM;
	this->_data.reserve(53);
}

AtmCell::AtmCell(Data data): NetPacket(data)
{
	this->_name = "ATM";
	this->_type = NET_PROTO_ATM;
	this->_data.reserve(53);
}

AtmCell::AtmCell(): NetPacket()
{
	this->_name = "ATM";
	this->_type = NET_PROTO_ATM;
	this->_data.reserve(53);
}

AtmCell::~AtmCell()
{
}

int AtmCell::qos()
{
	return (this->vci() & 0x07);
}

void AtmCell::setQos(int qos)
{
	uint16_t vci;

	if((qos & 0x07) != qos)
	{
		UTI_ERROR("Be careful, you have set a QoS priority greater than 7, "
		           "this can not stand in 3 bits of VCI of ATM cell !!!\n");
	}

	vci = this->vci();
	vci &= ~0x07;
	vci |= qos & 0x07;

	this->setVci(vci);
}

unsigned long AtmCell::macId()
{
	return this->vpi();
}

void AtmCell::setMacId(unsigned long macId)
{
	if((macId & 0xff) != macId)
	{
		UTI_ERROR("Be careful, you have set a MAC ID greater than 0xff, "
		          "this can not stand in 8-bit VPI field of ATM cell !!!\n");
	}

	this->setVpi(macId);
}

long AtmCell::talId()
{
	return ((this->vci() >> 3) & 0x1fff);
}

void AtmCell::setTalId(long talId)
{
	uint16_t vci;

	vci = this->vci();
	vci &= ~(0x1fff << 3);
	vci |= (talId & 0x1fff) << 3;

	if((talId & 0x1fff) != talId)
	{
		UTI_ERROR("Be careful, you have set a TAL ID greater than 0x1fff, "
		          "this can not stand in 13 bits of VCI field of ATM cell !!!\n");
	}

	this->setVci(vci);
}

bool AtmCell::isValid()
{
	bool is_ok = true;

	if(this->totalLength() != AtmCell::length())
	{
		UTI_NOTICE("total length (%u) != ATM cell length (%u)\n",
		           this->totalLength(), AtmCell::length());
		is_ok = false;
	}

	return is_ok;
}

uint16_t AtmCell::totalLength()
{
	return this->_data.length();
}

uint16_t AtmCell::payloadLength()
{
	return this->totalLength() - 5;
}

Data AtmCell::payload()
{
	if(!this->isValid())
	{
		UTI_ERROR("invalid ATM cell\n");
		return Data();
	}

	return Data(this->_data, 5, this->payloadLength());
}

// UNI VPI field (8 bits)
uint8_t AtmCell::vpi()
{
	if(!this->isValid())
	{
		UTI_ERROR("[AtmCell::vpi] invalid ATM cell\n");
		return 0;
	}

	return (uint8_t) (((this->_data.at(0) & 0x0f) << 4)
	                + ((this->_data.at(1) & 0xf0) >> 4));
}

// VCI field (16 bits)
uint16_t AtmCell::vci()
{
	if(!this->isValid())
	{
		UTI_ERROR("[AtmCell::vci] invalid ATM cell\n");
		return 0;
	}

	return (uint16_t) (((this->_data.at(1) & 0x0f) << 12)
	                 + ((this->_data.at(2) & 0xff) << 4)
	                 + ((this->_data.at(3) & 0xf0) >> 4));
}

// PTI (3 bits)
uint8_t AtmCell::pt()
{
	if(!this->isValid())
	{
		UTI_ERROR("[AtmCell::pt] invalid ATM cell\n");
		return 0;
	}

	return (uint8_t) ((this->_data.at(3) & 0x0e) >> 1);
}

// is this ATM cell the last one of the AAL5 frame ?
bool AtmCell::isLastCell()
{
	return (this->pt() & 0x01);
}

void AtmCell::setGfc(uint8_t gfc)
{
	this->_data.replace(0, 1, 1,
		((gfc << 4) & 0xf0) + (this->_data.at(0) & 0x0f));
}

void AtmCell::setVpi(uint8_t vpi)
{
	this->_data.replace(0, 1, 1,
		(this->_data.at(0) & 0xf0) + ((vpi >> 4) & 0x0f));
	this->_data.replace(1, 1, 1,
		((vpi << 4) & 0xf0) + (this->_data.at(1) & 0x0f));
}

void AtmCell::setVci(uint16_t vci)
{
	this->_data.replace(1, 1, 1,
		(this->_data.at(1) & 0xf0) + ((vci >> 12) & 0x0f));
	this->_data.replace(2, 1, 1, (vci >> 4) & 0xff);
	this->_data.replace(3, 1, 1,
		((vci << 4) & 0xf0) + (this->_data.at(3) & 0x0f));
}

void AtmCell::setPt(uint8_t pt)
{
	this->_data.replace(3, 1, 1,
		(this->_data.at(3) & 0xf1) + ((pt << 1) & 0x0e));
}

void AtmCell::setClp(uint8_t clp)
{
	this->_data.replace(3, 1, 1,
		(this->_data.at(3) & 0xfe) + (clp & 0x01));
}

void AtmCell::setIsLastCell(bool is_last_cell)
{
	uint8_t mask = is_last_cell ? 0x01 : 0x00;

	this->setPt((this->pt() & 0xfe) | mask);
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
unsigned int AtmCell::length()
{
	return 53;
}

// static
NetPacket *AtmCell::create(Data data)
{
	return new AtmCell(data);
}

// static
unsigned int AtmCell::length(Data *data, unsigned int offset)
{
	return AtmCell::length();
}

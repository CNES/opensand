/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file    Slot.cpp
 * @brief   Represent a RCS slot in a carrier
 * @author  Julien Bernard / Viveris Technologies
 */


#include "Slot.h"


Slot::Slot(unsigned int carriers_id,
           unsigned int slot_id):
	carriers_id(carriers_id),
	slot_id(slot_id),
	packets()
{
}

Slot::~Slot()
{
	this->release();
}

unsigned int Slot::getCarriersId() const
{
	return this->carriers_id;
}

unsigned int Slot::getId() const
{
	return this->slot_id;
}

void Slot::addPacket(SlottedAlohaPacketData *packet)
{
	this->packets.push_back(packet);
}

unsigned int Slot::getNbrPackets(void) const
{
	return this->packets.size();
}

saloha_packets_t &Slot::getPackets(void)
{
	return this->packets;
}


void Slot::clear(void)
{
	this->packets.clear();
}

void Slot::release(void)
{
	for(saloha_packets_t::iterator it = this->packets.begin();
	    it != this->packets.end(); ++it)
	{
		delete *it;
	}
	this->clear();
}

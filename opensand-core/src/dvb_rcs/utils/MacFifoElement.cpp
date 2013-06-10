/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file MacFifoElement.cpp
 * @brief Fifo Element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "MacFifoElement.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"


MacFifoElement::MacFifoElement(unsigned char *data, unsigned int length,
                               long tick_in, long tick_out):
	type(0),
	data(data),
	length(length),
	packet(NULL),
	tick_in(tick_in),
	tick_out(tick_out)
{
}

MacFifoElement::MacFifoElement(NetPacket *packet,
                               long tick_in, long tick_out):
	type(1),
	data(NULL),
	length(0),
	packet(packet),
	tick_in(tick_in),
	tick_out(tick_out)
{
}


MacFifoElement::~MacFifoElement()
{
}

unsigned char *MacFifoElement::getData()
{
	return this->data;
}

unsigned int MacFifoElement::getDataLength()
{
	return this->length;
}

void MacFifoElement::setPacket(NetPacket *packet)
{
	this->packet = packet;
}

NetPacket *MacFifoElement::getPacket()
{
	return this->packet;
}

long MacFifoElement::getType()
{
	return this->type;
}

long MacFifoElement::getTickIn()
{
	return this->tick_in;
}

long MacFifoElement::getTickOut()
{
	return this->tick_out;
}

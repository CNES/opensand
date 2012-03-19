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
 * @file MacFifoElement.cpp
 * @brief Fifo Element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "MacFifoElement.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"

// Be careful, the maximum FIFO size sum should be smaller than the number of elements
// in the memory pool (it is the worst case)
// TODO configuration parameter for that or compute it with FIFO sizes
mgl_memory_pool MacFifoElement::mempool(sizeof(MacFifoElement), 100000,  "fifo_element");


MacFifoElement::MacFifoElement(unsigned char *data, unsigned int length,
                               long tick_in, long tick_out)
{
	this->_type = 0;
	this->_data = data;
	this->_length = length;
	this->_tick_in = tick_in;
	this->_tick_out = tick_out;
}

MacFifoElement::MacFifoElement(NetPacket *packet,
                               long tick_in, long tick_out)
{
	this->_type = 1;
	this->_packet = packet;
	this->_tick_in = tick_in;
	this->_tick_out = tick_out;
}


MacFifoElement::~MacFifoElement()
{
}

void MacFifoElement::addTrace(std::string name_function)
{
	mempool.add_function(name_function, (char *) this);
}


unsigned char *MacFifoElement::getData()
{
	return this->_data;
}

unsigned int MacFifoElement::getDataLength()
{
	return this->_length;
}

void MacFifoElement::setPacket(NetPacket *packet)
{
	this->_packet = packet;
}

NetPacket *MacFifoElement::getPacket()
{
	return this->_packet;
}

long MacFifoElement::getType()
{
	return this->_type;
}

long MacFifoElement::getTickIn()
{
	return this->_tick_in;
}

long MacFifoElement::getTickOut()
{
	return this->_tick_out;
}

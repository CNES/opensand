/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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
 * @file TestDelayFifoElement.cpp
 * @brief Fifo Element
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.com>
 */


#include "TestDelayFifoElement.h"

TestDelayFifoElement::TestDelayFifoElement(NetContainer *elem,
                                             time_t tick_in, time_t tick_out):
	elem(elem),
	tick_in(tick_in),
	tick_out(tick_out)
{
}

TestDelayFifoElement::~TestDelayFifoElement()
{
}

NetContainer *TestDelayFifoElement::getElem() const
{
	return this->elem;
}

void TestDelayFifoElement::setElem(NetContainer *elem)
{
	this->elem = elem;
}

size_t TestDelayFifoElement::getTotalLength() const
{
	return this->elem->getTotalLength();
}

time_t TestDelayFifoElement::getTickIn() const
{
	return this->tick_in;
}

time_t TestDelayFifoElement::getTickOut() const
{
	return this->tick_out;
}

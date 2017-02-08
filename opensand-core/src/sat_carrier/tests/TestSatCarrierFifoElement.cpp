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
 * @file TestSatCarrierFifoElement.cpp
 * @brief Fifo Element
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.com>
 */


#include "TestSatCarrierFifoElement.h"

TestSatCarrierFifoElement::TestSatCarrierFifoElement(NetContainer *elem,
                                             time_t tick_in, time_t tick_out):
	elem(elem),
	tick_in(tick_in),
	tick_out(tick_out)
{
}

TestSatCarrierFifoElement::~TestSatCarrierFifoElement()
{
}

NetContainer *TestSatCarrierFifoElement::getElem() const
{
	return this->elem;
}

void TestSatCarrierFifoElement::setElem(NetContainer *elem)
{
	this->elem = elem;
}

size_t TestSatCarrierFifoElement::getTotalLength() const
{
	return this->elem->getTotalLength();
}

time_t TestSatCarrierFifoElement::getTickIn() const
{
	return this->tick_in;
}

time_t TestSatCarrierFifoElement::getTickOut() const
{
	return this->tick_out;
}

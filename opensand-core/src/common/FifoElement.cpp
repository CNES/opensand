/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file FifoElement.cpp
 * @brief Fifo Element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */


#include "FifoElement.h"
#include "NetContainer.h"


FifoElement::FifoElement(Rt::Ptr<NetContainer> elem,
                         time_ms_t tick_in, time_ms_t tick_out):
	elem{std::move(elem)},
	tick_in{tick_in},
	tick_out{tick_out}
{
}


FifoElement::~FifoElement()
{
}


template<>
Rt::Ptr<NetContainer> FifoElement::getElem()
{
	return std::move(this->elem);
}


void FifoElement::setElem(Rt::Ptr<NetContainer> elem)
{
	this->elem = std::move(elem);
}


size_t FifoElement::getTotalLength() const
{
	return this->elem->getTotalLength();
}


time_ms_t FifoElement::getTickIn() const
{
	return this->tick_in;
}


time_ms_t FifoElement::getTickOut() const
{
	return this->tick_out;
}

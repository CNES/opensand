/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file AtmIdentifier.cpp
 * @brief ATM identifier (unique index given by the association of both
 *        the VPI and VCI fields of the ATM header)
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "AtmIdentifier.h"


AtmIdentifier::AtmIdentifier(uint8_t vpi, uint16_t vci)
{
	this->setVpi(vpi);
	this->setVci(vci);
}

AtmIdentifier::~AtmIdentifier()
{
}

uint8_t AtmIdentifier::getVpi()
{
	return this->vpi;
}

void AtmIdentifier::setVpi(uint8_t vpi)
{
	this->vpi = vpi;
}

uint16_t AtmIdentifier::getVci()
{
	return this->vci;
}

void AtmIdentifier::setVci(uint16_t vci)
{
	this->vci = vci;
}

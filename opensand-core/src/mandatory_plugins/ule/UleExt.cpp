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
 * @file UleExt.cpp
 * @brief ULE extension
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "UleExt.h"


UleExt::UleExt(): _payload()
{
}

UleExt::~UleExt()
{
}

uint8_t UleExt::type()
{
	return this->_type;
}

bool UleExt::isMandatory()
{
	return this->is_mandatory;
}

Data UleExt::payload()
{
	return this->_payload;
}

uint16_t UleExt::payloadType()
{
	return this->_payloadType;
}

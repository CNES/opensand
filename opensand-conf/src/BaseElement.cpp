/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 Viveris Technologies
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file BaseElement.cpp
 * @brief Base class of all elements.
 */

#include <sstream>

#include "BaseElement.h"


OpenSANDConf::BaseElement::BaseElement(const std::string &id):
	id(id)
{
}


OpenSANDConf::BaseElement::BaseElement(const OpenSANDConf::BaseElement &other):
	id(other.id)
{
}


OpenSANDConf::BaseElement::~BaseElement()
{
}


const std::string &OpenSANDConf::BaseElement::getId() const
{
	return this->id;
}

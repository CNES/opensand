/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file Data.cpp
 * @brief Represents a data value
 */

#include "Data.h"


OpenSANDConf::Data::Data():
	is_set(false)
{
}

OpenSANDConf::Data::~Data()
{
}

bool OpenSANDConf::Data::isSet() const
{
	return this->is_set;
}

void OpenSANDConf::Data::reset()
{
	this->is_set = false;
}

bool OpenSANDConf::Data::equal(const OpenSANDConf::Data &other) const
{
	return this->is_set == other.is_set;
}

bool OpenSANDConf::operator== (const OpenSANDConf::Data &v1, const OpenSANDConf::Data &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::Data &v1, const OpenSANDConf::Data &v2)
{
	return !(v1.equal(v2));
}

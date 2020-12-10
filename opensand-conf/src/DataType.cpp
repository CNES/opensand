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
 * @file DataType.cpp
 * @brief Represents a data type
 */

#include "DataType.h"
#include "DataTypesList.h"
#include "Data.h"


OpenSANDConf::DataType::DataType(const std::string &id):
	OpenSANDConf::BaseElement(id)
{
}

OpenSANDConf::DataType::~DataType()
{
}

bool OpenSANDConf::DataType::equal(const OpenSANDConf::DataType &other) const
{
	return this->getId() == other.getId();
}

bool OpenSANDConf::operator== (const OpenSANDConf::DataType &v1, const OpenSANDConf::DataType &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::DataType &v1, const OpenSANDConf::DataType &v2)
{
	return !v1.equal(v2);
}

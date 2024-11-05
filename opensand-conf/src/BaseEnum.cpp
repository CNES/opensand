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
 * @file BaseEnum.cpp
 * @brief Represents the Base of enumeration
 */

#include <set>
#include <algorithm>

#include "BaseEnum.h"


OpenSANDConf::BaseEnum::BaseEnum(const std::vector<std::string> &values):
	values()
{
	std::set<std::string> unique_values(values.begin(), values.end());
	this->values = std::vector<std::string>(unique_values.begin(), unique_values.end());
}


OpenSANDConf::BaseEnum::BaseEnum(const OpenSANDConf::BaseEnum &other):
	values(other.values)
{
}


OpenSANDConf::BaseEnum::~BaseEnum()
{
}


const std::vector<std::string> &OpenSANDConf::BaseEnum::getValues() const
{
	return this->values;
}

std::vector<std::string> &OpenSANDConf::BaseEnum::getMutableValues()
{
	return this->values;
}


bool OpenSANDConf::BaseEnum::equal(const BaseEnum &other) const
{
	return this->values.size() == other.values.size()
		&& std::equal(this->values.begin(), this->values.end(), other.values.begin());
}

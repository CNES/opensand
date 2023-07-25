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
 * @file DataValue.h
 * @brief Represents a specialized data value
 */

#include "DataValue.h"


template <>
bool OpenSANDConf::DataValue<std::string>::fromString(std::string val)
{
	return this->set(val);
}


template <>
bool OpenSANDConf::DataValue<uint8_t>::fromString(std::string val)
{
	std::stringstream ss(val);
	unsigned int tmp;
	ss >> tmp;
	return (!ss.fail()) && this->set(tmp);
}


template <>
bool OpenSANDConf::DataValue<int8_t>::fromString(std::string val)
{
	std::stringstream ss(val);
	int tmp;
	ss >> tmp;
	return (!ss.fail()) && this->set(tmp);
}


template <>
std::string OpenSANDConf::DataValue<uint8_t>::toString() const
{
	std::stringstream ss;
	if(this->is_set)
	{
		ss << static_cast<unsigned int>(this->value);
	}
	return ss.str();
}


template <>
std::string OpenSANDConf::DataValue<int8_t>::toString() const
{
	std::stringstream ss;
	if(this->is_set)
	{
		ss << static_cast<int>(this->value);
	}
	return ss.str();
}

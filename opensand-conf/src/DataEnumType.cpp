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
 * @file DataEnumType.cpp
 * @brief Represents a data enumeration type
 */

#include <algorithm>

#include "DataEnumType.h"
#include "DataValue.h"
#include "DataTypesList.h"


OpenSANDConf::DataEnumType::DataEnumType(const std::string &id, const std::vector<std::string> &values):
	OpenSANDConf::DataValueType<std::string>(id),
	OpenSANDConf::BaseEnum(values)
{
}

OpenSANDConf::DataEnumType::~DataEnumType()
{
}

std::shared_ptr<OpenSANDConf::DataType> OpenSANDConf::DataEnumType::clone() const
{
	return std::shared_ptr<OpenSANDConf::DataEnumType>(new OpenSANDConf::DataEnumType(this->getId(), this->getValues()));
}

bool OpenSANDConf::DataEnumType::equal(const DataType &other) const
{
	const OpenSANDConf::DataEnumType *elt = dynamic_cast<const OpenSANDConf::DataEnumType *>(&other);
	if(elt == nullptr || !this->OpenSANDConf::DataValueType<std::string>::equal(*elt))
	{
		return false;
	}
	return this->OpenSANDConf::BaseEnum::equal(*elt);
}

bool OpenSANDConf::DataEnumType::check(std::string value) const
{
	if(!this->OpenSANDConf::DataValueType<std::string>::check(value))
	{
		return true;
	}
	auto values = this->getValues();
	auto iter = std::find(values.begin(), values.end(), value);
	return iter != values.end();
}

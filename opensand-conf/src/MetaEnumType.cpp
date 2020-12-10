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
 * @file MetaEnumType.cpp
 * @brief Represents a meta enumeration type
 */

#include <algorithm>

#include "MetaEnumType.h"
#include "DataEnumType.h"


OpenSANDConf::MetaEnumType::MetaEnumType(
		const std::string &id,
		const std::string &name,
		const std::string &description,
		const std::vector<std::string> &values):
	OpenSANDConf::MetaValueType<std::string>(id, name, description),
	OpenSANDConf::BaseEnum(values)
{
}

OpenSANDConf::MetaEnumType::MetaEnumType(const OpenSANDConf::MetaEnumType &other):
	OpenSANDConf::MetaValueType<std::string>(other),
	OpenSANDConf::BaseEnum(other)
{
}

OpenSANDConf::MetaEnumType::~MetaEnumType()
{
}

std::shared_ptr<OpenSANDConf::MetaType> OpenSANDConf::MetaEnumType::clone() const
{
	return std::shared_ptr<OpenSANDConf::MetaEnumType>(new OpenSANDConf::MetaEnumType(*this));
}

std::shared_ptr<OpenSANDConf::DataType> OpenSANDConf::MetaEnumType::createData() const
{
	return std::shared_ptr<OpenSANDConf::DataEnumType>(new OpenSANDConf::DataEnumType(this->getId(), this->getValues()));
}

bool OpenSANDConf::MetaEnumType::equal(const MetaType &other) const
{
	auto elt = dynamic_cast<const OpenSANDConf::MetaEnumType *>(&other);
	if(elt == nullptr || !this->OpenSANDConf::MetaValueType<std::string>::equal(*elt))
	{
		return false;
	}
	return this->OpenSANDConf::BaseEnum::equal(*elt);
}

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
 * @file DataTypesList.cpp
 * @brief Represents a list of data types.
 */

#include <algorithm>

#include "DataTypesList.h"
#include "DataType.h"


OpenSANDConf::DataTypesList::DataTypesList():
	types()
{
}

OpenSANDConf::DataTypesList::DataTypesList(const OpenSANDConf::DataTypesList &other):
	types()
{
	for(auto type: other.types)
	{
		this->types.insert({
				type.first,
				std::static_pointer_cast<OpenSANDConf::DataType>(type.second->clone())
		});
	}
}

OpenSANDConf::DataTypesList::~DataTypesList()
{
}

std::shared_ptr<OpenSANDConf::DataTypesList> OpenSANDConf::DataTypesList::clone() const
{
	return std::shared_ptr<OpenSANDConf::DataTypesList>(new OpenSANDConf::DataTypesList(*this));
}

bool OpenSANDConf::DataTypesList::equal(const OpenSANDConf::DataTypesList &other) const
{
	return this->types.size() == other.types.size()
		&& std::equal(this->types.begin(), this->types.end(), other.types.begin());
}

std::shared_ptr<OpenSANDConf::DataType> OpenSANDConf::DataTypesList::getType(const std::string &id) const
{
	auto type = this->types.find(id);
	return type != this->types.end() ? type->second : nullptr;
}

bool OpenSANDConf::DataTypesList::addType(std::shared_ptr<DataType> type)
{
	auto id = type->getId();
	if(this->getType(id) != nullptr)
	{
		return false;
	}
	this->types.insert({ id, type});
	return true;
}

bool OpenSANDConf::operator== (const OpenSANDConf::DataTypesList &v1, const OpenSANDConf::DataTypesList &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::DataTypesList &v1, const OpenSANDConf::DataTypesList &v2)
{
	return !v1.equal(v2);
}

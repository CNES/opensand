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
 * @file MetaTypesList.cpp
 * @brief Represents a list of meta types.
 */

#include <set>
#include <algorithm>

#include "MetaTypesList.h"

#include "MetaType.h"
#include "MetaValueType.h"
#include "MetaEnumType.h"


void OpenSANDConf::MetaTypesList::addToMap(std::map<std::string, std::shared_ptr<OpenSANDConf::MetaType>> &types, std::shared_ptr<OpenSANDConf::MetaType> type)
{
	types.insert({ type->getId(), type });
}

OpenSANDConf::MetaTypesList::MetaTypesList():
	types(),
	enums()
{
	// TODO: specify numerical limit
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<bool>("bool", "Boolean", "A boolean")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<double>("double", "Double", "A double")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<float>("float", "Float", "A float")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<int8_t>("byte", "Byte integer", "A single byte integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<int16_t>("short", "Short integer", "A two bytes integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<int32_t>("int", "Integer", "A four bytes integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<int64_t>("long", "Long integer", "An eight bytes integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<uint8_t>("ubyte", "Unsigned Byte", "An unsigned byte integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<uint16_t>("ushort", "Unsgined Short", "An unsgined short integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<uint32_t>("uint", "Unsigned Integer", "An unsigned integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<uint64_t>("ulong", "Unsigned Long", "An unsigned long integer")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<std::string>("char", "Character", "A single character")));
	addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
	         new OpenSANDConf::MetaValueType<std::string>("string", "String", "A string")));
}

OpenSANDConf::MetaTypesList::MetaTypesList(const OpenSANDConf::MetaTypesList &other):
	types(),
	enums()
{
	for(auto type: other.types)
	{
		addToMap(this->types, std::shared_ptr<OpenSANDConf::MetaType>(
		         type.second->clone()));
	}
	for(auto type: other.enums)
	{
		addToMap(this->enums, std::shared_ptr<OpenSANDConf::MetaType>(
		         type.second->clone()));
	}
}

OpenSANDConf::MetaTypesList::~MetaTypesList()
{
}

std::shared_ptr<OpenSANDConf::MetaTypesList> OpenSANDConf::MetaTypesList::clone() const
{
	return std::shared_ptr<OpenSANDConf::MetaTypesList>(new OpenSANDConf::MetaTypesList(*this));
}

std::shared_ptr<OpenSANDConf::DataTypesList> OpenSANDConf::MetaTypesList::createData() const
{
	auto data = std::shared_ptr<OpenSANDConf::DataTypesList>(new OpenSANDConf::DataTypesList{});
	for(auto type: this->getTypes())
	{
		data->addType(type->createData());
	}
	return data;
}

bool OpenSANDConf::MetaTypesList::equal(const OpenSANDConf::MetaTypesList &other) const
{
	return this->types.size() == other.types.size()
		&& this->enums.size() == other.enums.size()
		&& std::equal(this->types.begin(), this->types.end(), other.types.begin())
		&& std::equal(this->enums.begin(), this->enums.end(), other.enums.begin());
}

std::shared_ptr<OpenSANDConf::MetaType> OpenSANDConf::MetaTypesList::getType(const std::string &id) const
{
	auto iter = this->types.find(id);
	if(iter != this->types.end())
	{
		return iter->second;
	}
	iter = this->enums.find(id);
	return iter != this->enums.end() ? iter->second : nullptr;
}

std::vector<std::shared_ptr<OpenSANDConf::MetaEnumType>> OpenSANDConf::MetaTypesList::getEnumTypes() const
{
  std::vector<std::shared_ptr<OpenSANDConf::MetaEnumType>> enumtypes;
	for(auto iter: this->enums)
	{
		enumtypes.push_back(std::static_pointer_cast<OpenSANDConf::MetaEnumType>(iter.second));
	}
	return enumtypes;
}

std::shared_ptr<OpenSANDConf::MetaEnumType> OpenSANDConf::MetaTypesList::addEnumType(const std::string &id, const std::string &name, const std::vector<std::string> &values)
{
	return this->addEnumType(id, name, values, "");
}

std::shared_ptr<OpenSANDConf::MetaEnumType> OpenSANDConf::MetaTypesList::addEnumType(const std::string &id, const std::string &name, const std::vector<std::string> &values, const std::string &description)
{
	std::set<std::string> unique_values(values.begin(), values.end());
	if(unique_values.size() == 0 || this->getType(id) != nullptr)
	{
		return nullptr;
	}
	auto type = std::shared_ptr<OpenSANDConf::MetaEnumType>(new OpenSANDConf::MetaEnumType(id, name, description, std::vector<std::string>(unique_values.begin(), unique_values.end())));
	if(type != nullptr)
	{
		addToMap(this->enums, std::shared_ptr<OpenSANDConf::MetaType>(type));
	}
	return type;
}

std::vector<std::shared_ptr<OpenSANDConf::MetaType>> OpenSANDConf::MetaTypesList::getTypes() const
{
	std::vector<std::shared_ptr<OpenSANDConf::MetaType>> alltypes;
	for(auto &&iter: this->types)
	{
		alltypes.push_back(std::static_pointer_cast<OpenSANDConf::MetaType>(iter.second));
	}
	for(auto &&iter: this->enums)
	{
		alltypes.push_back(std::static_pointer_cast<OpenSANDConf::MetaType>(iter.second));
	}
	return alltypes;
}


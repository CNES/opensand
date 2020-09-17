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

#include <MetaTypesList.h>

#include <MetaType.h>
#include <MetaValueType.h>
#include <MetaEnumType.h>

#include <set>
#include <algorithm>

void OpenSANDConf::MetaTypesList::addToMap(map<string, shared_ptr<OpenSANDConf::MetaType>> &types, shared_ptr<OpenSANDConf::MetaType> type)
{
	types.insert({ type->getId(), type });
}

OpenSANDConf::MetaTypesList::MetaTypesList():
	types(),
	enums()
{
	// TODO: specify numerical limit
	addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
			new OpenSANDConf::MetaValueType<bool>("bool", "Boolean", "A boolean")));
	addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
			new OpenSANDConf::MetaValueType<double>("double", "Double", "A double")));
	addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
			new OpenSANDConf::MetaValueType<float>("float", "Float", "A float")));
	addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
			new OpenSANDConf::MetaValueType<int>("int", "Integer", "An integer")));
	addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
			new OpenSANDConf::MetaValueType<short>("short", "Short integer", "A short integer")));
	addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
			new OpenSANDConf::MetaValueType<long>("long", "Long integer", "A lon integer")));
	addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
			new OpenSANDConf::MetaValueType<string>("string", "String", "A string")));
}

OpenSANDConf::MetaTypesList::MetaTypesList(const OpenSANDConf::MetaTypesList &other):
	types(),
	enums()
{
	for(auto type: other.types)
	{
		addToMap(this->types, shared_ptr<OpenSANDConf::MetaType>(
					type.second->clone()));
	}
	for(auto type: other.enums)
	{
		addToMap(this->enums, shared_ptr<OpenSANDConf::MetaType>(
					type.second->clone()));
	}
}

OpenSANDConf::MetaTypesList::~MetaTypesList()
{
}

shared_ptr<OpenSANDConf::MetaTypesList> OpenSANDConf::MetaTypesList::clone() const
{
	return shared_ptr<OpenSANDConf::MetaTypesList>(new OpenSANDConf::MetaTypesList(*this));
}

shared_ptr<OpenSANDConf::DataTypesList> OpenSANDConf::MetaTypesList::createData() const
{
	auto data = shared_ptr<OpenSANDConf::DataTypesList>(new OpenSANDConf::DataTypesList());
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

shared_ptr<OpenSANDConf::MetaType> OpenSANDConf::MetaTypesList::getType(const string &id) const
{
	auto iter = this->types.find(id);
	if(iter != this->types.end())
	{
		return iter->second;
	}
	iter = this->enums.find(id);
	return iter != this->enums.end() ? iter->second : nullptr;
}

vector<shared_ptr<OpenSANDConf::MetaEnumType>> OpenSANDConf::MetaTypesList::getEnumTypes() const
{
	vector<shared_ptr<OpenSANDConf::MetaEnumType>> enumtypes;
	for(auto iter: this->enums)
	{
		enumtypes.push_back(std::static_pointer_cast<OpenSANDConf::MetaEnumType>(iter.second));
	}
	return enumtypes;
}

shared_ptr<OpenSANDConf::MetaEnumType> OpenSANDConf::MetaTypesList::addEnumType(const string &id, const string &name, const std::vector<string> &values)
{
	return this->addEnumType(id, name, values, "");
}

shared_ptr<OpenSANDConf::MetaEnumType> OpenSANDConf::MetaTypesList::addEnumType(const string &id, const string &name, const std::vector<string> &values, const string &description)
{
	std::set<string> unique_values(values.begin(), values.end());
	if(unique_values.size() == 0 || this->getType(id) != nullptr)
	{
		return nullptr;
	}
	auto type = shared_ptr<OpenSANDConf::MetaEnumType>(new OpenSANDConf::MetaEnumType(id, name, description, vector<string>(unique_values.begin(), unique_values.end())));
	if(type != nullptr)
	{
		addToMap(this->enums, shared_ptr<OpenSANDConf::MetaType>(type));
	}
	return type;
}

vector<shared_ptr<OpenSANDConf::MetaType>> OpenSANDConf::MetaTypesList::getTypes() const
{
	vector<shared_ptr<OpenSANDConf::MetaType>> alltypes;
	for(auto iter: this->types)
	{
		alltypes.push_back(std::static_pointer_cast<OpenSANDConf::MetaType>(iter.second));
	}
	for(auto iter: this->enums)
	{
		alltypes.push_back(std::static_pointer_cast<OpenSANDConf::MetaType>(iter.second));
	}
	return alltypes;
}


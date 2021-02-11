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
 * @file DataParameter.cpp
 * @brief Represents a datamodel parameter
 */

#include "DataParameter.h"
#include "DataType.h"


OpenSANDConf::DataParameter::DataParameter(
		const std::string &id,
		const std::string &parent,
		std::shared_ptr<OpenSANDConf::Data> data):
	OpenSANDConf::DataElement(id, parent),
	std::enable_shared_from_this<OpenSANDConf::DataParameter>(),
	data(data)
{
}

OpenSANDConf::DataParameter::DataParameter(
		const OpenSANDConf::DataParameter &other,
		std::shared_ptr<DataTypesList> types):
	OpenSANDConf::DataElement(other),
	std::enable_shared_from_this<OpenSANDConf::DataParameter>(),
	data(other.data->clone(types))
{
}

OpenSANDConf::DataParameter::DataParameter(
		const std::string &id,
		const std::string &parent,
		const DataParameter &other):
	OpenSANDConf::DataElement(id, parent),
	std::enable_shared_from_this<OpenSANDConf::DataParameter>(),
	data(other.data->duplicate())
{
}

OpenSANDConf::DataParameter::~DataParameter()
{
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataParameter::clone(std::shared_ptr<OpenSANDConf::DataTypesList> types) const
{
	return std::shared_ptr<OpenSANDConf::DataParameter>(new OpenSANDConf::DataParameter(*this, types));
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataParameter::duplicateObject(const std::string &id, const std::string &parent) const
{
	return std::shared_ptr<OpenSANDConf::DataParameter>(new OpenSANDConf::DataParameter(id, parent, *this));
}

std::tuple<std::shared_ptr<const OpenSANDConf::DataParameter>, std::shared_ptr<OpenSANDConf::Data>> OpenSANDConf::DataParameter::createReference() const
{
	auto expected = this->data->getType()->createData();
	return std::make_tuple(this->shared_from_this(), expected);
}

bool OpenSANDConf::DataParameter::validate() const
{
	if(!this->DataElement::checkReference())
	{
		return true;
	}
	return this->data->isSet();
}

std::shared_ptr<OpenSANDConf::Data> OpenSANDConf::DataParameter::getData() const
{
	return this->data;
}

bool OpenSANDConf::DataParameter::equal(const DataElement &other) const
{
	const OpenSANDConf::DataParameter *param = dynamic_cast<const OpenSANDConf::DataParameter *>(&other);
	if(param == nullptr || !this->OpenSANDConf::DataElement::equal(*param))
	{
		return false;
	}
	return this->data == param->data;
}

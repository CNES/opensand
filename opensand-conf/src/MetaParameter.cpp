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
 * @file MetaParameter.cpp
 * @brief Represents a metamodel parameter
 */

#include "MetaParameter.h"
#include "MetaTypesList.h"
#include "MetaType.h"
#include "DataTypesList.h"
#include "DataParameter.h"
#include "DataType.h"
#include "Data.h"


OpenSANDConf::MetaParameter::MetaParameter(
		const std::string &id,
		const std::string &parent,
		const std::string &name,
		const std::string &description,
		std::shared_ptr<OpenSANDConf::MetaType> type):
	OpenSANDConf::MetaElement(id, parent, name, description),
	std::enable_shared_from_this<OpenSANDConf::MetaParameter>(),
	type(type),
	unit()
{
}

OpenSANDConf::MetaParameter::MetaParameter(
		const OpenSANDConf::MetaParameter &other,
		std::weak_ptr<const MetaTypesList> types):
	OpenSANDConf::MetaElement(other),
	std::enable_shared_from_this<OpenSANDConf::MetaParameter>(),
	type(types.lock()->getType(other.type->getId())),
	unit(other.unit)
{
}

OpenSANDConf::MetaParameter::~MetaParameter()
{
}

std::shared_ptr<OpenSANDConf::MetaElement> OpenSANDConf::MetaParameter::clone(std::weak_ptr<const OpenSANDConf::MetaTypesList> types) const
{
	return std::shared_ptr<OpenSANDConf::MetaParameter>(new OpenSANDConf::MetaParameter(*this, types));
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::MetaParameter::createData(std::shared_ptr<OpenSANDConf::DataTypesList> types) const
{
	auto datatype = types->getType(this->type->getId());
	if(datatype == nullptr)
	{
		return nullptr;
	}
	return std::shared_ptr<OpenSANDConf::DataParameter>(new OpenSANDConf::DataParameter(this->getId(), this->getParentPath(), datatype->createData()));
}

std::shared_ptr<OpenSANDConf::MetaType> OpenSANDConf::MetaParameter::getType() const
{
	return this->type;
}

const std::string &OpenSANDConf::MetaParameter::getUnit() const
{
	return this->unit;
}

void OpenSANDConf::MetaParameter::setUnit(const std::string &unit)
{
	this->unit = unit;
}

bool OpenSANDConf::MetaParameter::equal(const MetaElement &other) const
{
	const OpenSANDConf::MetaParameter *param = dynamic_cast<const OpenSANDConf::MetaParameter *>(&other);
	if(param == nullptr || !this->OpenSANDConf::MetaElement::equal(*param))
	{
		return false;
	}
	return this->type == param->type && this->unit == param->unit;
}

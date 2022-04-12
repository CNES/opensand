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
 * @file MetaElement.cpp
 * @brief Base class of all metamodel elements.
 */

#include <sstream>

#include "MetaElement.h"
#include "DataElement.h"
#include "MetaParameter.h"
#include "MetaType.h"
#include "DataType.h"
#include "Data.h"
#include "MetaContainer.h"


std::shared_ptr<OpenSANDConf::MetaElement> OpenSANDConf::MetaElement::getItemFromRoot(std::shared_ptr<OpenSANDConf::MetaElement> root, const std::string &path)
{
  std::shared_ptr<OpenSANDConf::MetaElement> elt;
  std::shared_ptr<OpenSANDConf::MetaContainer> cont;

  std::string item;
  std::stringstream ss(path);

	if(path.empty())
	{
		return nullptr;
	}
	elt = root;
	while(elt != nullptr && std::getline(ss, item, '/'))
	{
		if(item.empty())
		{
			continue;
		}
		cont = std::dynamic_pointer_cast<OpenSANDConf::MetaContainer>(elt);
		if(cont == nullptr)
		{
			elt = nullptr;
			break;
		}
		elt = cont->getItem(item);
	}
	return elt;
}

OpenSANDConf::MetaElement::MetaElement(
		const std::string &id,
		const std::string &parent,
		const std::string &name,
		const std::string &description):
	OpenSANDConf::NamedElement(id, name, description),
	parent(parent),
	advanced(false),
	readOnly(false)
{
	this->reference = std::make_tuple<std::shared_ptr<OpenSANDConf::MetaParameter>, std::shared_ptr<OpenSANDConf::Data>, std::shared_ptr<OpenSANDConf::DataType>>(nullptr, nullptr, nullptr);
}

OpenSANDConf::MetaElement::MetaElement(const OpenSANDConf::MetaElement &other):
	OpenSANDConf::NamedElement(other),
	parent(other.parent),
	advanced(other.advanced),
	readOnly(other.readOnly)
{
	// This constructor by copy is used to clone the object.
	// The reference must be set after the complete copy of the meta model.
	this->reference = std::make_tuple<std::shared_ptr<OpenSANDConf::MetaParameter>, std::shared_ptr<OpenSANDConf::Data>, std::shared_ptr<OpenSANDConf::DataType>>(nullptr, nullptr, nullptr);
}

OpenSANDConf::MetaElement::~MetaElement()
{
}

const std::string &OpenSANDConf::MetaElement::getParentPath() const
{
	return this->parent;
}

std::string OpenSANDConf::MetaElement::getPath() const
{
	std::stringstream ss;
	ss << this->parent << "/" << this->getId();
	// check root case
	return ss.str() != "/" ? ss.str() : "";
}

bool OpenSANDConf::MetaElement::isAdvanced() const
{
	return this->advanced;
}

void OpenSANDConf::MetaElement::setAdvanced(bool advanced)
{
	this->advanced = advanced;
}

bool OpenSANDConf::MetaElement::isReadOnly() const
{
	return this->readOnly;
}

void OpenSANDConf::MetaElement::setReadOnly(bool readOnly)
{
	this->readOnly = readOnly;
}

void OpenSANDConf::MetaElement::setReference(std::shared_ptr<OpenSANDConf::MetaParameter> target)
{
	if(target != nullptr)
	{
		auto datatype = target->getType()->createData();
		auto expected = datatype->createData();
		this->reference = std::make_tuple(target, expected, datatype);
	}
	else
	{
		this->reference = std::make_tuple<std::shared_ptr<OpenSANDConf::MetaParameter>, std::shared_ptr<OpenSANDConf::Data>, std::shared_ptr<OpenSANDConf::DataType>>(nullptr, nullptr, nullptr);
	}
}

std::shared_ptr<OpenSANDConf::MetaParameter> OpenSANDConf::MetaElement::getReferenceTarget() const
{
	return std::get<0>(this->reference);
}

std::shared_ptr<OpenSANDConf::Data> OpenSANDConf::MetaElement::getReferenceData() const
{
	return std::get<1>(this->reference);
}

bool OpenSANDConf::MetaElement::equal(const OpenSANDConf::MetaElement &other) const
{
	if(!this->OpenSANDConf::NamedElement::equal(other))
	{
		return false;
	}
	auto target = std::get<0>(this->reference);
	auto otarget = std::get<0>(other.reference);
	if((target == nullptr && otarget != nullptr)
		|| (target != nullptr && otarget == nullptr)
		|| (target != nullptr && otarget != nullptr && *(target) != *(otarget)))
	{
		return false;
	}
	auto data = std::get<1>(this->reference);
	auto odata = std::get<1>(other.reference);
	if((data == nullptr && odata != nullptr)
		|| (data != nullptr && odata == nullptr)
		|| (data != nullptr && odata != nullptr
		       	&& *(data) != *(odata)))
	{
		return false;
	}
	return this->parent == other.parent
		&& this->advanced == other.advanced
		&& this->readOnly == other.readOnly;
}

bool OpenSANDConf::operator== (const OpenSANDConf::MetaElement &v1, const OpenSANDConf::MetaElement &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::MetaElement &v1, const OpenSANDConf::MetaElement &v2)
{
	return !(v1.equal(v2));
}

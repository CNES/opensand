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
 * @file DataElement.cpp
 * @brief Base class of all datamodel elements.
 */

#include <sstream>

#include "DataElement.h"
#include "DataTypesList.h"
#include "DataParameter.h"
#include "DataList.h"
#include "DataComponent.h"
#include "Data.h"
#include "DataContainer.h"


std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataElement::getItemFromRoot(std::shared_ptr<OpenSANDConf::DataElement> root, const std::string &path, bool meta)
{
  std::shared_ptr<OpenSANDConf::DataElement> elt;
  std::shared_ptr<OpenSANDConf::DataContainer> cont;

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
		cont = std::dynamic_pointer_cast<OpenSANDConf::DataContainer>(elt);
		if(cont == nullptr)
		{
			elt = nullptr;
			break;
		}
		if(item == "*" && meta)
		{
			auto lst = std::dynamic_pointer_cast<OpenSANDConf::DataList>(cont);
			if(lst == nullptr)
			{
				elt = nullptr;
				break;
			}
			elt = lst->getPattern();
			continue;
		}
		elt = cont->getItem(item);
        }
	return elt;
}

OpenSANDConf::DataElement::DataElement(const std::string &id, const std::string &parent):
	OpenSANDConf::BaseElement(id),
	parent(parent)
{
	this->reference = std::make_tuple<std::shared_ptr<const OpenSANDConf::DataParameter>, std::shared_ptr<OpenSANDConf::Data>>(nullptr, nullptr);
}

OpenSANDConf::DataElement::DataElement(const OpenSANDConf::DataElement &other):
	OpenSANDConf::BaseElement(other),
	parent(other.parent)
{
	// This constructor by copy is used to clone the object.
	// The reference must be set after the complete copy of the meta model.
	this->reference = std::make_tuple<std::shared_ptr<const OpenSANDConf::DataParameter>, std::shared_ptr<OpenSANDConf::Data>>(nullptr, nullptr);
}

OpenSANDConf::DataElement::~DataElement()
{
}

bool OpenSANDConf::DataElement::duplicateReference(std::shared_ptr<OpenSANDConf::DataElement> copy) const
{
	auto target = this->getReferenceTarget();
	if(target == nullptr)
	{
		return true;
	}
	copy->setReference(target);
	auto copy_data = copy->getReferenceData();
	if(copy_data == nullptr || !copy_data->copy(this->getReferenceData()))
	{
		return false;
	}
	return true;
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataElement::duplicate(const std::string &id, const std::string &parent) const
{
	auto copy = this->duplicateObject(id, parent);
	if(!this->duplicateReference(copy))
	{
		return nullptr;
	}
	return copy;
}

bool OpenSANDConf::DataElement::checkReference() const
{
	auto target = this->getReferenceTarget();
	if(target == nullptr)
	{
		return true;
	}
	auto expected = this->getReferenceData();
	if(expected == nullptr || !expected->isSet())
	{
		return false;
	}
	auto data = target->getData();
	if(data == nullptr || !data->isSet())
	{
		return false;
	}
	return *data == *expected;
}

const std::string &OpenSANDConf::DataElement::getParentPath() const
{
	return this->parent;
}

std::string OpenSANDConf::DataElement::getPath() const
{
	std::stringstream ss;
	ss << this->parent << "/" << this->getId();
	// check root case
	return ss.str() != "/" ? ss.str() : "";
}

void OpenSANDConf::DataElement::setReference(std::shared_ptr<const OpenSANDConf::DataParameter> target)
{
	if(target != nullptr)
	{
		this->reference = target->createReference();
	}
	else
	{
		this->reference = std::make_tuple<std::shared_ptr<const OpenSANDConf::DataParameter>, std::shared_ptr<OpenSANDConf::Data>>(nullptr, nullptr);
	}
}

std::shared_ptr<const OpenSANDConf::DataParameter> OpenSANDConf::DataElement::getReferenceTarget() const
{
	return std::get<0>(this->reference);
}

std::shared_ptr<OpenSANDConf::Data> OpenSANDConf::DataElement::getReferenceData() const
{
	return std::get<1>(this->reference);
}


bool OpenSANDConf::DataElement::equal(const OpenSANDConf::DataElement &other) const
{
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
	return this->getId() == other.getId()
		&& this->parent == other.parent;
}

bool OpenSANDConf::operator== (const OpenSANDConf::DataElement &v1, const OpenSANDConf::DataElement &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::DataElement &v1, const OpenSANDConf::DataElement &v2)
{
	return !(v1.equal(v2));
}

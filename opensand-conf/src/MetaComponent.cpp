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
 * @file MetaComponent.cpp
 * @brief Represents a generic metamodel component
 *        (holds a list of components, lists and parameters).
 */

#include <MetaComponent.h>
#include <MetaTypesList.h>
#include <DataComponent.h>
#include <Path.h>

OpenSANDConf::MetaComponent::MetaComponent(
		const string &id,
		const string &parent,
		const string &name,
		const string &description,
		weak_ptr<const OpenSANDConf::MetaTypesList> types):
	OpenSANDConf::MetaContainer(id, parent, name, description, types)
{
}

OpenSANDConf::MetaComponent::MetaComponent(
		const OpenSANDConf::MetaComponent &other,
		weak_ptr<const OpenSANDConf::MetaTypesList> types):
	OpenSANDConf::MetaContainer(other, types)
{
}

OpenSANDConf::MetaComponent::~MetaComponent()
{
}

shared_ptr<OpenSANDConf::MetaElement> OpenSANDConf::MetaComponent::clone(weak_ptr<const OpenSANDConf::MetaTypesList> types) const
{
	return shared_ptr<OpenSANDConf::MetaComponent>(new OpenSANDConf::MetaComponent(*this, types));
}

shared_ptr<OpenSANDConf::MetaParameter> OpenSANDConf::MetaComponent::getParameter(const string &id) const
{
	return std::dynamic_pointer_cast<OpenSANDConf::MetaParameter>(this->getItem(id));
}

shared_ptr<OpenSANDConf::MetaComponent> OpenSANDConf::MetaComponent::getComponent(const string &id) const
{
	return std::dynamic_pointer_cast<OpenSANDConf::MetaComponent>(this->getItem(id));
}

shared_ptr<OpenSANDConf::MetaList> OpenSANDConf::MetaComponent::getList(const string &id) const
{
	return std::dynamic_pointer_cast<OpenSANDConf::MetaList>(this->getItem(id));
}

shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::MetaComponent::createData(shared_ptr<OpenSANDConf::DataTypesList> types) const
{
	auto data = shared_ptr<DataComponent>(new DataComponent(this->getId(), this->getParentPath()));
	this->createAndAddDataItems(types, data);
	return data;
}

shared_ptr<OpenSANDConf::MetaComponent> OpenSANDConf::MetaComponent::addComponent(const string &id, const string &name)
{
	return this->addComponent(id, name, "");
}

shared_ptr<OpenSANDConf::MetaComponent> OpenSANDConf::MetaComponent::addComponent(const string &id, const string &name, const string &description)
{
	if(!checkPathId(id))
	{
		return nullptr;
	}
	if(this->getItem(id) != nullptr)
	{
		return nullptr;
	}
	auto elt = shared_ptr<OpenSANDConf::MetaComponent>(new OpenSANDConf::MetaComponent(id, this->getPath(), name, description, this->getTypes()));
	this->addItem(elt);
	return elt;
}

shared_ptr<OpenSANDConf::MetaList> OpenSANDConf::MetaComponent::addList(const string &id, const string &name, const string &pattern_name)
{
	return this->addList(id, name, pattern_name, "", "");
}

shared_ptr<OpenSANDConf::MetaList> OpenSANDConf::MetaComponent::addList(const string &id, const string &name, const string &pattern_name, const string &description)
{
	return this->addList(id, name, pattern_name, description, "");
}

shared_ptr<OpenSANDConf::MetaList> OpenSANDConf::MetaComponent::addList(const string &id, const string &name, const string &pattern_name, const string &description, const string &pattern_description)
{
	if(!checkPathId(id))
	{
		return nullptr;
	}
	if(this->getItem(id) != nullptr)
	{
		return nullptr;
	}
	auto path = this->getPath() + "/" + id;
	auto pattern = shared_ptr<OpenSANDConf::MetaComponent>(new OpenSANDConf::MetaComponent("*", path, pattern_name, pattern_description, this->getTypes()));
	auto elt = shared_ptr<OpenSANDConf::MetaList>(new OpenSANDConf::MetaList(id, this->getPath(), name, description, pattern, this->getTypes()));
	this->addItem(elt);
	return elt;
}

shared_ptr<OpenSANDConf::MetaParameter> OpenSANDConf::MetaComponent::addParameter(const string &id, const string &name, shared_ptr<OpenSANDConf::MetaType> type)
{
	return this->addParameter(id, name, type, "");
}

shared_ptr<OpenSANDConf::MetaParameter> OpenSANDConf::MetaComponent::addParameter(const string &id, const string &name, shared_ptr<OpenSANDConf::MetaType> type, const string &description)
{
	if(!checkPathId(id))
	{
		return nullptr;
	}
	if(this->getItem(id) != nullptr || type == nullptr)
	{
		return nullptr;
	}
	{
		auto listtype = this->getTypes().lock()->getType(type->getId());
		if(*listtype != *type)
		{
			return nullptr;
		}
	}
	auto elt = shared_ptr<OpenSANDConf::MetaParameter>(new OpenSANDConf::MetaParameter(id, this->getPath(), name, description, type));
	this->addItem(elt);
	return elt;
}

bool OpenSANDConf::MetaComponent::equal(const OpenSANDConf::MetaElement &other) const
{
	const OpenSANDConf::MetaComponent *cpt = dynamic_cast<const OpenSANDConf::MetaComponent *>(&other);
	return cpt != nullptr && this->OpenSANDConf::MetaContainer::equal(*cpt);
}

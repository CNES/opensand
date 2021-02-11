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
 * @file MetaContainer.cpp
 * @brief Base class of all metamodel containers.
 */

#include <algorithm>

#include "MetaContainer.h"
#include "DataContainer.h"


OpenSANDConf::MetaContainer::MetaContainer(
		const std::string &id,
		const std::string &parent,
		const std::string &name,
		const std::string &description,
		std::weak_ptr<const MetaTypesList> types):
	MetaElement(id, parent, name, description),
	types(types),
	items()
{
}

OpenSANDConf::MetaContainer::MetaContainer(
		const OpenSANDConf::MetaContainer &other,
		std::weak_ptr<const MetaTypesList> types):
	MetaElement(other),
	types(types),
	items()
{
	for(auto item: other.items)
	{
		auto copy = std::dynamic_pointer_cast<MetaElement>(item->clone(types));
		if(copy != nullptr)
		{
			this->items.push_back(copy);
		}
	}
}

OpenSANDConf::MetaContainer::~MetaContainer()
{
}

std::weak_ptr<const OpenSANDConf::MetaTypesList> OpenSANDConf::MetaContainer::getTypes() const
{
	return this->types;
}

const std::vector<std::shared_ptr<OpenSANDConf::MetaElement>> &OpenSANDConf::MetaContainer::getItems() const
{
	return this->items;
}

std::shared_ptr<OpenSANDConf::MetaElement> OpenSANDConf::MetaContainer::getItem(const std::string &id) const
{
	auto elt = std::find_if(this->items.begin(), this->items.end(),
		[id](std::shared_ptr<MetaElement> elt) { return elt->getId() == id; });
	return elt != this->items.end() ? *elt : nullptr;
}

void OpenSANDConf::MetaContainer::addItem(std::shared_ptr<OpenSANDConf::MetaElement> item)
{
	this->items.push_back(item);
}

void OpenSANDConf::MetaContainer::createAndAddDataItems(
		std::shared_ptr<OpenSANDConf::DataTypesList> types,
		std::shared_ptr<DataContainer> container) const
{
	for(auto item: this->items)
	{
		container->addItem(item->createData(types));
	}
}

bool OpenSANDConf::MetaContainer::equal(const OpenSANDConf::MetaElement &other) const
{
	const OpenSANDConf::MetaContainer *cont = dynamic_cast<const OpenSANDConf::MetaContainer *>(&other);
	if(cont == nullptr || !this->OpenSANDConf::MetaElement::equal(*cont))
	{
		return false;
	}
	return this->items.size() == cont->items.size()
		&& std::equal(this->items.begin(), this->items.end(), cont->items.begin());
}

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
 * @file DataContainer.cpp
 * @brief Base class of all datamodel containers.
 */

#include <algorithm>
#include <queue>

#include "DataContainer.h"


OpenSANDConf::DataContainer::DataContainer(const std::string &id, const std::string &parent):
	DataElement(id, parent),
	items()
{
}

OpenSANDConf::DataContainer::DataContainer(const OpenSANDConf::DataContainer &other, std::shared_ptr<OpenSANDConf::DataTypesList> types):
	DataElement(other),
	items()
{
	for(auto item: other.items)
	{
		auto copy = std::dynamic_pointer_cast<DataElement>(item->clone(types));
		if(copy != nullptr)
		{
			this->items.push_back(copy);
		}
	}
}

OpenSANDConf::DataContainer::DataContainer(const std::string &id, const std::string &parent, const OpenSANDConf::DataContainer &other):
	DataElement(id, parent),
	items()
{
	for(auto item: other.items)
	{
		auto copy = std::dynamic_pointer_cast<DataElement>(item->duplicate(item->getId(), this->getPath()));
		if(copy != nullptr)
		{
			this->items.push_back(copy);
		}
	}
}

OpenSANDConf::DataContainer::~DataContainer()
{
}

bool OpenSANDConf::DataContainer::duplicateReference(std::shared_ptr<OpenSANDConf::DataElement> copy) const
{
	if(!this->OpenSANDConf::DataElement::duplicateReference(copy))
	{
		return false;
	}
	auto copy_cont = std::dynamic_pointer_cast<OpenSANDConf::DataContainer>(copy);
	if(copy_cont == nullptr || this->items.size() != copy_cont->items.size())
	{
		return false;
	}
	bool success = true;
	for(unsigned int i = 0; success && i < this->items.size(); ++i)
	{
		success = this->items[i]->duplicateReference(copy_cont->items[i]);
	}
	return success;
}

bool OpenSANDConf::DataContainer::validate() const
{
	if(!this->checkReference())
	{
		return true;
	}
	for(auto item: this->items)
	{
		if(!item->validate())
		{
			return false;
		}
	}
	return true;
}

const std::vector<std::shared_ptr<OpenSANDConf::DataElement>> &OpenSANDConf::DataContainer::getItems() const
{
	return this->items;
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataContainer::getItem(std::string id) const
{
	auto elt = std::find_if(this->items.begin(), this->items.end(),
		[id](std::shared_ptr<DataElement> elt) { return elt->getId() == id; });
	return elt != this->items.end() ? *elt : nullptr;
}

void OpenSANDConf::DataContainer::addItem(std::shared_ptr<OpenSANDConf::DataElement> item)
{
	this->items.push_back(item);
}

void OpenSANDConf::DataContainer::clearItems()
{
	this->items.clear();
}

bool OpenSANDConf::DataContainer::equal(const OpenSANDConf::DataElement &other) const
{
	const OpenSANDConf::DataContainer *cont = dynamic_cast<const OpenSANDConf::DataContainer *>(&other);
	if(cont == nullptr || !this->OpenSANDConf::DataElement::equal(*cont))
	{
		return false;
	}
	return this->items.size() == cont->items.size()
		&& std::equal(this->items.begin(), this->items.end(), cont->items.begin());
}

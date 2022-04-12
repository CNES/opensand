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
 * @file DataList.cpp
 * @brief Represents a datamodel list
 *        (holds list items following a pattern).
 */

#include <queue>
#include <sstream>

#include "DataList.h"
#include "DataComponent.h"
#include "Path.h"


OpenSANDConf::DataList::DataList(
		const std::string &id,
		const std::string &parent,
		std::shared_ptr<OpenSANDConf::DataComponent> pattern,
		std::shared_ptr<DataTypesList> types):
	OpenSANDConf::DataContainer(id, parent),
	pattern(pattern),
	types(types)
{
}

OpenSANDConf::DataList::DataList(
		const OpenSANDConf::DataList &other,
		std::shared_ptr<DataTypesList> types):
	OpenSANDConf::DataContainer(other, types),
	pattern(std::static_pointer_cast<OpenSANDConf::DataComponent>(other.pattern->clone(types))),
	types(types)
{
}

OpenSANDConf::DataList::DataList(const std::string &id, const std::string &parent, const DataList &other):
	OpenSANDConf::DataContainer(id, parent, other),
	pattern(nullptr),
	types(other.types)
{
	this->pattern = std::static_pointer_cast<OpenSANDConf::DataComponent>(other.pattern->duplicate(other.pattern->getId(), this->getPath()));
}

OpenSANDConf::DataList::~DataList()
{
}

std::shared_ptr<OpenSANDConf::DataComponent> OpenSANDConf::DataList::getPattern() const
{
	return this->pattern;
}

std::shared_ptr<OpenSANDConf::DataComponent> OpenSANDConf::DataList::addItem()
{
	std::stringstream ss;
	ss << this->getItems().size();
	auto item = std::static_pointer_cast<OpenSANDConf::DataComponent>(this->pattern->duplicate(ss.str(), this->getPath()));

	// Get pattern references
	std::queue<std::shared_ptr<DataElement>> queue;
	std::vector<std::shared_ptr<DataElement>> referenced;
	queue.push(this->pattern);
	while(!queue.empty())
	{
		// Check element has a reference
		auto elt = queue.front();
		queue.pop();
		auto target = elt->getReferenceTarget();
		if(target != nullptr)
		{
			auto common_path = getCommonPath(this->getPath(), target->getPath());
			auto remaining_ids = splitPath(getRelativePath(common_path, target->getPath()));
			if(common_path == this->getPath() && remaining_ids.front() == "*")
			{
				referenced.push_back(elt);
			}
		}
		auto cont = std::dynamic_pointer_cast<DataContainer>(elt);
		if(cont == nullptr)
		{
			continue;
		}
		for(auto elt2: cont->getItems())
		{
			queue.push(elt2);
		}
		auto lst = std::dynamic_pointer_cast<DataList>(cont);
		if(lst == nullptr)
		{
			continue;
		}
		queue.push(lst->getPattern());
	}

	// Update item references
	for(auto elt: referenced)
	{
		auto elt_path = getRelativePath(this->getPath() + "/*", elt->getPath());
		auto pattern_target = elt->getReferenceTarget();
		auto pattern_expected = elt->getReferenceData();
		auto item_elt = OpenSANDConf::DataElement::getItemFromRoot(item, elt_path, true);
		if (item_elt == nullptr)
		{
			return nullptr;
		}

		auto target_path = getRelativePath(this->getPath() + "/*", pattern_target->getPath());
		auto item_target = std::dynamic_pointer_cast<DataParameter>(OpenSANDConf::DataElement::getItemFromRoot(item, target_path, true));
		if(item_target == nullptr)
		{
			return nullptr;
		}
		item_elt->setReference(item_target);
		auto item_expected = item_elt->getReferenceData();
		if(!item_expected->copy(pattern_expected))
		{
			return nullptr;
		}
	}

	this->OpenSANDConf::DataContainer::addItem(item);
	return item;
}

void OpenSANDConf::DataList::clearItems()
{
	this->OpenSANDConf::DataContainer::clearItems();
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataList::clone(std::shared_ptr<DataTypesList> types) const
{
	return std::shared_ptr<OpenSANDConf::DataList>(new OpenSANDConf::DataList(*this, types));
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataList::duplicateObject(const std::string &id, const std::string &parent) const
{
	return std::shared_ptr<OpenSANDConf::DataList>(new OpenSANDConf::DataList(id, parent, *this));
}

bool OpenSANDConf::DataList::duplicateReference(std::shared_ptr<OpenSANDConf::DataElement> copy) const
{
	if(!this->DataContainer::duplicateReference(copy))
	{
		return false;
	}
	auto copy_lst = std::dynamic_pointer_cast<OpenSANDConf::DataList>(copy);
	if(copy_lst == nullptr || !this->pattern->duplicateReference(copy_lst->pattern))
	{
		return false;
	}
	return true;
}

bool OpenSANDConf::DataList::equal(const OpenSANDConf::DataElement &other) const
{
	const OpenSANDConf::DataList *lst = dynamic_cast<const OpenSANDConf::DataList *>(&other);
	return lst != nullptr && this->OpenSANDConf::DataContainer::equal(*lst);
}

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
 * @file DataModel.cpp
 * @brief Represents a metamodel.
 */

#include <queue>

#include "DataModel.h"
#include "DataTypesList.h"
#include "DataElement.h"


OpenSANDConf::DataModel::DataModel(const std::string &version, std::shared_ptr<OpenSANDConf::DataTypesList> types, std::shared_ptr<OpenSANDConf::DataComponent> root):
	version(version),
	types(types),
	root(root)
{
}

OpenSANDConf::DataModel::DataModel(const OpenSANDConf::DataModel &other):
	version(other.version),
	types(nullptr),
	root(nullptr)
{
	this->types = other.types->clone();
	this->root = std::static_pointer_cast<OpenSANDConf::DataComponent>(other.root->clone(this->types));
}

OpenSANDConf::DataModel::~DataModel()
{
}

std::shared_ptr<OpenSANDConf::DataModel> OpenSANDConf::DataModel::clone() const
{
	// Copy data
	auto clone = std::shared_ptr<OpenSANDConf::DataModel>(new OpenSANDConf::DataModel(*this));

	// Find references
	std::queue<std::shared_ptr<DataElement>> queue;
  std::vector<std::shared_ptr<DataElement>> referenced;
	queue.push(this->root);
	while(!queue.empty())
	{
		// Check element has a reference
		auto elt = queue.front();
		queue.pop();
		if(elt->getReferenceTarget() != nullptr)
		{
			referenced.push_back(elt);
		}
		auto cont = std::dynamic_pointer_cast<DataContainer>(elt);
		if(cont == nullptr)
		{
			continue;
		}
		auto lst = std::dynamic_pointer_cast<DataList>(cont);
		if(lst != nullptr)
		{
			queue.push(lst->getPattern());
		}
		for(auto elt2: cont->getItems())
		{
			queue.push(elt2);
		}
	}

	// Copy reference
	for(auto elt: referenced)
	{
		auto target = elt->getReferenceTarget();
		auto data = elt->getReferenceData();
		auto clone_elt = OpenSANDConf::DataElement::getItemFromRoot(clone->root, elt->getPath(), true);
		if(clone_elt == nullptr)
		{
			return nullptr;
		}
		auto clone_target = std::dynamic_pointer_cast<DataParameter>(OpenSANDConf::DataElement::getItemFromRoot(clone->root, target->getPath(), true));
		if(clone_target == nullptr)
		{
			return nullptr;
		}
		clone_elt->setReference(clone_target);
		auto clone_data = clone_elt->getReferenceData();
		if(!clone_data->copy(data))
		{
			return nullptr;
		}
	}

	return clone;
}

bool OpenSANDConf::DataModel::validate() const
{
	return this->root->validate();
}

bool OpenSANDConf::DataModel::equal(const OpenSANDConf::DataModel &other) const
{
	return this->version == this->version
		&& *(this->types) == *(other.types)
		&& *(this->root) == *(other.root);
}

const std::string &OpenSANDConf::DataModel::getVersion() const
{
	return this->version;
}

std::shared_ptr<OpenSANDConf::DataComponent> OpenSANDConf::DataModel::getRoot() const
{
	return this->root;
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataModel::getItemByPath(const std::string &path) const
{
	return OpenSANDConf::DataElement::getItemFromRoot(this->root, path, false);
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataModel::getItemByMetaPath(const std::string &path) const
{
	return OpenSANDConf::DataElement::getItemFromRoot(this->root, path, true);
}

bool OpenSANDConf::operator== (const OpenSANDConf::DataModel &v1, const OpenSANDConf::DataModel &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::DataModel &v1, const OpenSANDConf::DataModel &v2)
{
	return !v1.equal(v2);
}

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
 * @file MetaModel.cpp
 * @brief Represents a metamodel.
 */

#include <queue>
#include <sstream>

#include "MetaModel.h"
#include "DataModel.h"
#include "DataComponent.h"
#include "MetaElement.h"
#include "MetaContainer.h"
#include "MetaParameter.h"
#include "Path.h"


OpenSANDConf::MetaModel::MetaModel(const std::string &version):
	version(version),
	types(nullptr),
	root(nullptr)
{
	this->types = std::shared_ptr<OpenSANDConf::MetaTypesList>(
			new OpenSANDConf::MetaTypesList());
	this->root = std::shared_ptr<OpenSANDConf::MetaComponent>(
			new OpenSANDConf::MetaComponent("", "", "Root", "Root component", this->types));
}

OpenSANDConf::MetaModel::MetaModel(const OpenSANDConf::MetaModel &other):
	version(other.version),
	types(nullptr),
	root(nullptr)
{
	this->types = other.types->clone();
	this->root = std::static_pointer_cast<OpenSANDConf::MetaComponent>(
			other.root->clone(this->types));
}

OpenSANDConf::MetaModel::~MetaModel()
{
}

std::shared_ptr<OpenSANDConf::MetaModel> OpenSANDConf::MetaModel::clone() const
{
	// Copy data
	auto clone = std::make_shared<OpenSANDConf::MetaModel>(*this);

	// Find references
	std::queue<std::shared_ptr<MetaElement>> queue;
  std::vector<std::shared_ptr<MetaElement>> referenced;
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
		auto cont = std::dynamic_pointer_cast<MetaContainer>(elt);
		if(cont == nullptr)
		{
			continue;
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
		auto clone_elt = OpenSANDConf::MetaElement::getItemFromRoot(clone->root, elt->getPath());
		if(clone_elt == nullptr)
		{
			return nullptr;
		}
		auto clone_target = std::dynamic_pointer_cast<MetaParameter>(OpenSANDConf::MetaElement::getItemFromRoot(clone->root, target->getPath()));
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

std::shared_ptr<OpenSANDConf::DataModel> OpenSANDConf::MetaModel::createData() const
{
	// Create data
	auto datatypes = this->types->createData();
	auto dataroot = std::static_pointer_cast<OpenSANDConf::DataComponent>(this->root->createData(datatypes));

	// Find references
	std::queue<std::shared_ptr<MetaElement>> queue;
  std::vector<std::shared_ptr<MetaElement>> referenced;
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
		auto cont = std::dynamic_pointer_cast<MetaContainer>(elt);
		if(cont == nullptr)
		{
			continue;
		}
		for(auto elt2: cont->getItems())
		{
			queue.push(elt2);
		}
	}

	// Create data references
	for(auto elt: referenced)
	{
		auto target = elt->getReferenceTarget();
		auto data = elt->getReferenceData();
		if(!data->isSet())
		{
			return nullptr;
		}
		auto data_elt = OpenSANDConf::DataElement::getItemFromRoot(dataroot, elt->getPath(), true);
		if(data_elt == nullptr)
		{
			return nullptr;
		}
		auto data_target = std::dynamic_pointer_cast<DataParameter>(OpenSANDConf::DataElement::getItemFromRoot(dataroot, target->getPath(), true));
		if(data_target == nullptr)
		{
			return nullptr;
		}
		data_elt->setReference(data_target);
		auto expected_data = data_elt->getReferenceData();
		if(!expected_data->copy(data))
		{
			return nullptr;
		}
	}

	return std::shared_ptr<OpenSANDConf::DataModel>(
			new OpenSANDConf::DataModel(this->version, datatypes, dataroot));
}

bool OpenSANDConf::MetaModel::setReference(std::shared_ptr<OpenSANDConf::MetaElement> element, std::shared_ptr<OpenSANDConf::MetaParameter> target) const
{
	std::queue<std::string> remaining_ids;
  std::shared_ptr<MetaElement> elt;

	auto common_path = getCommonPath(element->getPath(), target->getPath());
	for(auto id: splitPath(getRelativePath(common_path, target->getPath())))
	{
		remaining_ids.push(id);
	}
	if(remaining_ids.empty())
	{
		return false;
	}

	elt = common_path.empty() ? this->root : OpenSANDConf::MetaElement::getItemFromRoot(this->root, common_path);
	while(elt != nullptr && !remaining_ids.empty())
	{
		auto cont = std::dynamic_pointer_cast<OpenSANDConf::MetaContainer>(elt);
		if(cont == nullptr)
		{
			elt = nullptr;
			break;
		}
		auto id = remaining_ids.front();
		remaining_ids.pop();
		elt = cont->getItem(id);
		if(elt == nullptr)
		{
			break;
		}
		auto lst = std::dynamic_pointer_cast<OpenSANDConf::MetaList>(elt);
		if(lst != nullptr)
		{
			elt = nullptr;
			break;
		}
	}
	if(elt != nullptr)
	{
		element->setReference(target);
	}
	return (elt != nullptr);
}

void OpenSANDConf::MetaModel::resetReference(std::shared_ptr<OpenSANDConf::MetaElement> element) const
{
	element->setReference(nullptr);
}

bool OpenSANDConf::MetaModel::equal(const OpenSANDConf::MetaModel &other) const
{
	return this->version == this->version
		&& this->types->equal(*(other.types))
		&& *(this->root) == *(other.root);
}

const std::string &OpenSANDConf::MetaModel::getVersion() const
{
	return this->version;
}

std::shared_ptr<OpenSANDConf::MetaTypesList> OpenSANDConf::MetaModel::getTypesDefinition() const
{
	return this->types;
}

std::shared_ptr<OpenSANDConf::MetaComponent> OpenSANDConf::MetaModel::getRoot() const
{
	return this->root;
}

std::shared_ptr<OpenSANDConf::MetaElement> OpenSANDConf::MetaModel::getItemByPath(const std::string &path) const
{
	return OpenSANDConf::MetaElement::getItemFromRoot(this->root, path);
}

bool OpenSANDConf::operator== (const OpenSANDConf::MetaModel &v1, const OpenSANDConf::MetaModel &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::MetaModel &v1, const OpenSANDConf::MetaModel &v2)
{
	return !v1.equal(v2);
}

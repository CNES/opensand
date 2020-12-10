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
 * @file MetaList.cpp
 * @brief Represents a metamodel list
 *        (holds list items following a pattern).
 */

#include "MetaList.h"
#include "MetaComponent.h"
#include "DataComponent.h"
#include "DataList.h"


OpenSANDConf::MetaList::MetaList(
		const std::string &id,
		const std::string &parent,
		const std::string &name,
		const std::string &description,
		std::shared_ptr<OpenSANDConf::MetaComponent> pattern,
		std::weak_ptr<const OpenSANDConf::MetaTypesList> types):
	OpenSANDConf::MetaContainer(id, parent, name, description, types)
{
	this->addItem(pattern);
}

OpenSANDConf::MetaList::MetaList(
		const OpenSANDConf::MetaList &other,
		std::weak_ptr<const OpenSANDConf::MetaTypesList> types):
	OpenSANDConf::MetaContainer(other, types)
{
}

OpenSANDConf::MetaList::~MetaList()
{
}

std::shared_ptr<OpenSANDConf::MetaElement> OpenSANDConf::MetaList::clone(std::weak_ptr<const OpenSANDConf::MetaTypesList> types) const
{
	return std::shared_ptr<OpenSANDConf::MetaList>(new OpenSANDConf::MetaList(*this, types));
}

std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::MetaList::createData(std::shared_ptr<OpenSANDConf::DataTypesList> types) const
{
	auto datapattern = std::static_pointer_cast<OpenSANDConf::DataComponent>(this->getPattern()->createData(types));
	return std::shared_ptr<OpenSANDConf::DataList>(new OpenSANDConf::DataList(this->getId(), this->getParentPath(), datapattern, types));
}

std::shared_ptr<OpenSANDConf::MetaComponent> OpenSANDConf::MetaList::getPattern() const
{
	return std::static_pointer_cast<MetaComponent>(this->getItems()[0]);
}

bool OpenSANDConf::MetaList::equal(const OpenSANDConf::MetaElement &other) const
{
	const OpenSANDConf::MetaList *lst = dynamic_cast<const OpenSANDConf::MetaList *>(&other);
	return lst != nullptr && this->OpenSANDConf::MetaContainer::equal(*lst);
}

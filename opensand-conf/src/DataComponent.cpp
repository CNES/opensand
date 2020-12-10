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
 * @file DataComponent.cpp
 * @brief Represents a generic datamodel component
 *        (holds a list of components, lists and parameters).
 */

#include "DataComponent.h"


OpenSANDConf::DataComponent::DataComponent(const std::string &id, const std::string &parent):
	OpenSANDConf::DataContainer(id, parent)
{
}


OpenSANDConf::DataComponent::DataComponent(
		const OpenSANDConf::DataComponent &other,
		std::shared_ptr<OpenSANDConf::DataTypesList> types):
	OpenSANDConf::DataContainer(other, types)
{
}


OpenSANDConf::DataComponent::DataComponent(
		const std::string &id,
		const std::string &parent,
		const DataComponent &other):
	OpenSANDConf::DataContainer(id, parent, other)
{
}


OpenSANDConf::DataComponent::~DataComponent()
{
}


std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataComponent::clone(std::shared_ptr<OpenSANDConf::DataTypesList> types) const
{
	return std::shared_ptr<OpenSANDConf::DataComponent>(new OpenSANDConf::DataComponent(*this, types));
}


std::shared_ptr<OpenSANDConf::DataElement> OpenSANDConf::DataComponent::duplicateObject(const std::string &id, const std::string &parent) const
{
	return std::shared_ptr<OpenSANDConf::DataComponent>(new OpenSANDConf::DataComponent(id, parent, *this));
}


std::shared_ptr<OpenSANDConf::DataParameter> OpenSANDConf::DataComponent::getParameter(const std::string &id) const
{
	return std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(this->getItem(id));
}


std::shared_ptr<OpenSANDConf::DataComponent> OpenSANDConf::DataComponent::getComponent(const std::string &id) const
{
	return std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(this->getItem(id));
}


std::shared_ptr<OpenSANDConf::DataList> OpenSANDConf::DataComponent::getList(const std::string &id) const
{
	return std::dynamic_pointer_cast<OpenSANDConf::DataList>(this->getItem(id));
}


bool OpenSANDConf::DataComponent::equal(const OpenSANDConf::DataElement &other) const
{
	const OpenSANDConf::DataComponent *cpt = dynamic_cast<const OpenSANDConf::DataComponent *>(&other);
	return cpt != nullptr && this->OpenSANDConf::DataContainer::equal(*cpt);
}

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
 * @file NamedElement.cpp
 * @brief Base class of all described elements.
 */

#include "NamedElement.h"


OpenSANDConf::NamedElement::NamedElement(
		const std::string &id,
		const std::string &name,
		const std::string &description):
	BaseElement(id),
	name(name),
	description(description)
{
}

OpenSANDConf::NamedElement::NamedElement(const OpenSANDConf::NamedElement &other):
	BaseElement(other),
	name(other.name),
	description(other.description)
{
}

OpenSANDConf::NamedElement::~NamedElement()
{
}

const std::string &OpenSANDConf::NamedElement::getName() const
{
	return this->name;
}

const std::string &OpenSANDConf::NamedElement::getDescription() const
{
	return this->description;
}

void OpenSANDConf::NamedElement::setDescription(const std::string& description)
{
	this->description = description;
}

bool OpenSANDConf::NamedElement::equal(const OpenSANDConf::NamedElement &other) const
{
	return this->getId() == other.getId()
		&& this->name == other.name
		&& this->description == other.description;
}

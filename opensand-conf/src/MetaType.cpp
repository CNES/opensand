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
 * @file MetaType.cpp
 * @brief Represents a meta type
 */

#include "MetaType.h"


OpenSANDConf::MetaType::MetaType(
		const std::string &id,
		const std::string &name,
		const std::string &description):
	NamedElement(id, name, description)
{
}

OpenSANDConf::MetaType::MetaType(const MetaType &other):
	OpenSANDConf::NamedElement(other)
{
}

OpenSANDConf::MetaType::~MetaType()
{
}

bool OpenSANDConf::MetaType::equal(const OpenSANDConf::MetaType &other) const
{
	return this->OpenSANDConf::NamedElement::equal(other);
}

bool OpenSANDConf::operator== (const OpenSANDConf::MetaType &v1, const OpenSANDConf::MetaType &v2)
{
	return v1.equal(v2);
}

bool OpenSANDConf::operator!= (const OpenSANDConf::MetaType &v1, const OpenSANDConf::MetaType &v2)
{
	return !v1.equal(v2);
}

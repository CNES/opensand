/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file TrafficCategory.cpp
 * @brief A traffic flow category regroups several traffic flows served
 *        in the same way
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#include "TrafficCategory.h"



/**
 * constructor
 */
TrafficCategory::TrafficCategory(qos_t pcp):
	id{0},
	name{},
	pcp{pcp}
{
}


/**
 * Destroy the TrafficCategory object
 */
TrafficCategory::~TrafficCategory()
{
}


void TrafficCategory::setId(qos_t id)
{
	this->id = id;
}


void TrafficCategory::setName(std::string name)
{
	this->name = name;
}


qos_t TrafficCategory::getId() const
{
	return this->id;
}


std::string TrafficCategory::getName() const
{
	return this->name;
}


qos_t TrafficCategory::getPcp() const
{
	return this->pcp;
}

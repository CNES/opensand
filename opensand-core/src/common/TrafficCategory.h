/*
 *
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
 * @file TrafficCategory.h
 * @brief A traffic flow category regroups several traffic flows served
 *        in the same way
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef TRAFFICCATEGORY_H
#define TRAFFICCATEGORY_H


#include <string>

#include "OpenSandCore.h"


/**
 * @class TrafficCategory
 * @brief A traffic flow category regroups several traffic flows served in the same way
 */
class TrafficCategory
{
public:
	TrafficCategory(qos_t pcp);
	~TrafficCategory();

	/**
	 * @brief Set the traffic caegory ID
	 *
	 * @param id  the traffic category id
	 */
	void setId(qos_t id);

	/**
	 * @brief Set the traffic category name
	 *
	 * @param name  the traffic category name
	 */
	void setName(std::string name);

	/**
	 * @brief Get the traffic category ID
	 *
	 * @return the traffic category id
	 */
	qos_t getId() const;

	/**
	 * @brief Get the traffic category name
	 *
	 * @return the traffic category name
	 */
	std::string getName() const;

	/**
	 * @brief Get the PCP value associated to this traffic category
	 *
	 * @return the PCP value for the traffic category
	 */
	qos_t getPcp() const;

private:
	/// Traffic category identifier
	qos_t id;

	/// Traffic category name
	std::string name;

	/// PCP value associated to this Traffic category
	qos_t pcp;
};

#endif

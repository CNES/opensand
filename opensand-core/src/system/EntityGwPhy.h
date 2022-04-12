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
 * @file EntityGwPhy.h
 * @brief Entity physical gateway process
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef ENTITY_GATEWAY_PHYSICAL_H
#define ENTITY_GATEWAY_PHYSICALH


#include "Entity.h"

#include <string>


/**
 * @class EntityGwPhy
 * @brief Entity physical gateway process
 */
class EntityGwPhy: public Entity
{
 public:
	/**
	 * Build an entity physical gateway process
	 */
	EntityGwPhy(tal_id_t instance_id);

	/**
	 * Destroy an entity physical gateway process
	 */
	virtual ~EntityGwPhy();

 protected:
	/**
	 * Load configuration files
	 *
	 * @param profile_path   The path to the entity configuration file
	 *
	 * @return true on success, false otherwise
	 */
	bool loadConfiguration(const std::string &profile_path);

	/**
	 * Create blocks of the specific entity process
	 *
	 * @return true on success, false otherwise
	 */
	bool createSpecificBlocks();

	/**
	 * Create configuration for the blocks of the specific entity process
	 *
	 * @param filepath   The path of the file to write the configuration into
	 *
	 * @return true on success, false otherwise
	 */
	bool createSpecificConfiguration(const std::string &filepath) const;

 private:
	void defineProfileMetaModel() const;

	std::string ip_address;
	std::string interconnect_address;
};

#endif

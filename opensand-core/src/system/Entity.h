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
 * @file Entity.h
 * @brief Entity process
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef ENTITY_H
#define ENTITY_H


#include <string>
#include <memory>

#include "OpenSandCore.h"


class OutputEvent;


/**
 * @class Entity
 * @brief Entity process
 */
class Entity
{
public:
	/**
	 * Build an entity process
	 */
	Entity(const std::string& name, tal_id_t instance_id);

	/**
	 * Destroy an entity process
	 */
	virtual ~Entity();

	/**
	 * Parse arguments of the entity process
	 *
	 * @param argc  The arguments count
	 * @param argv  The arguments list
	 *
	 * @return true on success, false otherwise
	 */
	static std::shared_ptr<Entity> parseArguments(int argc, char **argv, int &return_code);

	/**
	 * Get the entity name
	 *
	 * @return the type
	 */
	const std::string &getName() const;

	/**
	 * Get the entity id
	 *
	 * @return the type
	 */
	tal_id_t getInstanceId() const;

	/**
	 * Create blocks of the entity process
	 *
	 * @return true on success, false otherwise
	 */
	bool createBlocks();

	/**
	 * Run the entity process
	 *
	 * @return true on success, false otherwise
	 */
	bool run();

protected:
	/**
	 * Load configuration files
	 *
	 * @param profile_path   The path to the entity configuration file
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool loadConfiguration(const std::string &profile_path) = 0;

	/**
	 * Create blocks of the specific entity process
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool createSpecificBlocks() = 0;

	/**
	 * Create configuration for the blocks of the specific entity process
	 *
	 * @param filepath   The path of the file to write the configuration into
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool createSpecificConfiguration(const std::string &filepath) const = 0;

	std::string name;
	tal_id_t instance_id;

	std::shared_ptr<OutputEvent> status;
};

#endif

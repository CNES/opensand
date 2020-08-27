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
 * @file EntitySt.h
 * @brief Entity satellite terminal process
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef ENTITY_SATELLITE_TERMINAL_H
#define ENTITY_SATELLITE_TERMINAL_H


#include "Entity.h"

#include <iostream>
#include <string>
#include <vector>

#include <opensand_output/Output.h>

#include "OpenSandConf.h"

using std::string;
using std::vector;


/**
 * @class EntitySt
 * @brief Entity satellite terminal process
 */
class EntitySt: public Entity
{
 public:
	/**
	 * Build an entity satellite terminal process
	 */
	EntitySt():
		Entity("st"),
		instance_id(),
		ip_address(),
		tap_iface()
	{
	};

	/**
	 * Destroy an entity satellite terminal process
	 */
	virtual ~EntitySt() {};

	/**
	 * Generate the usage message
	 *
	 * @param progname  The program name
	 *
	 * @return the usage message
	 */
	vector<string> generateUsage(const string &progname) const;

 protected:
	/**
	 * Parse arguments of the specific entity process
	 *
	 * @param argc            The arguments count
	 * @param argv            The arguments list
	 * @param name            The entity name
	 * @param conf_path       The configuration directory path
	 * @param output_folder   The output folder path
	 * @param remote_address  The remote collector ip address
	 * @param stats_port      The remote collector port for stats
	 * @param logs_port       The remote collector port for logs
	 *
	 * @return true on success, false otherwise
	 */
	bool parseSpecificArguments(int argc, char **argv,
		string &name,
		string &conf_path,
		string &output_folder, string &remote_address,
		unsigned short &stats_port, unsigned short &logs_port);

	/**
	 * Create blocks of the specific entity process
	 *
	 * @return true on success, false otherwise
	 */
	bool createSpecificBlocks();

	tal_id_t instance_id;
	string ip_address;
	string tap_iface;
};

#endif

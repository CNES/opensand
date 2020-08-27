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


#include <iostream>
#include <string>
#include <vector>

#include <opensand_output/Output.h>

using std::string;
using std::vector;


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
	Entity(const string &type):
		type(type),
		name(),
		conf_path(),
		plugin_conf_path(),
		output_folder(),
		remote_address(),
		stats_port(12345),
		logs_port(23456)
	{
		this->status = Output::Get()->registerEvent("Status");
	};

	/**
	 * Destroy an entity process
	 */
	virtual ~Entity() {};

	/**
	 * Get the entity type
	 *
	 * @return the type
	 */
	const string &getType() const;

	/**
	 * Generate the usage message
	 *
	 * @param progname  The program name
	 *
	 * @return the usage message
	 */
	virtual vector<string> generateUsage(const string &name) const = 0;

	/**
	 * Parse arguments of the entity process
	 *
	 * @param argc  The arguments count
	 * @param argv  The arguments list
	 *
	 * @return true on success, false otherwise
	 */
	bool parseArguments(int argc, char **argv);

	/**
	 * Load configuration files
	 *
	 * @return true on success, false otherwise
	 */
	bool loadConfiguration();

	/**
	 * Load plugins
	 *
	 * @return true on success, false otherwise
	 */
	bool loadPlugins();

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

	/**
	 * Release plugins
	 */
	void releasePlugins();

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
	virtual bool parseSpecificArguments(int argc, char **argv,
		string &name,
		string &conf_path,
		string &output_folder, string &remote_address,
		unsigned short &stats_port, unsigned short &logs_port) = 0;

	/**
	 * Create blocks of the specific entity process
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool createSpecificBlocks() = 0;

 private:
	string type;
	string name;

	string conf_path;
	string plugin_conf_path;

	string output_folder;
	string remote_address;
	unsigned short stats_port;
	unsigned short logs_port;

	std::shared_ptr<OutputEvent> status;
};

#endif

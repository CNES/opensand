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
 * @file Entity.cpp
 * @brief Entity process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 */


#include "Entity.h"

#include "Plugin.h"
#include "OpenSandConf.h"

#include <opensand_old_conf/ConfigurationFile.h>

#include <opensand_rt/Rt.h>

#include <vector>
#include <map>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

using std::vector;

const string &Entity::getType() const
{
	return this->type;
}

bool Entity::parseArguments(int argc, char **argv)
{
	if(!this->parseSpecificArguments(argc, argv,
		this->name,
		this->conf_path,
		this->output_folder, this->remote_address,
		this->stats_port, this->logs_port))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: failed to init the process",
		        this->type);
		return false;
	}
	if(this->conf_path.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: missing mandatory configuration path option",
			this->type);
		return false;
	}
	if (!output_folder.empty() && !Output::Get()->configureLocalOutput(this->output_folder, this->name))
	{
		return false;
	}
	if (!remote_address.empty() && !Output::Get()->configureRemoteOutput(this->remote_address, this->stats_port, this->logs_port))
	{
		return false;
	}

	DFLTLOG(LEVEL_NOTICE, "starting output\n");

	return true;
}

bool Entity::loadConfiguration()
{
	struct sched_param param;

	string topology_file;
	string global_file;
	string default_file;

	vector<string> conf_files;
	map<string, log_level_t> levels;
	map<string, log_level_t> spec_level;

	this->plugin_conf_path = this->conf_path + "/" + string("plugins/");

	// increase the realtime responsiveness of the process
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &param);

	topology_file = this->conf_path + "/" + string(CONF_TOPOLOGY);
	global_file = this->conf_path + "/" + string(CONF_GLOBAL_FILE);
	default_file = this->conf_path + "/" + string(CONF_DEFAULT_FILE);

	conf_files.push_back(topology_file);
	conf_files.push_back(global_file);
	conf_files.push_back(default_file);

	// Load configuration files content
	if(!Conf::loadConfig(conf_files))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load configuration files, quit",
		        this->type);
		return false;
	}

	OpenSandConf::loadConfig();

	// read all packages debug levels
	if(!Conf::loadLevels(levels, spec_level))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load default levels, quit",
		        this->type);
		return false;
	}
	// Output::setLevels(levels, spec_level);
	return true;
}

bool Entity::loadPlugins()
{
	// load the plugins
	if(!Plugin::loadPlugins(true, this->plugin_conf_path))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load the plugins",
		        this->type);
		return false;
	}
	return true;
}

bool Entity::createBlocks()
{
	if(!this->createSpecificBlocks())
	{
		return false;
	}
	DFLTLOG(LEVEL_DEBUG,
	        "All blocks are created, start");
	return true;
}

bool Entity::run()
{
	// make the entity alive
	if(!Rt::init())
	{
		return false;
	}
	Output::Get()->finalizeConfiguration();
	status->sendEvent("Blocks initialized");

	if(!Rt::run())
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot run process loop",
		        this->type);
		return false;
	}
	status->sendEvent("Simulation stopped");
	return true;
}

void Entity::releasePlugins()
{
	Plugin::releasePlugins();
}

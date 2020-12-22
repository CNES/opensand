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
 * @file opensand.cpp
 * @brief OpenSAND emulator process
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include <iostream>
#include <opensand_output/Output.h>

#include "Entity.h"
#include "OpenSandModelConf.h"


int main(int argc, char **argv)
{
	int status = -1;
	auto entity = Entity::parseArguments(argc, argv, status);

	if(status != 0 || entity == nullptr)
	{
		// usage or error message handled through parseArguments
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: failed to init the process",
		        entity == nullptr ? argv[0] : entity->getName().c_str());
		return status;
	}

	DFLTLOG(LEVEL_NOTICE, "starting output\n");
	std::map<std::string, log_level_t> levels;
	std::map<std::string, log_level_t> spec_levels;
	if(!OpenSandModelConf::Get()->logLevels(levels, spec_levels))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load default levels, quit",
		        entity->getName().c_str());
		std::cerr << argv[0] << ": error: unable to load default log levels" << std::endl;
		return 100;
	}
	// Output::Get()->setLevels(levels, spec_level);

	if(!entity->loadPlugins())
	{
		std::cerr << argv[0] << ": error: unable to load plugins" << std::endl;
		return 101;
	}

	if(!entity->createBlocks())
	{
		std::cerr << argv[0] << ": error: unable to create specific blocks" << std::endl;
		entity->releasePlugins();
		return 102;
	}

	if(!entity->run())
	{
		std::cerr << argv[0] << ": error during entity execution" << std::endl;
		entity->releasePlugins();
		return 103;
	}
}

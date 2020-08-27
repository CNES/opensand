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


#include "Entity.h"

#include "EntitySat.h"
#include "EntityGw.h"
#include "EntityGwPhy.h"
#include "EntityGwNetAcc.h"
#include "EntitySt.h"

#include <string>
#include <vector>
#include <map>
#include <iostream>

using std::string;
using std::vector;
using std::map;


void usage(std::ostream &stream, const string &progname, const map<string, Entity *> &entities)
{
	std::stringstream names;
	string prefix("  ");
	vector<string> names_list;
	vector<string> usages_list;

	map<string, Entity *>::const_iterator entity_iter;
	vector<string>::const_iterator iter;

	for(entity_iter = entities.begin(); entity_iter != entities.end(); entity_iter++)
	{
		vector<string> usage;

		names_list.push_back(entity_iter->first);
		usage = entity_iter->second->generateUsage(progname);
		usage.push_back("");

		usages_list.reserve(usages_list.size() + distance(usage.begin(), usage.end()));
		usages_list.insert(usages_list.end(), usage.begin(), usage.end());
	}
	if(names_list.begin() != names_list.end())
	{
		iter = names_list.begin();
		names << *iter++;
		for(; iter != names_list.end(); iter++)
		{
			names << "|" << *iter;
		}
	}
	stream << "usage: " << progname << " [-h] [{" << names.str() << "} ...]" << std::endl;
	for(iter = usages_list.begin(); iter != usages_list.end(); iter++)
	{
		stream << prefix << *iter << std::endl;
	}
}

int main(int argc, char **argv)
{
	int status = -1;
	Entity *entity;
	map<string, Entity *> entities;
	map<string, Entity *>::iterator iter;

	entity = new EntitySat();
	entities[entity->getType()] = entity;
	entity = new EntityGw();
	entities[entity->getType()] = entity;
	entity = new EntityGwPhy();
	entities[entity->getType()] = entity;
	entity = new EntityGwNetAcc();
	entities[entity->getType()] = entity;
	entity = new EntitySt();
	entities[entity->getType()] = entity;

	// Check minimal arguments count
	if(argc < 2)
	{
		std::cerr << "Invalid arguments count" << std::endl;
 		usage(std::cerr, argv[0], entities);
		goto end;
	}

	// Check help message is required
	for(int i = 1; i < argc; ++i)
	{
		string arg(argv[i]);
		if(arg == "-h")
		{
			usage(std::cout, argv[0], entities);
			status = 0;
			goto end;
		}
	}

	// Get specific process
	iter = entities.find(string(argv[1]));
	if(iter == entities.end())
	{
		std::cerr << "Invalid command \"" << argv[1] << "\"" << std::endl;
		usage(std::cerr, argv[0], entities);
		goto end;
	}
	entity = iter->second;

	// Parse specific arguments
	if(!entity->parseArguments(argc - 1, &(argv[1])))
	{
		std::cerr << "Invalid \"" << argv[1] << "\" arguments" << std::endl;
		usage(std::cerr, argv[0], entities);
		goto end;
	}

	// Process
	if(!entity->loadConfiguration())
	{
		goto end;
	}
	if(!entity->loadPlugins())
	{
		goto end;
	}
	if(!entity->createBlocks())
	{
		entity->releasePlugins();
		goto end;
	}
	if(!entity->run())
	{
		entity->releasePlugins();
		goto end;
	}
	status = 0;

end:
	for(iter = entities.begin(); iter != entities.end(); iter++)
	{
		delete iter->second;
	}
	return status;
}

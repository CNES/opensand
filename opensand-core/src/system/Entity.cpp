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


#include <iostream>
#include <vector>
#include <map>
#include <unistd.h>

#include "Entity.h"
#include "EntityGw.h"
#include "EntityGwNetAcc.h"
#include "EntityGwPhy.h"
#include "EntitySat.h"
#include "EntitySt.h"
#include "Plugin.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>


void usage(std::ostream &stream, const std::string &progname)
{
	stream << progname << " [-h] (-i infrastructure_path -t topology_path [-p profile_path] | -g configuration_folder)" << std::endl;
	stream << "\t-h                         print this message and exit" << std::endl;
	stream << "\t-i <infrastructure_path>   path to the XML file describing the network infrastructure of the platform" << std::endl;
	stream << "\t-t <topology_path>         path to the XML file describing the satcom topology of the platform" << std::endl;
	stream << "\t-p <profile_path>          path to the XML file selecting options for this specific entity" << std::endl;
	stream << "\t-g <configuration_folder>  path to a folder where to generate XSD files for the various entities" << std::endl;
}


Entity::Entity(const std::string& name, tal_id_t instance_id): name(name), instance_id(instance_id)
{
	this->status = Output::Get()->registerEvent("Status");
}


Entity::~Entity()
{
}


const std::string &Entity::getName() const
{
	return this->name;
}


tal_id_t Entity::getInstanceId() const
{
	return this->instance_id;
}


std::shared_ptr<Entity> Entity::parseArguments(int argc, char **argv, int &return_code)
{
	int opt;
	const std::string progname = argv[0];
	std::string infrastructure_path;
	std::string topology_path;
	std::string profile_path;

	auto Conf = OpenSandModelConf::Get();
	return_code = 0;
	while((opt = getopt(argc, argv, "-hi:t:p:g:")) != EOF)
	{
		switch(opt)
		{
		case 'i':
			infrastructure_path = optarg;
			break;
		case 't':
			topology_path = optarg;
			break;
		case 'p':
			profile_path = optarg;
			break;
		case 'g':
			{
				// TODO: Error handling
				std::string folder = optarg;
				Conf->createModels();
				Conf->writeInfrastructureModel(folder + "/infrastructure.xsd");
				Conf->writeTopologyModel(folder + "/topology.xsd");

				std::shared_ptr<Entity> temporary;
				temporary = std::make_shared<EntitySat>();
				temporary->createSpecificConfiguration(folder + "/profile_sat.xsd");
				temporary = std::make_shared<EntitySt>(0);
				temporary->createSpecificConfiguration(folder + "/profile_st.xsd");
				temporary = std::make_shared<EntityGw>(0);
				temporary->createSpecificConfiguration(folder + "/profile_gw.xsd");
				temporary = std::make_shared<EntityGwNetAcc>(0);
				temporary->createSpecificConfiguration(folder + "/profile_gw_net acc.xsd");
				temporary = std::make_shared<EntityGwPhy>(0);
				temporary->createSpecificConfiguration(folder + "/profile_gw_phy.xsd");
			}
			return nullptr;
		case 'h':
		case '?':
			usage(std::cout, progname);
			return nullptr;
		default:
			usage(std::cerr, progname);
			std::cerr << "\n" << progname << ": error: unknown option '-" << (char)opt << "'." << std::endl;
			return_code = 1;
			return nullptr;
		}
	}

	if(infrastructure_path.empty())
	{
		usage(std::cerr, progname);
		std::cerr << "\n" << progname << ": error: option '-i' is missing." << std::endl;
		return_code = 2;
		return nullptr;
	}

	if(topology_path.empty())
	{
		usage(std::cerr, progname);
		std::cerr << "\n" << progname << ": error: option '-t' is missing." << std::endl;
		return_code = 3;
		return nullptr;
	}

	Conf->createModels();
	if(!Conf->readInfrastructure(infrastructure_path))
	{
		std::cerr << progname <<
		             ": error: impossible to validate network infrastructure in " <<
		             infrastructure_path << "." << std::endl;
		return_code = 10;
		return nullptr;
	}

	std::string type;
	tal_id_t entity_id;
	if(!Conf->getComponentType(type, entity_id))
	{
		if(type.empty())
		{
			std::cerr << progname << ": error: infrastructure file is missing this entity type." << std::endl;
			return_code = 11;
		}
		else
		{
			std::cerr << progname << ": error: infrastructure file is missing this entity ID." << std::endl;
			return_code = 12;
		}
		return nullptr;
	}

	std::shared_ptr<Entity> entity;
	if(type == "sat")
	{
		entity = std::make_shared<EntitySat>();
	}
	else if(type == "gw")
	{
		entity = std::make_shared<EntityGw>(entity_id);
	}
	else if(type == "gw_net_acc")
	{
		entity = std::make_shared<EntityGwNetAcc>(entity_id);
	}
	else if(type == "gw_phy")
	{
		entity = std::make_shared<EntityGwPhy>(entity_id);
	}
	else if(type == "st")
	{
		entity = std::make_shared<EntitySt>(entity_id);
	}
	else
	{
		std::cerr << progname << ": error: infrastructure file defines an entity that is not handled by this program." << std::endl;
		return_code = 13;
		return nullptr;
	}

	bool enabled = false;
	std::string output_folder;
	if(Conf->getLocalStorage(enabled, output_folder) && enabled)
	{
		// TODO: Error handling
		Output::Get()->configureLocalOutput(output_folder, entity->getName());
	}
	std::string remote_address;
	unsigned short stats_port = 12345;
	unsigned short logs_port = 23456;
	if(Conf->getRemoteStorage(enabled, remote_address, stats_port, logs_port) && enabled)
	{
		// TODO: Error handling
		Output::Get()->configureRemoteOutput(remote_address, stats_port, logs_port);
	}

	if(!Conf->readInfrastructure(topology_path))
	{
		std::cerr << progname <<
		             ": error: impossible to validate satcom topology in " <<
		             topology_path << "." << std::endl;
		return_code = 14;
		return entity;
	}

	if(!entity->loadConfiguration(profile_path))
	{
		std::cerr << progname <<
		             ": error: impossible to validate entity profile in " <<
		             profile_path << "." << std::endl;
		return_code = 15;
		return entity;
	}

	return entity;
}

bool Entity::loadPlugins()
{
	// load the plugins
	if(!Plugin::loadPlugins(true))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load the plugins",
		        this->name.c_str());
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
		        this->name.c_str());
		return false;
	}
	status->sendEvent("Simulation stopped");
	return true;
}

void Entity::releasePlugins()
{
	Plugin::releasePlugins();
}

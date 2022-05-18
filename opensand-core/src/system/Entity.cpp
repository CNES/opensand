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
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>
#include <opensand_output/OutputEvent.h>
#include <opensand_rt/Rt.h>


constexpr char OPENSAND_VERSION[] = "6.0.0";


void usage(std::ostream &stream, const std::string &progname)
{
	stream << progname << " [-h] [-v] [-V] -i infrastructure_path -t topology_path [-p profile_path]" << std::endl;
	stream << "\t-h                         print this message and exit" << std::endl;
	stream << "\t-V                         print version and exit" << std::endl;
	stream << "\t-v                         enable verbose output: logs are handed to stderr in addition" << std::endl;
	stream << "\t                           to the configuration in the infrastructure configuration file" << std::endl;
	stream << "\t-i <infrastructure_path>   path to the XML file describing the network infrastructure of the platform" << std::endl;
	stream << "\t-t <topology_path>         path to the XML file describing the satcom topology of the platform" << std::endl;
	stream << "\t-p <profile_path>          path to the XML file selecting options for this specific entity" << std::endl;
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
	bool verbose = false;

	auto Conf = OpenSandModelConf::Get();
	return_code = 0;
	while((opt = getopt(argc, argv, "-hVvi:t:p:g:")) != EOF)
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
		case 'v':
			verbose = true;
			break;
		case 'g':
			{
				// TODO: Error handling
				std::string folder = optarg;
				Conf->createModels();
				Conf->writeTopologyModel(folder + "/topology.xsd");
				Conf->writeInfrastructureModel(folder + "/infrastructure.xsd");

				auto types = Conf->getModelTypesDefinition();
				types->addEnumType("topology_xsd", "Topology XSD Files", {"topology.xsd",});
				types->addEnumType("infrastructure_xsd", "Infrastructure XSD Files", {"infrastructure.xsd",});
				types->addEnumType("profile_xsd", "Profile XSD Files", {"profile_st.xsd",
				                                                        // "profile_sat.xsd",  // -> nothing generated yet
				                                                        "profile_gw.xsd",
				                                                        "profile_gw_net_acc.xsd",
				                                                        "profile_gw_phy.xsd"});
				types->addEnumType("entity_type", "Entity Type", {"Gateway",
																  "Gateway Net Access",
																  "Gateway Phy",
																  "Satellite",
																  "Terminal"});
				types->addEnumType("upload", "Upload Method", {"Download", "NFS", "SCP", "SFTP"});
				types->addEnumType("run", "Run Method", {"LAUNCH", "STATUS", "PING", "STOP"});

				auto platform = Conf->getOrCreateComponent("platform", "Platform", "The Machines of the Project");
				auto project = platform->addParameter("project", "Project Name", types->getType("string"));
				project->setReadOnly(true);

				auto machines = platform->addList("machines", "Machines", "machine")->getPattern();
				machines->addParameter("run", "Run Method", types->getType("run"));
				machines->addParameter("entity_name", "Name", types->getType("string"))->setReadOnly(true);
				machines->addParameter("entity_type", "Type", types->getType("entity_type"))->setReadOnly(true);
				machines->addParameter("address", "[USER@]IP[:PORT]", types->getType("string"));
				machines->addParameter("upload", "Upload Method", types->getType("upload"));
				machines->addParameter("folder", "Upload Folder", types->getType("string"));

				auto configuration = Conf->getOrCreateComponent("configuration", "Configuration", "The Project Configuration Files");
				configuration->addParameter("topology__template", "Topology Template", types->getType("string"));
				auto temp = configuration->addParameter("topology", "Topology Model", types->getType("topology_xsd"));
				temp->setReadOnly(true);
				temp->setAdvanced(true);

				auto entities = configuration->addList("entities", "Entities", "entity");
				entities->setReadOnly(true);
				auto entity = entities->getPattern();
				entity->addParameter("entity_name", "Name",types->getType("string"))->setReadOnly(true);
				auto entityType = entity->addParameter("entity_type", "Type", types->getType("entity_type"));
				entityType->setReadOnly(true);
				entityType->setAdvanced(true);
				entity->addParameter("infrastructure__template", "Infrastructure Template", types->getType("string"));
				temp = entity->addParameter("infrastructure", "Infrastructure Model", types->getType("infrastructure_xsd"));
				temp->setReadOnly(true);
				temp->setAdvanced(true);
				entity->addParameter("profile__template", "Profile Template", types->getType("string"));
				temp = entity->addParameter("profile", "Profile Model", types->getType("profile_xsd"));
				temp->setReadOnly(true);
				temp->setAdvanced(true);

				Conf->writeProfileModel(folder + "/project.xsd");

				std::shared_ptr<Entity> temporary;
				/* Does not generate anything yet
				temporary = std::make_shared<EntitySat>();
				temporary->createSpecificConfiguration(folder + "/profile_sat.xsd");
				*/
				temporary = std::make_shared<EntitySt>(0);
				temporary->createSpecificConfiguration(folder + "/profile_st.xsd");
				temporary = std::make_shared<EntityGw>(0);
				temporary->createSpecificConfiguration(folder + "/profile_gw.xsd");
				temporary = std::make_shared<EntityGwNetAcc>(0);
				temporary->createSpecificConfiguration(folder + "/profile_gw_net_acc.xsd");
				temporary = std::make_shared<EntityGwPhy>(0);
				temporary->createSpecificConfiguration(folder + "/profile_gw_phy.xsd");
			}
			return nullptr;
		case 'V':
			std::cout << "OpenSAND version " << OPENSAND_VERSION << std::endl;
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

	auto output = Output::Get();
	std::map<std::string, log_level_t> levels;
	if(!Conf->logLevels(levels))
	{
		std::cerr << progname << ": error: unable to load default log levels" << std::endl;
		return_code = 101;
		return nullptr;
	}
	output->setLevels(levels);

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
		entity = std::make_shared<EntitySat>(entity_id);
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
	output->setEntityName(entity->getName());

	if (verbose)
	{
		output->configureTerminalOutput();
	}

	std::string output_folder;
	if(Conf->getLocalStorage(enabled, output_folder) && enabled)
	{
		// TODO: Error handling
		output->configureLocalOutput(output_folder);
	}
	std::string remote_address;
	unsigned short stats_port = 12345;
	unsigned short logs_port = 23456;
	if(Conf->getRemoteStorage(enabled, remote_address, stats_port, logs_port) && enabled)
	{
		// TODO: Error handling
		output->configureRemoteOutput(remote_address, stats_port, logs_port);
	}
	DFLTLOG(LEVEL_NOTICE, "starting output\n");

	if(!Conf->readTopology(topology_path))
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

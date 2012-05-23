/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 * Copyright © 2012 CNES
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
 * @file sat.cpp
 * @brief satellite emulator process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 *
 * SE uses the following stack of mgl blocs installed over 1 NIC:
 *
 * <pre>
 *
 *                +---+
 *                |   |
 *            Encap/Desencap
 *                |   |
 *             Dvb Rcs Sat
 *                |   |
 *           Sat Carrier Eth
 *                |   |
 *               eth nic
 *
 * </pre>
 *
 *
 */

// System includes
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Scheduling policy
#include <sched.h>

// Margouilla includes
#include "opensand_margouilla/mgl_eventmgr.h"
#include "opensand_margouilla/mgl_bloc.h"
#include "opensand_margouilla/mgl_blocmgr.h"

// Project includes
#include "opensand_conf/conf.h"
#include "opensand_conf/uti_debug.h"

#include "bloc_encap_sat.h"
#include "bloc_dvb_rcs_sat.h"
#include "bloc_sat_carrier.h"
#include "PluginUtils.h"

// environment plane include
#include "opensand_env_plane/EnvironmentAgent_e.h"


/// global variable for the environment agent
T_ENV_AGENT EnvAgent;

/// global variable saying whether the satellite component is alive or not
bool alive = true;


/**
 * Argument treatment
 */
bool init_process(int argc, char **argv, string &ip_addr, string &iface_name)
{
	T_INT16 scenario_id = 1, run_id = 1, opt;
	T_COMPONENT_TYPE comp_type = C_COMP_SAT;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-s:hr:a:n:i")) != EOF)
	{
		switch(opt)
		{
			case 's':
				// get scenario id
				scenario_id = atoi(optarg);
				break;
			case 'r':
				// get run id
				run_id = atoi(optarg);
				break;
			case 'a':
				/// get local IP address
				ip_addr = optarg;
				break;
			case 'n':
				// get local interface name
				iface_name = optarg;
				break;
			case 'i':
				// instance id ignored
				break;
			case 'h':
			case '?':
				fprintf(stderr, "usage: %s [-h] [-e -s scenario_id -r run_id "
				                "-a ip_address -n interface_name]\n",
				        argv[0]);
				fprintf(stderr, "\t-h              print this message\n");
				fprintf(stderr, "\t-s <scenario>   set the scenario id\n");
				fprintf(stderr, "\t-r <run>        set the run id\n");
				fprintf(stderr, "\t-a <ip_address  set the IP address\n");
				fprintf(stderr, "\t-n <interface_name   set the interface name\n");
				fprintf(stderr, "\t-i <instance>   set the instance id (ignored)\n");

				UTI_ERROR("usage printed on stderr\n");
				return false;
		}
	}

	UTI_PRINT(LOG_INFO, "starting environment plane scenario %d run %d\n",
	          scenario_id, run_id);

	// environment agent initialisation
	if(ENV_AGENT_Init(&EnvAgent, comp_type, 0, scenario_id, run_id) != C_ERROR_OK)
	{
		UTI_ERROR("failed to init the environment agent\n");
		return false;
	}

	if(ip_addr.size() == 0)
	{
		UTI_ERROR("missing mandatory IP address option");
		return false;
	}

	if(iface_name.size() == 0)
	{
		UTI_ERROR("missing mandatory interface name option");
		return false;
	}
	return true;
}


void sigendHandler(int sig)
{
	UTI_PRINT(LOG_INFO, "Signal %d received, terminate the process\n", sig);
	alive = false;
}


int main(int argc, char **argv)
{
	const char *progname = argv[0];
	struct sched_param param;
	bool is_init = false;
	string ip_addr;
	string iface_name;

	mgl_eventmgr *eventmgr;
	mgl_blocmgr *blocmgr;

	std::string satellite_type;

	BlocDVBRcsSat *blocDVBRcsSat;
	BlocEncapSat *blocEncapSat;
	BlocSatCarrier *blocSatCarrier;
	PluginUtils utils;
	vector<string> conf_files;

	std::map<std::string, EncapPlugin *> encap_plug;

	int is_failure = 1;

	// catch TERM and INT signals
	signal(SIGTERM, sigendHandler);
	signal(SIGINT, sigendHandler);

	// retrieve arguments on command line
	if(init_process(argc, argv, ip_addr, iface_name) == false)
	{
		UTI_ERROR("%s: failed to init the process\n", progname);
		goto quit;
	}

	// increase the realtime responsiveness of the process
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &param);

	conf_files.push_back(CONF_TOPOLOGY);
	conf_files.push_back(CONF_GLOBAL_FILE);
	conf_files.push_back(CONF_DEFAULT_FILE);
	// Load configuration files content
	if(!globalConfig.loadConfig(conf_files))
	{
		UTI_ERROR("%s: cannot load configuration files, quit\n",
		          progname);
		goto unload_config;
	}

	// read all packages debug levels
	UTI_readDebugLevels();

	// retrieve the type of satellite from configuration
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          satellite_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto unload_config;
	}
	UTI_PRINT(LOG_INFO, "Satellite type = %s\n", satellite_type.c_str());

	// instantiate event manager
	eventmgr = new mgl_eventmgr(realTime);
	if(eventmgr == NULL)
	{
		UTI_ERROR("%s: cannot create the event manager\n", progname);
		goto unload_config;
	}

	// instantiate bloc manager
	blocmgr = new mgl_blocmgr();
	if(blocmgr == NULL)
	{
		UTI_ERROR("%s: cannot create the bloc manager\n", progname);
		goto destroy_eventmgr;
	}

	MGL_TRACE_SET_LEVEL(0); // set mgl runtime debug level
	blocmgr->setEventMgr(eventmgr);

	// load the encapsulation plugins
	if(!utils.loadEncapPlugins(encap_plug))
	{
		UTI_ERROR("%s: cannot load the encapsulation plugins\n", progname);
		goto destroy_blocmgr;
	}

	// instantiate all blocs
	blocDVBRcsSat = new BlocDVBRcsSat(blocmgr, 0, "DVBRcsSat",
	                                  encap_plug);
	if(blocDVBRcsSat == NULL)
	{
		UTI_ERROR("%s: cannot create the DVBRcsSat bloc\n", progname);
		goto release_plugins;
	}

	if(satellite_type == REGENERATIVE_SATELLITE)
	{
		blocEncapSat = new BlocEncapSat(blocmgr, 0, "EncapSat",
		                                encap_plug);
		if(blocEncapSat == NULL)
		{
			UTI_ERROR("%s: cannot create the EncapSat bloc\n", progname);
			goto release_plugins;
		}

		blocEncapSat->setLowerLayer(blocDVBRcsSat->getId());

		blocDVBRcsSat->setUpperLayer(blocEncapSat->getId());
	}

	blocSatCarrier = new BlocSatCarrier(blocmgr, 0, "SatCarrier",
	                                    satellite, ip_addr, iface_name);
	if(blocSatCarrier == NULL)
	{
		UTI_ERROR("%s: cannot create the SatCarrier bloc\n", progname);
		goto release_plugins;
	}

	// blocs communication
	blocDVBRcsSat->setLowerLayer(blocSatCarrier->getId());
	blocSatCarrier->setUpperLayer(blocDVBRcsSat->getId());

	ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, 0, C_EVENT_STATE_INIT,
	                    C_EVENT_COMP_STATE);

	// make the SAT alive
	while(alive)
	{
		blocmgr->process_step();
		if(!is_init && blocmgr->isRunning())
		{
			ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, 0, C_EVENT_STATE_RUN,
			                    C_EVENT_COMP_STATE);
			is_init = true;
		}

	}

	ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, 0, C_EVENT_STATE_STOP,
	                    C_EVENT_COMP_STATE);

	// everything went fine, so report success
	is_failure = 0;

	// cleanup when SAT stops
release_plugins:
	utils.releaseEncapPlugins();
destroy_blocmgr:
	delete blocmgr; /* destroy the bloc manager and all the blocs */
destroy_eventmgr:
	delete eventmgr;
unload_config:
	globalConfig.unloadConfig();
	ENV_AGENT_Terminate(&EnvAgent);
quit:
	UTI_PRINT(LOG_INFO, "%s: SAT process stopped with exit code %d\n",
	          progname, is_failure);
	closelog();
	return is_failure;
}

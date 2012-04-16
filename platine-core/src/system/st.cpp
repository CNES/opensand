/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file st.cpp
 * @brief Satellite station (ST) process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 *
 * ST uses the following stack of Margouilla blocs installed over 2 NICs
 * (nic1 on user network side and nic2 on satellite network side):
 *
 * <pre>
 *
 *                     eth nic 1
 *                         |
 *                      IP QoS  --------------
 *                         |                  |
 *                   Encap/Desencap      IpMacQoSInteraction
 *                         |                  |
 *                    Dvb Rcs Tal  -----------
 *                    [Dama Agent]
 *                         |
 *                  Sat Carrier Eth
 *                         |
 *                     eth nic 2
 *
 * </pre>
 *
 */


// System includes
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// Scheduling policy
#include <sched.h>

// Margouilla includes
#include "platine_margouilla/mgl_eventmgr.h"
#include "platine_margouilla/mgl_bloc.h"
#include "platine_margouilla/mgl_blocmgr.h"

// Project includes
#include "platine_conf/uti_debug.h"
#include "platine_conf/ConfigurationFile.h"

#include "bloc_ip_qos.h"
#include "bloc_encap.h"
#include "bloc_dvb_rcs_tal.h"
#include "bloc_sat_carrier.h"
#include "PluginUtils.h"

// environment plane include
#include "platine_env_plane/EnvironmentAgent_e.h"


// global variable for the environment agent
T_ENV_AGENT EnvAgent;

/// global variable saying whether the ST component is alive or not
bool alive = true;


/**
 * Argument treatment
 */
bool init_process(int argc, char **argv,
                  string &ip_addr, tal_id_t &instance_id)
{
	T_INT16 scenario_id = 1, run_id = 1, opt;
	T_COMPONENT_TYPE comp_type = C_COMP_ST;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-s:hr:i:a:")) != EOF)
	{
		switch(opt)
		{
		case 's':
			/* get scenario id */
			scenario_id = atoi(optarg);
			break;
		case 'r':
			/* get run id */
			run_id = atoi(optarg);
			break;
		case 'i':
			/* get instance id */
			instance_id = atoi(optarg);
			break;
		case 'a':
			/// get local IP address
			ip_addr = optarg;
			break;
		case 'h':
		case '?':
			fprintf(stderr, "usage: %s [-h] [-s scenario_id -r run_id -i "
			                "instance_id -a ip_address]\n",
			        argv[0]);
			fprintf(stderr, "\t-h                   print this message\n");
			fprintf(stderr, "\t-s <scenario>        set the scenario id\n");
			fprintf(stderr, "\t-r <run>             set the run id\n");
			fprintf(stderr, "\t-a <ip_address       set the IP address\n");
			fprintf(stderr, "\t-i <instance>        set the instance id\n");

			UTI_ERROR("usage printed on stderr\n");
			return false;
		}
	}

	UTI_PRINT(LOG_INFO, "starting environment plane scenario %d run %d\n",
	          scenario_id, run_id);

	// environment agent initialisation
	if(ENV_AGENT_Init(&EnvAgent, comp_type, instance_id, scenario_id, run_id) != C_ERROR_OK)
	{
		UTI_ERROR("failed to init the environment agent\n");
		return false;
	}

	if(ip_addr.size() == 0)
	{
		UTI_ERROR("missing mandatory IP address option");
		return false;
	}

	return true;
}


// signal handler
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
	tal_id_t mac_id;

	mgl_eventmgr *eventmgr;
	mgl_blocmgr *blocmgr;

	BlocIPQoS *blocIPQoS;
	BlocEncap *blocEncap;
	BlocDVBRcsTal *blocDvbRcsTal;
	BlocSatCarrier *blocSatCarrier;
	PluginUtils utils;
	vector<string> conf_files;

	std::map<std::string, EncapPlugin *> encap_plug;

	int is_failure = 1;

	// catch TERM and INT signals
	signal(SIGTERM, sigendHandler);
	signal(SIGINT, sigendHandler);

	// retrieve arguments on command line
	if(init_process(argc, argv, ip_addr, mac_id) == false)
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

	MGL_TRACE_SET_LEVEL(0);		  // set mgl runtime debug level
	blocmgr->setEventMgr(eventmgr);

	// load the encapsulation plugins
	if(!utils.loadEncapPlugins(encap_plug))
	{
		UTI_ERROR("%s: cannot load the encapsulation plugins\n", progname);
		goto destroy_blocmgr;
	}

	// instantiate all blocs
	blocIPQoS = new BlocIPQoS(blocmgr, 0, "IP-QoS", terminal);
	if(blocIPQoS == NULL)
	{
		UTI_ERROR("%s: cannot create the IP-QoS bloc\n", progname);
		goto release_plugins;
	}

	blocEncap = new BlocEncap(blocmgr, 0, "Encap", terminal, encap_plug);
	if(blocEncap == NULL)
	{
		UTI_ERROR("%s: cannot create the Encap bloc\n", progname);
		goto release_plugins;
	}

	blocIPQoS->setLowerLayer(blocEncap->getId());
	blocEncap->setUpperLayer(blocIPQoS->getId());

	blocDvbRcsTal = new BlocDVBRcsTal(blocmgr, 0, "DvbRcsTal",
	                                  mac_id, encap_plug);
	if(blocDvbRcsTal == NULL)
	{
		UTI_ERROR("%s: cannot create the DvbRcsTal bloc\n", progname);
		goto release_plugins;
	}

	blocEncap->setLowerLayer(blocDvbRcsTal->getId());
	blocDvbRcsTal->setUpperLayer(blocEncap->getId());

	blocSatCarrier = new BlocSatCarrier(blocmgr, 0, "SatCarrier",
	                                    terminal, ip_addr);
	if(blocSatCarrier == NULL)
	{
		UTI_ERROR("%s: cannot create the SatCarrier bloc\n", progname);
		goto release_plugins;
	}

	blocDvbRcsTal->setLowerLayer(blocSatCarrier->getId());
	blocSatCarrier->setUpperLayer(blocDvbRcsTal->getId());

	// send the init event
	ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, 0, C_EVENT_STATE_INIT,
	                    C_EVENT_COMP_STATE);

	// make the ST alive
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

	// cleanup before ST stops
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
	UTI_PRINT(LOG_INFO, "%s: end of the ST process\n", progname);
	closelog();
	return is_failure;
}

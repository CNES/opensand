/**
 * @file st.cpp
 * @brief Satellite station (ST) process
 * @author AQL (sylvie)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
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

// environment plane include
#include "platine_env_plane/EnvironmentAgent_e.h"


// global variable for the environment agent
T_ENV_AGENT EnvAgent;

/// global variable saying whether the ST component is alive or not
bool alive = true;


/**
 * Argument treatment
 */
bool init_process(int argc, char **argv)
{
	T_INT16 scenario_id = 1, run_id = 1, instance_id = 0, opt;
	T_COMPONENT_TYPE comp_type = C_COMP_ST;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-s:hr:i:")) != EOF)
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
		case 'h':
		case '?':
			fprintf(stderr, "usage: %s [-h] [-s scenario_id -r run_id -i instance_id]\n",
			        argv[0]);
			fprintf(stderr, "\t-h                   print this message\n");
			fprintf(stderr, "\t-s <scenario>        set the scenario id\n");
			fprintf(stderr, "\t-r <run>             set the run id\n");
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

	mgl_eventmgr *eventmgr;
	mgl_blocmgr *blocmgr;

	BlocIPQoS *blocIPQoS;
	BlocEncap *blocEncap;
	BlocDVBRcsTal *blocDvbRcsTal;
	BlocSatCarrier *blocSatCarrier;

	int is_failure = 1;

	// catch TERM and INT signals
	signal(SIGTERM, sigendHandler);
	signal(SIGINT, sigendHandler);

	// retrieve arguments on command line
	if(init_process(argc, argv) == false)
	{
		UTI_ERROR("%s: failed to init the process\n", progname);
		goto quit;
	}

	// increase the realtime responsiveness of the process
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &param);

	// Load configuration file content
	if(globalConfig.loadConfig(CONF_DEFAULT_FILE) < 0)
	{
		UTI_ERROR("%s: cannot load config from file, quit\n", progname);
		goto term_env_agent;
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

	// instantiate all blocs
	blocIPQoS = new BlocIPQoS(blocmgr, 0, "IP-QoS", "ST");
	if(blocIPQoS == NULL)
	{
		UTI_ERROR("%s: cannot create the IP-QoS bloc\n", progname);
		goto destroy_blocmgr;
	}

	blocEncap = new BlocEncap(blocmgr, 0, "Encap");
	if(blocEncap == NULL)
	{
		UTI_ERROR("%s: cannot create the Encap bloc\n", progname);
		goto destroy_blocmgr;
	}

	blocIPQoS->setLowerLayer(blocEncap->getId());
	blocEncap->setUpperLayer(blocIPQoS->getId());

	blocDvbRcsTal = new BlocDVBRcsTal(blocmgr, 0, "DvbRcsTal");
	if(blocDvbRcsTal == NULL)
	{
		UTI_ERROR("%s: cannot create the DvbRcsTal bloc\n", progname);
		goto destroy_blocmgr;
	}

	blocEncap->setLowerLayer(blocDvbRcsTal->getId());
	blocDvbRcsTal->setUpperLayer(blocEncap->getId());

	blocSatCarrier = new BlocSatCarrier(blocmgr, 0, "SatCarrier");
	if(blocSatCarrier == NULL)
	{
		UTI_ERROR("%s: cannot create the SatCarrier bloc\n", progname);
		goto destroy_blocmgr;
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
destroy_blocmgr:
	delete blocmgr; /* destroy the bloc manager and all the blocs */
destroy_eventmgr:
	delete eventmgr;
unload_config:
	globalConfig.unloadConfig();
term_env_agent:
	ENV_AGENT_Terminate(&EnvAgent);
quit:
	UTI_PRINT(LOG_INFO, "%s: end of the ST process\n", progname);
	closelog();
	return is_failure;
}


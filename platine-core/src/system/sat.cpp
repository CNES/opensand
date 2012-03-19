/**
 * @file sat.cpp
 * @brief satellite emulator process
 * @author AQL (eddy)
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
 * based on system_ncc.cpp (aql, sylvie) and on test_sat.cpp (ASP, sebastien)
 *
 */

// System includes
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Scheduling policy
#include <sched.h>

// Margouilla includes
#include "platine_margouilla/mgl_eventmgr.h"
#include "platine_margouilla/mgl_bloc.h"
#include "platine_margouilla/mgl_blocmgr.h"

// Project includes
#include "platine_conf/conf.h"
#include "platine_conf/uti_debug.h"

#include "bloc_encap_sat.h"
#include "bloc_dvb_rcs_sat.h"
#include "bloc_sat_carrier.h"

// environment plane include
#include "platine_env_plane/EnvironmentAgent_e.h"


/// global variable for the environment agent
T_ENV_AGENT EnvAgent;

/// global variable saying whether the satellite component is alive or not
bool alive = true;


/**
 * Argument treatment
 */
bool init_process(int argc, char **argv)
{
	T_INT16 scenario_id = 1, run_id = 1, opt;
	T_COMPONENT_TYPE comp_type = C_COMP_SAT;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-s:hr:i")) != EOF)
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
			case 'i':
				// instance id ignored
				break;
			case 'h':
			case '?':
				fprintf(stderr, "usage: %s [-h] [-e -s scenario_id -r run_id]\n",
				        argv[0]);
				fprintf(stderr, "\t-h              print this message\n");
				fprintf(stderr, "\t-s <scenario>   set the scenario id\n");
				fprintf(stderr, "\t-r <run>        set the run id\n");
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

	mgl_eventmgr *eventmgr;
	mgl_blocmgr *blocmgr;

	std::string satellite_type;

	BlocDVBRcsSat *blocDVBRcsSat;
	BlocEncapSat *blocEncapSat;
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

	// load configuration file content
	if(globalConfig.loadConfig(CONF_DEFAULT_FILE) < 0)
	{
		UTI_ERROR("%s: cannot load config from file, quit\n", progname);
		goto term_env_agent;
	}

	// read all packages debug levels
	UTI_readDebugLevels();

	// retrieve the type of satellite from configuration
	if(globalConfig.getStringValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                               satellite_type) < 0)
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

	MGL_TRACE_SET_LEVEL(0);		  // set mgl runtime debug level
	blocmgr->setEventMgr(eventmgr);

	// instantiate all blocs
	blocDVBRcsSat = new BlocDVBRcsSat(blocmgr, 0, "DVBRcsSat");
	if(blocDVBRcsSat == NULL)
	{
		UTI_ERROR("%s: cannot create the DVBRcsSat bloc\n", progname);
		goto destroy_blocmgr;
	}

	if(satellite_type == REGENERATIVE_SATELLITE)
	{
		blocEncapSat = new BlocEncapSat(blocmgr, 0, "EncapSat");
		if(blocEncapSat == NULL)
		{
			UTI_ERROR("%s: cannot create the EncapSat bloc\n", progname);
			goto destroy_blocmgr;
		}

		blocEncapSat->setLowerLayer(blocDVBRcsSat->getId());

		blocDVBRcsSat->setUpperLayer(blocEncapSat->getId());
	}

	blocSatCarrier = new BlocSatCarrier(blocmgr, 0, "SatCarrier");
	if(blocSatCarrier == NULL)
	{
		UTI_ERROR("%s: cannot create the SatCarrier bloc\n", progname);
		goto destroy_blocmgr;
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
destroy_blocmgr:
	delete blocmgr; /* destroy the bloc manager and all the blocs */
destroy_eventmgr:
	delete eventmgr;
unload_config:
	globalConfig.unloadConfig();
term_env_agent:
	ENV_AGENT_Terminate(&EnvAgent);
quit:
	UTI_PRINT(LOG_INFO, "%s: SAT process stopped with exit code %d\n",
	          progname, is_failure);
	closelog();
	return is_failure;
}


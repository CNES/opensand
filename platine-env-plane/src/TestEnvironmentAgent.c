/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file TestEnvironmentAgent.c
 * @author TAS
 * @brief Test file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "EnvironmentAgent_e.h"

#define SLEEP_TIME 1

int main()
{
	T_ENV_AGENT EnvAgent;
	int i, j;
	T_COMPONENT_TYPE _ComponentType;
	T_INT32 _InstanceId;
	T_UINT16 _SimulationReference;
	T_UINT16 _SimulationRun;

	fprintf(stdout, "===========================\n");
	fprintf(stdout, "= Environment agent tests =\n");
	fprintf(stdout, "===========================\n");
	_ComponentType = C_COMP_ST;
	_InstanceId = 0;
	_SimulationReference = 1;
	_SimulationRun = 1;

	/* init */
	ENV_AGENT_Init(&EnvAgent, _ComponentType, _InstanceId, _SimulationReference,
						_SimulationRun);

	ENV_AGENT_Sync(&EnvAgent, 0, 0);

	sleep(SLEEP_TIME);

	/* error */
	fprintf(stdout, "\n--------------- start sending errors ----------------\n");
	ENV_AGENT_Error_Send(&EnvAgent, 3, 42, 1, 1);

	sleep(SLEEP_TIME);

	/* event */
	fprintf(stdout, "\n--------------- start sending events ---------------\n");
	ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, 0, C_EVENT_STATE_START,
	                    C_EVENT_COMP_STATE);
	ENV_AGENT_Event_Put(&EnvAgent, 4, 1, 2, C_EVENT_CONNECTION_START);
	ENV_AGENT_Event_Put(&EnvAgent, 4, 1, 2, C_EVENT_CONNECTION_STOP);

	ENV_AGENT_Sync(&EnvAgent, 1, 0);
	ENV_AGENT_Send(&EnvAgent);

	sleep(SLEEP_TIME);



	/* probe */
	fprintf(stdout, "\n--------------- start sending probes ---------------\n");
#ifdef TESTS
	ENV_AGENT_Probe_PutInt(&EnvAgent, 0, 5, 1000);	/* out of range */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 4, 10, 5000);	/* index 10 not defined for probe id 4 */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 5, 0, 5000);	/* no probe */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 7, 1, 2000);	/* no probe */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 1, 1, 5000);	/* index 1 not defined for probe id 6 */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 55, 1, 2000);	/* out of range */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 1, 0, 5000);	/* OK */

	ENV_AGENT_Probe_PutInt(&EnvAgent, 2, 5, 1000);	/* OK */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 2, 5, 50);	/* OK */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 2, 5, 100);	/* OK */

	ENV_AGENT_Probe_PutInt(&EnvAgent, 6, 4, 2000);	/* OK */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 6, 4, 4000);	/* OK */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 6, 4, 1000);	/* OK */

	ENV_AGENT_Probe_PutFloat(&EnvAgent, 4, 9, 3000.0);	/* OK */
	ENV_AGENT_Probe_PutFloat(&EnvAgent, 4, 9, 10.0);	/* OK */

	ENV_AGENT_Probe_PutInt(&EnvAgent, 5, 2, 2000);	/* OK */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 5, 2, 2000);	/* OK */
	ENV_AGENT_Probe_PutInt(&EnvAgent, 5, 2, 5000);	/* OK */

	/* Sends Probe value */
  /*-------------------*/
	ENV_AGENT_Sync(&EnvAgent, 4, 0);
	ENV_AGENT_Send(&EnvAgent);
	sleep(SLEEP_TIME);
#endif

	for(i = 5; i < 1000; i++)
	{

		for(j = 1; j <= 4; j++)
		{
			ENV_AGENT_Probe_PutInt(&EnvAgent, 1, j,
										  (int) (50. * sin((double) i) + 50.));
		}
		ENV_AGENT_Probe_PutFloat(&EnvAgent, 18, 0, sin((double) i) + 1.);
		ENV_AGENT_Sync(&EnvAgent, i, 0);
		ENV_AGENT_Send(&EnvAgent);
		sleep(1);
	}

	/* Remove Probe Agent */
  /*--------------------*/
	ENV_AGENT_Terminate(&EnvAgent);

	return 0;


}

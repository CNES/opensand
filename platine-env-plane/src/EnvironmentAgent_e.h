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
 * @file EnvironmentAgent_e.h
 * @author TAS
 * @brief The ExecContext class implements the execution context of template
 */

#ifndef EnvironmentAgent_e
#   define EnvironmentAgent_e

#   include "Types_e.h"
#   include "ErrorAgent_e.h"
#   include "EventAgent_e.h"
#   include "ProbeAgent_e.h"
#   include "DominoConstants_e.h"

/* define correspondance between component names and integer */
/* values used in methods implementation                     */

#   ifdef __cplusplus
extern "C"
{
#   endif

/* Environment agent class */
	typedef struct
	{
		T_COMPONENT_TYPE _ComponentType;
		T_INT32 _InstanceId;

		T_ERROR_AGENT _ErrorAgent;
		T_EVENT_AGENT _EventAgent;
		T_PROBE_AGENT _ProbeAgent;
		T_UINT32 _ProbePeriod;	  /* Probe emission period */
		T_UINT32 _EventPeriod;	  /* Event emission period */

		T_UINT32 _FSMCount;		  /* Current FSM count  [0 -> LastFSM - 1] */
		T_UINT8 _FSMId;			  /* FSM Id [0 -> FSM per Frame - 1] */
		T_UINT32 _FRSFrameCount;  /* Current FRS Frame count */

		T_UINT16 _SimReference;
		T_UINT16 _SimRun;

	} T_ENV_AGENT;


/* Active function prototype */
	typedef T_ERROR(*T_ACTIVE_FUNC) (
												  /* INOUT */ T_ENV_AGENT * ptr_ExecCxt);


/* -------------------------------------------------------------------------------------------------- */
/* Initialisation and Termination */

	T_ERROR ENV_AGENT_Init(
									 /* IN    */ T_ENV_AGENT * ptr_this,
									 /* IN    */ T_COMPONENT_TYPE ComponentType,
									 /* IN    */ T_INT32 InstanceId,
									 /* IN    */ T_UINT16 SimulationReference,
									 /* IN    */ T_UINT16 SimulationRun);

	T_ERROR ENV_AGENT_Terminate(
											/* IN    */ T_ENV_AGENT * ptr_this);

/* Send Error */
	void ENV_AGENT_SendError(
										/* IN    */ T_ENV_AGENT * ptr_this,
										/* IN    */ T_ERROR Error);

	T_CHAR *ENV_AGENT_FindComponentName(
													  /* IN    */ T_COMPONENT_TYPE
													  MyComponentType);



	T_ERROR ENV_AGENT_Error_Send( /* INOUT */ T_ENV_AGENT * ptr_this,
										  /* IN    */ T_ERROR_CATEGORY cat,
										  /* IN    */ T_ERROR_INDEX index,
										  /* IN    */ T_ERROR_VALUE value,
										  /* IN    */ T_ERROR error);

	T_ERROR ENV_AGENT_Probe_PutInt( /* IN */ T_ENV_AGENT * ptr_this,
											 /* IN */ T_UINT8 probeId,
											 /* IN */ T_UINT16 index,
											 /* IN */ T_UINT32 intValue);

	T_ERROR ENV_AGENT_Probe_PutFloat( /* IN */ T_ENV_AGENT * ptr_this,
												/* IN */ T_UINT8 probeId,
												/* IN */ T_UINT16 index,
												/* IN */ T_FLOAT floatValue);
	T_ERROR ENV_AGENT_Event_Put( /* IN */ T_ENV_AGENT * ptr_this,
										 /* IN */ T_EVENT_CATEGORY cat,
										 /* IN */ T_EVENT_INDEX index,
										 /* IN */ T_EVENT_VALUE value,
										 /* IN */ T_EVENT Event);
	T_ERROR ENV_AGENT_Send( /* IN */ T_ENV_AGENT * ptr_this);

	T_ERROR ENV_AGENT_Sync( /* IN */ T_ENV_AGENT * ptr_this,
								  /* IN */ T_UINT32 FrameCount,
								  /* IN */ T_UINT32 FSMCount);
#   ifdef __cplusplus
}										  /* end of extern "C" */
#   endif

#endif

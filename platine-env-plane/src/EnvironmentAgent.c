/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Paul LAFARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The Environnement agent class implements the execution context of template
    @HISTORY :
    03-03-21 : Creation
*/
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "Types_e.h"
#include "IPAddr_e.h"
#include "ComParameters_e.h"
#include "Error_e.h"
#include "FilePath_e.h"
#include "TracesDef_e.h"
#include "DominoConstants_e.h"
#include "EnvironmentAgent_e.h"

T_ERROR ENV_AGENT_Init(
								 /* IN    */ T_ENV_AGENT * ptr_this,
								 /* IN    */ T_COMPONENT_TYPE ComponentType,
								 /* IN    */ T_INT32 InstanceId,
								 /* IN    */ T_UINT16 SimulationReference,
								 /* IN    */ T_UINT16 SimulationRun)
{
	T_ERROR rid = C_ERROR_OK;
	T_GENERIC_PKT * genericPktError, *genericPktEvent, *genericPktProbe;
	T_UINT32 simRef;
	T_COM_PARAMETERS _ComParams;

	// initialisation */
	memset(ptr_this, 0, sizeof(T_ENV_AGENT));

	/* Init other members */
	ptr_this->_ComponentType = ComponentType;
	ptr_this->_InstanceId = InstanceId;
	ptr_this->_SimReference = SimulationReference;
	ptr_this->_SimRun = SimulationRun;
	simRef =
		((T_UINT32) (ptr_this->_SimReference) << 16) +
		(T_UINT32) (ptr_this->_SimRun);

	/* Timing */
	ptr_this->_FRSFrameCount = 0;
	ptr_this->_FSMCount = 0;
	ptr_this->_FSMId = 0;

	COM_PARAMETERS_ReadConfigFile(&_ComParams);

#ifdef _ASP_TRACE_DEBUG
	{
		int iTraceComp, jTraceComp,
			rid = TRACES_DEF_ReadConfigFile(SimulationReference, SimulationRun);
		if(rid == C_ERROR_OK)
		{
			/* trace_param is found and valid */
			/* Init level to 0 */
			for(jTraceComp = 0; jTraceComp < C_TRACE_DEF_MAX_TRACES; jTraceComp++)
			{
				TraceMode[jTraceComp] = 0;
				ComponentName[jTraceComp] = 0;
			}
			iTraceComp = 0;
			for(iTraceParam = 0; iTraceParam < trace_params._nbTrace;
				 iTraceParam++)
			{
				TRACE_ACTIVATE(trace_params._Trace[iTraceParam]._Name,
									trace_params._Trace[iTraceParam]._Mode);
				/* Find if ComponentName is allready defined */
				jTraceComp = 0;
				while((jTraceComp <= iTraceComp)
						&& (trace_params._Trace[iTraceParam]._Name !=
							 ComponentName[jTraceComp]))
				{
					jTraceComp++;
				}
				if(iTraceComp < jTraceComp)
				{
					/* Component name is NOT found */
					iTraceComp++;
					ComponentName[iTraceComp] =
						trace_params._Trace[iTraceParam]._Name;
					TraceMode[iTraceComp] = trace_params._Trace[iTraceParam]._Mode;
				}
				else
				{
					/* Component name is found */
					TraceMode[jTraceComp] =
						TraceMode[jTraceComp] | trace_params._Trace[iTraceParam].
						_Mode;
				}
			}

			/* Update TRACE LEVEL */
			for(jTraceComp = 0; jTraceComp <= iTraceComp; jTraceComp++)
			{
				TRACE_ACTIVATE(ComponentName[jTraceComp], TraceMode[jTraceComp]);
			}
		}
		else
		{
			TRACE_ACTIVATE_ALL(C_TRACE_VALID_0 | C_TRACE_VALID_1 | C_TRACE_VALID_2
									 | C_TRACE_VALID_3 | C_TRACE_VALID_4 |
									 C_TRACE_VALID_5 | C_TRACE_VALID_6 | C_TRACE_VALID_7
									 | C_TRACE_DEBUG_0 | C_TRACE_DEBUG_1 |
									 C_TRACE_DEBUG_2 | C_TRACE_DEBUG_3 | C_TRACE_DEBUG_4
									 | C_TRACE_DEBUG_5 | C_TRACE_DEBUG_6 |
									 C_TRACE_DEBUG_7 | C_TRACE_ERROR | C_TRACE_FUNC);
		}
#endif

		/* Init child agents */
		/* Error Agent init */
		rid = ERROR_AGENT_Init(&ptr_this->_ErrorAgent,
									  &_ComParams._ControllersPorts._ErrorController.
									  _IpAddress, ptr_this->_ComponentType,
									  ptr_this->_InstanceId, &(ptr_this->_FRSFrameCount),
									  &(ptr_this->_FSMId));

		/* send the init packet to the controller */
		rid = GENERIC_PACKET_MakeInit(&genericPktError, simRef, C_COMP_ERROR_CTRL);
		JUMP_ERROR(FIN, rid,
					  GENERIC_PORT_SendGenPacket(&ptr_this->_ErrorAgent._genericPort,
														  genericPktError));
		JUMP_ERROR(FIN, rid, GENERIC_PACKET_Delete(&genericPktError));

		/* Event Agent init */
		rid = EVENT_AGENT_Init(&ptr_this->_EventAgent,
									  &ptr_this->_ErrorAgent,
									  &_ComParams._ControllersPorts._EventController.
									  _IpAddress, ptr_this->_ComponentType,
									  ptr_this->_InstanceId, ptr_this->_SimReference,
									  ptr_this->_SimRun, &(ptr_this->_FRSFrameCount),
									  &(ptr_this->_FSMId));

		rid = GENERIC_PACKET_MakeInit(&genericPktEvent, simRef, C_COMP_EVENT_CTRL);
		JUMP_ERROR(FIN, rid,
					  GENERIC_PORT_SendGenPacket(&ptr_this->_EventAgent._genericPort,
														  genericPktEvent));
		JUMP_ERROR(FIN, rid, GENERIC_PACKET_Delete(&genericPktEvent));

		/* Probe Agent init */
		rid = PROBE_AGENT_Init(&ptr_this->_ProbeAgent, &ptr_this->_ErrorAgent, 2,	/* !CB ProbePeriod needs to be a parameter */
									  &_ComParams._ControllersPorts._ProbeController.
									  _IpAddress, ptr_this->_SimReference,
									  ptr_this->_SimRun, ptr_this->_ComponentType,
									  ptr_this->_InstanceId);


		rid = GENERIC_PACKET_MakeInit(&genericPktProbe, simRef, C_COMP_PROBE_CTRL);
		JUMP_ERROR(FIN, rid,
					  GENERIC_PORT_SendGenPacket(&ptr_this->_ProbeAgent._genericPort,
														  genericPktProbe));
		JUMP_ERROR(FIN, rid, GENERIC_PACKET_Delete(&genericPktProbe));

		/* add sync */
		ENV_AGENT_Sync(ptr_this, 0, 0);

		/* send the init event */
		ENV_AGENT_Event_Put(ptr_this, C_EVENT_SIMU, 0, C_EVENT_STATE_START,
		                    C_EVENT_COMP_STATE);

	 FIN:

		return rid;
	}

	T_ERROR ENV_AGENT_Terminate(
											/* IN    */ T_ENV_AGENT * ptr_this)
	{
		T_ERROR rid = C_ERROR_OK;

		PROBE_AGENT_Terminate(&(ptr_this->_ProbeAgent));
		EVENT_AGENT_Terminate(&(ptr_this->_EventAgent));
		ERROR_AGENT_Terminate(&(ptr_this->_ErrorAgent));

		return rid;
	}

	T_ENUM_COUPLE ComponentName[C_COMP_MAX + 2] = { {"GW", C_COMP_GW}
	,
	{"SAT", C_COMP_SAT}
	,
	{"ST", C_COMP_ST}
	,
	{"AGGREGATE", C_COMP_ST_AGG}
	,
	{"OBPC", C_COMP_OBPC}
	,
	{"TRAFFIC", C_COMP_TG}
	,
	{"PROBE_CONTROLLER", C_COMP_PROBE_CTRL}
	,
	{"EVENT_CONTROLLER", C_COMP_EVENT_CTRL}
	,
	{"ERROR_CONTROLLER", C_COMP_ERROR_CTRL}
	,
	{"UNKNOWN", C_COMP_MAX}
	,
	{"", 0}
	};

	T_CHAR *ENV_AGENT_FindComponentName(
													  /* IN    */ T_COMPONENT_TYPE
													  MyComponentType)
	{
		T_UINT32 i = 0;

		if(MyComponentType >= C_COMP_MAX)
			return &(ComponentName[C_COMP_MAX]._StrValue[0]);

		/* choices last value shall always be NULL */
  /*-----------------------------------------*/
		while(ComponentName[i]._StrValue[0] != '\0')
		{
			if(ComponentName[i]._IntValue == (T_INT32) MyComponentType)
				break;
			i++;
		}

		return &(ComponentName[i]._StrValue[0]);
	}

	T_ERROR ENV_AGENT_Sync(
									 /* IN */ T_ENV_AGENT * ptr_this,
									 /* IN    */ T_UINT32 FrameCount,
									 /* IN    */ T_UINT32 FSMCount)
	{
		T_ERROR rid = C_ERROR_OK;

		ptr_this->_FRSFrameCount = FrameCount;
		ptr_this->_FSMCount = FSMCount;
		//  ptr_this->_FSMId = 0;

		return (rid);
	}
	T_ERROR ENV_AGENT_Error_Send(
											 /* INOUT */ T_ENV_AGENT * ptr_this,
											 /* IN    */ T_ERROR_CATEGORY cat,
											 /* IN    */ T_ERROR_INDEX index,
											 /* IN    */ T_ERROR_VALUE value,
											 /* IN    */ T_ERROR error)
	{
		return ERROR_AGENT_SetLastError(&ptr_this->_ErrorAgent, cat, index, value,
												  error);

	}

	T_ERROR ENV_AGENT_Probe_PutInt( /* IN */ T_ENV_AGENT * ptr_this,
											 /* IN */ T_UINT8 probeId,
											 /* IN */ T_UINT16 index,
											 /* IN */ T_UINT32 intValue)
	{
		return PROBE_AGENT_PutProbe(&ptr_this->_ProbeAgent, probeId, index,
											 ptr_this->_FRSFrameCount, intValue, 0.0);
	}

	T_ERROR ENV_AGENT_Probe_PutFloat( /* IN */ T_ENV_AGENT * ptr_this,
												/* IN */ T_UINT8 probeId,
												/* IN */ T_UINT16 index,
												/* IN */ T_FLOAT floatValue)
	{
		return PROBE_AGENT_PutProbe(&ptr_this->_ProbeAgent, probeId, index,
											 ptr_this->_FRSFrameCount, 0, floatValue);
	}

	T_ERROR ENV_AGENT_Event_Put( /* IN */ T_ENV_AGENT * ptr_this,
										 /* IN */ T_EVENT_CATEGORY cat,
										 /* IN */ T_EVENT_INDEX index,
										 /* IN */ T_EVENT_VALUE value,
										 /* IN */ T_EVENT Event)
	{
		T_ERROR rid;

		rid =  EVENT_AGENT_SetLastEvent(&(ptr_this->_EventAgent), cat, index,
		                                value, Event);

		EVENT_AGENT_SendAllEvents(&ptr_this->_EventAgent);

		return rid;
	}

	T_ERROR ENV_AGENT_Send( /* IN */ T_ENV_AGENT * ptr_this)
	{
		T_ERROR rid;

		/* send all probes */
		rid =
			PROBE_AGENT_SendAllProbes(&ptr_this->_ProbeAgent,
											  ptr_this->_FRSFrameCount);

		/* send all events */
		rid = EVENT_AGENT_SendAllEvents(&ptr_this->_EventAgent);

		return (rid);
	}

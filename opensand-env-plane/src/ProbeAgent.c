/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file ProbeAgent.c
 * @author TAS
 * @brief The ProbeAgent class implements the probe agent services
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "ProbesDef_e.h"
#include "ProbesActivation_e.h"
#include "GenericPacket_e.h"
#include "ProtoConstants_e.h"
#include "GenericPort_e.h"
#include "IPAddr_e.h"
#include "ErrorAgent_e.h"
#include "ProbeAgent_e.h"
#include "ProbeHolder_e.h"

/*  @ROLE    : This function initialises the probe agent
    @RETURN  : Error code */
T_ERROR PROBE_AGENT_Init(
									/* INOUT */ T_PROBE_AGENT * ptr_this,
									/* IN    */ T_ERROR_AGENT * ptr_errorAgent,
									/* IN    */ T_UINT32 ProbePeriod,
									/* IN    */ T_IP_ADDR * ptr_ipAddr,
									/* IN    */ T_UINT16 SimReference,
									/* IN    */ T_UINT16 SimRun,
									/* IN    */ T_COMPONENT_TYPE ComponentType,
									/* IN    */ T_INT32 InstanceId)
{
	T_ERROR rid = C_ERROR_OK;
#ifndef _ASP_NO_PROBE
	T_PROBES_DEF * ptr_probesDef = NULL;
	T_UINT32 i;
	T_UINT32 iGenPacket;
#endif /* _ASP_NO_PROBE */

	memset(ptr_this, 0, sizeof(T_PROBE_AGENT));

#ifndef _ASP_NO_PROBE
	ptr_probesDef = malloc(sizeof(T_PROBES_DEF));
	if(ptr_probesDef == NULL)
	{
		SEND_ERRNO_JUMP(FIN2, rid, C_ERROR_ALLOC,
							 ptr_errorAgent, C_ERROR_CRITICAL, 0,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
							  C_TRACE_ERROR, "malloc() failed"));
	}
	memset(ptr_probesDef, 0, sizeof(T_PROBES_DEF));

	/* Strore init value */
  /*-------------------*/
	ptr_this->_ptr_errorAgent = ptr_errorAgent;

	/* Allocate the generic packet */
  /*-----------------------------*/
	for(iGenPacket = 0; iGenPacket < C_MAX_PROBE_GEN_PACKET; iGenPacket++)
	{
		SEND_ERRNO_JUMP(FIN, rid,
							 GENERIC_PACKET_Create(&
														  (ptr_this->
															_ptr_genPacket[iGenPacket]),
														  C_MAX_PROBE_VALUE_NUMBER),
							 ptr_errorAgent, C_ERROR_CRITICAL, 0,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
							  C_TRACE_ERROR, "GENERIC_PACKET_Create() failed"));

		/* Init generic packet header */
	 /*----------------------------*/
		ptr_this->_ptr_genPacket[iGenPacket]->_componentId =
			MAKE_COMPONENT_ID(ComponentType, InstanceId);
		ptr_this->_ptr_genPacket[iGenPacket]->_FSMNumber = 0;
		ptr_this->_genPacketIsFree[iGenPacket] = TRUE;
	}

	/* Init Generic Packet Socket */
  /*----------------------------*/
	SEND_ERRNO_JUMP(FIN, rid,
						 GENERIC_PORT_InitSender(&(ptr_this->_genericPort),
														 ptr_ipAddr,
														 C_MAX_PROBE_VALUE_NUMBER),
						 ptr_errorAgent, C_ERROR_CRITICAL, C_II_P_SOCKET,
						 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
						  "GENERIC_PORT_InitSender() failed"));

	/* Read probe defintion file */
  /*---------------------------*/
	SEND_ERRNO_JUMP(FIN, rid,
						 PROBES_DEF_ReadConfigFile(ptr_probesDef, ComponentType),
						 ptr_errorAgent, C_ERROR_CRITICAL, C_PROBE_DEF_FILE,
						 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
						  "PROBES_DEF_ReadConfigFile() failed"));

	/* Init probe data */
  /*-----------------*/
	JUMP_ERROR_TRACE(FIN, rid, PROBE_HOLDER_Init(&(ptr_this->_probeData), ptr_probesDef, ComponentType, SimReference, SimRun, FALSE,	/* Probe Agent holder */
																ptr_errorAgent),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
							"PROBE_HOLDER_Init() failed"));

	/* Init internal data */
  /*--------------------*/
	ptr_this->_probePeriodCmpt = ProbePeriod;
	ptr_this->_ProbePeriod = ProbePeriod;
	ptr_this->_probeValueChgIndex = 0;
	memset(&(ptr_this->_ptr_probeValueChg[0]), 0,
			 sizeof(T_PROBE_VALUE *) * (C_MAX_PROBE_VALUE_NUMBER));

	/* Set probe ptr to NULL */
	for(i = 0; i < C_MAX_PROBE_VALUE_NUMBER; i++)
	{
		ptr_this->_ptr_probeValueChg[i] = NULL;
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PROBE_AGENT_Init() sucessful"));

 FIN:
	free(ptr_probesDef);

 FIN2:
#endif /* _ASP_NO_PROBE */
	return rid;
}


/*  @ROLE    : This function terminates the probe agent
    @RETURN  : Error code */
T_ERROR PROBE_AGENT_Terminate(
										  /* INOUT */ T_PROBE_AGENT * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

#ifndef _ASP_NO_PROBE
	T_UINT32 iGenPacket;
	/* Send all probes not sent (all that can be sent) */
	ptr_this->_probePeriodCmpt = 1;
	PROBE_AGENT_ThreadSendAllProbes(ptr_this);

	for(iGenPacket = 0; iGenPacket < C_MAX_PROBE_GEN_PACKET; iGenPacket++)
	{
		SEND_ERRNO(rid,
					  GENERIC_PACKET_Delete(&(ptr_this->_ptr_genPacket[iGenPacket])),
					  ptr_this->_ptr_errorAgent, C_ERROR_CRITICAL, 0,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
						"GENERIC_PACKET_Delete() failed"));
	}

	SEND_ERRNO(rid, PROBE_HOLDER_Terminate(&(ptr_this->_probeData)),
				  ptr_this->_ptr_errorAgent, C_ERROR_CRITICAL, 0,
				  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
					"PROBE_HOLDER_Terminate() failed"));

	SEND_ERRNO(rid, GENERIC_PORT_Terminate(&(ptr_this->_genericPort)),
				  ptr_this->_ptr_errorAgent, C_ERROR_CRITICAL, C_II_P_SOCKET,
				  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
					"GENERIC_PORT_Terminate() failed"));

	ptr_this->_ptr_errorAgent = NULL;

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PROBE_AGENT_Terminate() sucessful"));
#endif /* _ASP_NO_PROBE */

	return rid;
}


/*  @ROLE    : is used to put probe information
    @RETURN  : Error code */
T_ERROR PROBE_AGENT_PutProbe(
										 /* INOUT */ T_PROBE_AGENT * ptr_this,
										 /* IN    */ T_UINT8 probeId,
										 /* IN    */ T_UINT16 index,
										 /* IN    */ T_UINT32 FrameCount,
										 /* IN    */ T_UINT32 intValue,
										 /* IN    */ T_FLOAT floatValue)
{
#ifndef _ASP_NO_PROBE
	T_ERROR rid = C_ERROR_OK;

	if(ptr_this->_probeData._statIsActivated == TRUE)
	{
#   ifdef _ASP_PROBE_CHECK
		/* check the probeId */
	 /*-------------------*/
		if((probeId <= ptr_this->_probeData._nbStat) && (probeId != 0))
		{
#   endif
		 /* _ASP_PROBE_CHECK */

			/* check if the statistic is activated */
		/*-------------------------------------*/
			if(ptr_this->_probeData._probeInfo[probeId]._activate)
			{

#   ifdef _ASP_PROBE_CHECK
				/* check index */
		  /*-------------*/
				if((index <= ptr_this->_probeData._probeInfo[probeId]._nbLabels)
					&& ((index != 0)
						 || (ptr_this->_probeData._probeInfo[probeId]._nbLabels ==
							  0)))
				{
#   endif
		 /* _ASP_PROBE_CHECK */

					/* check the frame number */
			 /*------------------------*/
					if((FrameCount >= ptr_this->_probeData._startFrame)
						&& (FrameCount <= ptr_this->_probeData._stopFrame))
					{

						/* update the probe change table */
				/*-------------------------------*/
						if(ptr_this->_probeData._ptr_probeValue[probeId][index].
							_valueNumber == 0)
						{
							/* check the max change value */
							if(ptr_this->_probeValueChgIndex <
								C_MAX_PROBE_VALUE_NUMBER)
							{
								ptr_this->_ptr_probeValueChg[ptr_this->
																	  _probeValueChgIndex] =
									&(ptr_this->_probeData.
									  _ptr_probeValue[probeId][index]);
								ptr_this->_probeValueChgIndex++;
							}
							else
							{
								SEND_ERRNO_JUMP(FIN, rid, C_ERROR_BUF_OVERFLOW,
													 ptr_this->_ptr_errorAgent,
													 C_ERROR_MINOR, C_PROBE_COMMAND,
													 (C_TRACE_THREAD_UNKNOWN,
													  C_TRACE_COMP_PROBE, C_TRACE_ERROR,
													  "PROBE_AGENT_PutProbe() cannot store probe:max change value is reached"));
							}
						}

						/* store the statistic value */
				/*---------------------------*/
						switch (ptr_this->_probeData._probeInfo[probeId].
								  _aggregationMode)
						{
						case C_AGG_MIN:
							if(ptr_this->_probeData._probeInfo[probeId]._type ==
								C_PROBE_TYPE_INT)
							{
								ptr_this->_probeData._ptr_probeValue[probeId][index].
									_intValue =
									OPERATOR_MIN(ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _intValue, intValue,
													 ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _valueNumber);
							}
							else
							{
								ptr_this->_probeData._ptr_probeValue[probeId][index].
									_floatValue =
									OPERATOR_MIN(ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _floatValue, floatValue,
													 ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _valueNumber);
							}
							break;
						case C_AGG_MAX:
							if(ptr_this->_probeData._probeInfo[probeId]._type ==
								C_PROBE_TYPE_INT)
							{
								ptr_this->_probeData._ptr_probeValue[probeId][index].
									_intValue =
									OPERATOR_MAX(ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _intValue, intValue,
													 ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _valueNumber);
							}
							else
							{
								ptr_this->_probeData._ptr_probeValue[probeId][index].
									_floatValue =
									OPERATOR_MAX(ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _floatValue, floatValue,
													 ptr_this->_probeData.
													 _ptr_probeValue[probeId][index].
													 _valueNumber);
							}
							break;
						case C_AGG_MEAN:
							if(ptr_this->_probeData._probeInfo[probeId]._type ==
								C_PROBE_TYPE_INT)
							{
								OPERATOR_MEAN(ptr_this->_probeData.
												  _ptr_probeValue[probeId][index]._intValue,
												  intValue);
							}
							else
							{
								OPERATOR_MEAN(ptr_this->_probeData.
												  _ptr_probeValue[probeId][index].
												  _floatValue, floatValue);
							}
							break;
						case C_AGG_LAST:
							if(ptr_this->_probeData._probeInfo[probeId]._type ==
								C_PROBE_TYPE_INT)
							{
								OPERATOR_LAST(ptr_this->_probeData.
												  _ptr_probeValue[probeId][index]._intValue,
												  intValue);
							}
							else
							{
								OPERATOR_LAST(ptr_this->_probeData.
												  _ptr_probeValue[probeId][index].
												  _floatValue, floatValue);
							}
							break;
						case C_AGG_NB:
							/* not a real value, max of the enum, nothing to do */
							break;
						}

						ptr_this->_probeData._ptr_probeValue[probeId][index].
							_valueNumber++;

#   ifdef _ASP_TRACE
						if(ptr_this->_probeData._probeInfo[probeId]._type ==
							C_PROBE_TYPE_INT)
						{
							TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
										  C_TRACE_DEBUG,
										  "PROBE_AGENT_PutProbe() cmpt(%#x) probe(%d) index(%d) value(%d)",
										  ptr_this->_ptr_genPacket[0]->_componentId,
										  probeId, index, intValue));
						}
						else
						{
							TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
										  C_TRACE_DEBUG,
										  "PROBE_AGENT_PutProbe() cmpt(%#x) probe(%d) index(%d) value(%.03e)",
										  ptr_this->_ptr_genPacket[0]->_componentId,
										  probeId, index, floatValue));
						}
#   endif
		 /* _ASP_TRACE */
					}
#   ifdef _ASP_PROBE_CHECK
				}
				else
				{
					SEND_ERRNO_JUMP(FIN, rid, C_ERROR_BAD_PARAM,
										 ptr_this->_ptr_errorAgent, C_ERROR_MINOR,
										 C_PROBE_COMMAND, (C_TRACE_THREAD_UNKNOWN,
																 C_TRACE_COMP_PROBE,
																 C_TRACE_ERROR,
																 "PROBE_AGENT_PutProbe() index %d not defined for probe id %d",
																 index, probeId));
				}
#   endif
		 /* _ASP_PROBE_CHECK */
			}
#   ifdef _ASP_PROBE_CHECK
		}
		else
		{
			SEND_ERRNO_JUMP(FIN, rid, C_ERROR_BAD_PARAM,
								 ptr_this->_ptr_errorAgent, C_ERROR_MINOR,
								 C_PROBE_COMMAND, (C_TRACE_THREAD_UNKNOWN,
														 C_TRACE_COMP_PROBE, C_TRACE_ERROR,
														 "PROBE_AGENT_PutProbe() probe id %d out of range",
														 probeId));
		}
#   endif
		 /* _ASP_PROBE_CHECK */
	}

 FIN:
#endif /* _ASP_NO_PROBE */
	return C_ERROR_OK;
}

/*  @ROLE    : is used to send all probe info to the probe controller
    @RETURN  : Error code */
T_ERROR PROBE_AGENT_SendAllProbes(
												/* INOUT */ T_PROBE_AGENT * ptr_this,
												/* IN    */ T_UINT32 FrameCount
	)
{
	T_ERROR rid = C_ERROR_OK;
#ifndef _ASP_NO_PROBE
	T_UINT16 i;
	T_PROBE_VALUE * ptr_probeValue;
	T_ELT_GEN_PKT * eltGenPkt;
	T_UINT32 iGenPacket = 0;

	/* check the probe period */
	if(ptr_this->_probePeriodCmpt == 0)
		goto FIN;
	ptr_this->_probePeriodCmpt--;

	if(ptr_this->_probePeriodCmpt == 0)
	{
		ptr_this->_probePeriodCmpt = ptr_this->_ProbePeriod;

		/* Init generic packet header */
	 /*----------------------------*/

		while((!ptr_this->_genPacketIsFree[iGenPacket])
				&& (iGenPacket < C_MAX_PROBE_GEN_PACKET))
		{
			iGenPacket++;
		}
		if(iGenPacket == C_MAX_PROBE_GEN_PACKET)
		{
			SEND_ERRNO_JUMP(FIN, rid, C_ERROR_ALLOC,
								 ptr_this->_ptr_errorAgent, C_ERROR_CRITICAL,
								 C_PROBE_COMMAND, (C_TRACE_THREAD_UNKNOWN,
														 C_TRACE_COMP_PROBE, C_TRACE_ERROR,
														 "PROBE_AGENT_SendAllProbes No generic packet is free (C_MAX_PROBE_GEN_PACKET=%u)",
														 C_MAX_PROBE_GEN_PACKET));
		}

		ptr_this->_ptr_genPacket[iGenPacket]->_elementNumber =
			ptr_this->_probeValueChgIndex;
		ptr_this->_ptr_genPacket[iGenPacket]->_frameNumber = FrameCount;

		/* Init generic packet elmt */
	 /*--------------------------*/
		for(i = 0; i < ptr_this->_probeValueChgIndex; i++)
		{
			SEND_ERRNO_JUMP(FIN, rid,
								 GENERIC_PACKET_GetEltPkt(ptr_this->
																  _ptr_genPacket[iGenPacket], i,
																  &eltGenPkt),
								 ptr_this->_ptr_errorAgent, C_ERROR_CRITICAL,
								 C_PROBE_COMMAND, (C_TRACE_THREAD_UNKNOWN,
														 C_TRACE_COMP_PROBE, C_TRACE_ERROR,
														 "GENERIC_PACKET_GetEltPkt() cannot get elt generic packet n°%d",
														 i));

			/* update packet value */
		/*---------------------*/
			ptr_probeValue = ptr_this->_ptr_probeValueChg[i];
			eltGenPkt->_id = ptr_probeValue->_probeId;
			eltGenPkt->_categoryId =
				ptr_this->_probeData._probeInfo[ptr_probeValue->_probeId].
				_categoryId;
			eltGenPkt->_index = ptr_probeValue->_index;
			switch (ptr_this->_probeData._probeInfo[ptr_probeValue->_probeId].
					  _aggregationMode)
			{
			case C_AGG_MEAN:
				if(ptr_this->_probeData._probeInfo[ptr_probeValue->_probeId].
					_type == C_PROBE_TYPE_INT)
				{
					OPERATOR_END_MEAN_INT(ptr_probeValue->_intValue,
												 ptr_probeValue->_valueNumber);
				}
				else
				{
					OPERATOR_END_MEAN_FLOAT(ptr_probeValue->_floatValue,
													ptr_probeValue->_valueNumber);
				}
				break;
			case C_AGG_MIN:
			case C_AGG_MAX:
			case C_AGG_LAST:
			case C_AGG_NB:
				/* nothing to do */
				break;
			}
			if(ptr_this->_probeData._probeInfo[ptr_probeValue->_probeId]._type ==
			   C_PROBE_TYPE_INT)
				eltGenPkt->_value = ptr_probeValue->_intValue;
			else
				eltGenPkt->_value = (T_UINT32) ptr_probeValue->_floatValue;

#   ifdef _ASP_TRACE
			if(ptr_this->_probeData._probeInfo[ptr_probeValue->_probeId]._type ==
				C_PROBE_TYPE_INT)
			{
				TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
							  C_TRACE_DEBUG,
							  "PROBE_AGENT_SendAllProbes() nb(%d) cmpt(%#x) probe(%d) "
							  "index(%d) value(%d) frame(%d)", i,
							  ptr_this->_ptr_genPacket[iGenPacket]->_componentId,
							  eltGenPkt->_id, eltGenPkt->_index,
							  ptr_probeValue->_intValue,
							  ptr_this->_ptr_genPacket[iGenPacket]->_frameNumber));
			}
			else
			{
				TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
							  C_TRACE_DEBUG,
							  "PROBE_AGENT_SendAllProbes() nb(%d) cmpt(%#x) probe(%d) "
							  "index(%d) value(%.03e) frame(%d)", i,
							  ptr_this->_ptr_genPacket[iGenPacket]->_componentId,
							  eltGenPkt->_id, eltGenPkt->_index,
							  ptr_probeValue->_floatValue,
							  ptr_this->_ptr_genPacket[iGenPacket]->_frameNumber));
			}
#   endif
		 /* _ASP_TRACE */

			/* Reinit statistic value */
		/*------------------------*/
			ptr_probeValue->_intValue = 0;
			ptr_probeValue->_floatValue = 0.0;
			ptr_probeValue->_valueNumber = 0;
			ptr_this->_ptr_probeValueChg[i] = NULL;

#   ifdef truc
			/* send Packet */
	 /*-------------*/
			//        if(ptr_this->_probeValueChgIndex != 0) {
			SEND_ERRNO_JUMP(FIN, rid,
								 GENERIC_PORT_SendGenPacket(&(ptr_this->_genericPort),
																	 ptr_this->
																	 _ptr_genPacket[iGenPacket]),
								 ptr_this->_ptr_errorAgent, C_ERROR_CRITICAL,
								 C_II_P_SOCKET, (C_TRACE_THREAD_UNKNOWN,
													  C_TRACE_COMP_PROBE, C_TRACE_ERROR,
													  "PROBE_AGENT_SendAllProbes() cannot send packet"));
			ptr_this->_genPacketIsFree[iGenPacket] = TRUE;
			//  }

#   endif
		}

		if(ptr_this->_probeValueChgIndex != 0)
		{
			ptr_this->_IsToSend = TRUE;
			ptr_this->_genPacketIsFree[iGenPacket] = FALSE;
		}


		/* Reinit statistic value */
	 /*------------------------*/
		ptr_this->_probeValueChgIndex = 0;


		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
					  "PROBE_AGENT_SendAllProbes() sucessful"));
	}

	PROBE_AGENT_ThreadSendAllProbes(ptr_this);

 FIN:
#endif /* _ASP_NO_PROBE */
	return rid;
}

/*  @ROLE    : is used to send all probe info to the probe controller
@RETURN  : Error code */
T_ERROR PROBE_AGENT_ThreadSendAllProbes(
														/* INOUT */ T_PROBE_AGENT * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
#ifndef _ASP_NO_PROBE
	T_UINT32 iGenPacket = 0;

	if(ptr_this->_IsToSend)
	{
		/* Find the generic packet to send */
		for(iGenPacket = 0; iGenPacket < C_MAX_PROBE_GEN_PACKET; iGenPacket++)
		{
			if(!ptr_this->_genPacketIsFree[iGenPacket])
			{
				/* send Packet */
		  /*-------------*/
				SEND_ERRNO_JUMP(FIN, rid,
									 GENERIC_PORT_SendGenPacket(&
																		 (ptr_this->_genericPort),
																		 ptr_this->
																		 _ptr_genPacket
																		 [iGenPacket]),
									 ptr_this->_ptr_errorAgent, C_ERROR_CRITICAL,
									 C_II_P_SOCKET, (C_TRACE_THREAD_UNKNOWN,
														  C_TRACE_COMP_PROBE, C_TRACE_ERROR,
														  "PROBE_AGENT_SendAllProbes() cannot send packet"));

				ptr_this->_genPacketIsFree[iGenPacket] = TRUE;
			}
		}
	}

	ptr_this->_IsToSend = FALSE;
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PROBE_AGENT_SendAllProbes() sucessful"));

 FIN:
#endif /* _ASP_NO_PROBE */
	return rid;
}

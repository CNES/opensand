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
 * @file ProbeAgent_e.h
 * @author TAS
 * @brief The ProbeAgent class implements the probe agent services
 */

#ifndef ProbeAgent_e
#   define ProbeAgent_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "Probe_e.h"
#   include "GenericPort_e.h"
#   include "IPAddr_e.h"
#   include "GenericPacket_e.h"
#   include "ProbesDef_e.h"
#   include "ProbeHolder_e.h"

#   define C_MAX_PROBE_GEN_PACKET 5
											/* Max of generic packet used for thread agent */

typedef struct
{
	T_GENERIC_PORT _genericPort;
	T_ERROR_AGENT *_ptr_errorAgent;
	T_GENERIC_PKT *_ptr_genPacket[C_MAX_PROBE_GEN_PACKET];
	T_BOOL _genPacketIsFree[C_MAX_PROBE_GEN_PACKET];
	T_PROBE_HOLDER _probeData;
	T_UINT16 _probeValueChgIndex;
	T_PROBE_VALUE *_ptr_probeValueChg[C_MAX_PROBE_VALUE_NUMBER];
	T_UINT32 _probePeriodCmpt;	  /* Probe emission period */
	T_UINT32 _ProbePeriod;		  /* Probe emission period */
	T_BOOL _IsToSend;

} T_PROBE_AGENT;


/*  @ROLE    : This function initialises the probe agent
    @RETURN  : Error code */
extern T_ERROR PROBE_AGENT_Init(
											 /* INOUT */ T_PROBE_AGENT * ptr_this,
											 /* IN    */ T_ERROR_AGENT * ptr_errorAgent,
											 /* IN    */ T_UINT32 ProbePeriod,
											 /* IN    */ T_IP_ADDR * ptr_ipAddr,
											 /* IN    */ T_UINT16 SimReference,
											 /* IN    */ T_UINT16 SimRun,
											 /* IN    */ T_COMPONENT_TYPE ComponentType,
											 /* IN    */ T_INT32 InstanceId);


/*  @ROLE    : This function terminates the probe agent
    @RETURN  : Error code */
extern T_ERROR PROBE_AGENT_Terminate(
													/* INOUT */ T_PROBE_AGENT * ptr_this);


/*  @ROLE    : is used to put probe information
    @RETURN  : Error code */
#   define PROBE_AGENT_PutIntProbe(ptr_this,probeId,index,frame,intValue) \
   PROBE_AGENT_PutProbe(ptr_this,probeId,index,frame,intValue,0)
#   define PROBE_AGENT_PutFloatProbe(ptr_this,probeId,index,frame,floatValue) \
   PROBE_AGENT_PutProbe(ptr_this,probeId,index,frame,0,floatValue)
extern T_ERROR PROBE_AGENT_PutProbe(
												  /* INOUT */ T_PROBE_AGENT * ptr_this,
												  /* IN    */ T_UINT8 probeId,
												  /* IN    */ T_UINT16 index,
												  /* IN    */ T_UINT32 FrameCount,
												  /* IN    */ T_UINT32 intValue,
												  /* IN    */ T_FLOAT floatValue);


/*  @ROLE    : is used to send all probe info to the probe controller
    @RETURN  : Error code */
extern T_ERROR PROBE_AGENT_SendAllProbes(
														 /* INOUT */ T_PROBE_AGENT * ptr_this,
														 /* IN    */ T_UINT32 FrameCount);

/*  @ROLE    : is used to send all probe info to the probe controller
    @RETURN  : Error code */
T_ERROR PROBE_AGENT_ThreadSendAllProbes(
														/* INOUT */ T_PROBE_AGENT * ptr_this);
#endif /* ProbeAgent_e */

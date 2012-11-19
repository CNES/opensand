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
 * @file EventAgent_e.h
 * @author TAS
 * @brief The EventAgent class implements the event agent services
 */

#ifndef EventAgent_e
#   define EventAgent_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "ErrorAgent_e.h"
#   include "Error_e.h"
#   include "Event_e.h"
#   include "DominoConstants_e.h"
#   include "IPAddr_e.h"
#   include "GenericPort_e.h"
#   include "GenericPacket_e.h"
#   include "EventsActivation_e.h"

#   define C_MAX_EVENT_PKT_ELT_NB 1
											/* Maximum Number of Elements in 1 Event Packet */
#   define C_MAX_EVENT_ON_PERIOD 128/* Maximum number of error sent by one component in one period */


typedef struct
{
	T_GENERIC_PORT _genericPort;
	T_GENERIC_PKT *_ptr_genPacket;
	T_EVENT_CATEGORY _LastEventCat[C_MAX_EVENT_ON_PERIOD];
	T_EVENT_INDEX _LastEventIndex[C_MAX_EVENT_ON_PERIOD];
	T_EVENT_VALUE _LastEventValue[C_MAX_EVENT_ON_PERIOD];
	T_BOOL _eventValueIsGoingToBeSent[C_MAX_EVENT_ON_PERIOD];
	T_EVENT _Event[C_MAX_EVENT_ON_PERIOD];
	T_UINT32 _nbEventIndex;
	T_UINT32 _FRSNbr[C_MAX_EVENT_ON_PERIOD];	/* Frame of the event occuration */
	T_UINT8 _FSMId[C_MAX_EVENT_ON_PERIOD];	/* FSM Id of the Event occuration */
	T_UINT32 _FSM_OfLastSent;	  /* FSM Id of the Event occuration */

	T_UINT32 *_FRSFramecount;	  /* Used to retrieve FRSFrame from execution context */
	T_UINT8 *_FSMIdentifier;	  /* Used to retrieve FSM number from execution context */
	T_EVENTS_ACTIVATION MyActivation;
	T_ERROR_AGENT *ptr_errorAgent;	/* pointer on error agent */
	T_BOOL _IsToSend;
} T_EVENT_AGENT;


/*  @ROLE    : This function initialises the Event agent
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_Init(
									/* INOUT */ T_EVENT_AGENT * ptr_this,
									/* IN    */ T_ERROR_AGENT * ptr_errorAgent,
									/* IN    */ T_IP_ADDR * ptr_ipAddr,
									/* IN    */ T_INT32 ComponentId,
									/* IN    */ T_INT32 InstanceId,
									/* IN    */ T_UINT16 SimReference,
									/* IN    */ T_UINT16 SimRun,
									/* IN    */ T_UINT32 * FRSRef,
									/* IN    */ T_UINT8 * FSMRef);


/*  @ROLE    : This function terminates the Event agent
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_Terminate(
										  /* INOUT */ T_EVENT_AGENT * ptr_this);


/*  @ROLE    : This function set the Event category/index/value
    @RETURN  : None */
T_ERROR EVENT_AGENT_SetLastEvent(
											  /* INOUT */ T_EVENT_AGENT * ptr_this,
											  /* IN    */ T_EVENT_CATEGORY cat,
											  /* IN    */ T_EVENT_INDEX index,
											  /* IN    */ T_EVENT_VALUE value,
											  /* IN    */ T_EVENT Event);


/*  @ROLE    : This function sends Event to Event Controller
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_SendEvent(
										  /* INOUT */ T_EVENT_AGENT * ptr_this);

/*  @ROLE    : This function sends Event to Event Controller
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_SendAllEvents(
												/* INOUT */ T_EVENT_AGENT * ptr_this);

#endif

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
 * @file EventAgent.c
 * @author TAS
 * @brief The EventAgent class implements the event agent services 
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "EventsActivation_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "GenericPacket_e.h"
#include "ProtoConstants_e.h"
#include "GenericPort_e.h"
#include "IPAddr_e.h"
#include "EventAgent_e.h"
#include "EnvironmentAgent_e.h"
#include "unused.h"


/*  @ROLE    : This function initialises the Event agent
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_Init(
									/* INOUT */ T_EVENT_AGENT * ptr_this,
									/* IN    */ T_ERROR_AGENT * ptr_errorAgent,
									/* IN    */ T_IP_ADDR * ptr_ipAddr,
									/* IN    */ T_INT32 ComponentId,
									/* IN    */ T_INT32 InstanceId,
									/* IN    */ T_UINT16 SimReference,
									/* IN    */ T_UINT16 UNUSED(SimRun),
									/* IN    */ T_UINT32 * FRSRef,
									/* IN    */ T_UINT8 * FSMRef)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 iEvent;
#ifdef _ASP_TRACE
	T_UINT8 iCategory;
	T_CHAR MyString[255];
	T_CHAR Message[255];
#endif

	memset(ptr_this, 0, sizeof(T_EVENT_AGENT));

	ptr_this->_nbEventIndex = 0;
	/* initialyse pointer value of error agent */
	ptr_this->ptr_errorAgent = ptr_errorAgent;

	/* Read the event filter configuration file : event_conf.conf */
	rid = EVENTS_ACTIVATION_ReadConfigFile(&(ptr_this->MyActivation));
	if(rid != C_ERROR_OK)
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
					  "Read error <rid=%u> of event_conf.conf file from scenario_%d",
					  rid, SimReference));
		ERROR_AGENT_SetLastErrorErrno(ptr_errorAgent,
												C_ERROR_CRITICAL, C_EVENT_ACT_FILE, rid);
		JUMP_ERROR(FIN, rid, rid);
	}

	/* Trace defined events (used for validation) */
#ifdef _ASP_TRACE
	for(iCategory = 0; iCategory < ptr_this->MyActivation._nbCategory;
		 iCategory++)
	{
		sprintf(MyString, " Category=%02ld",
				  ptr_this->MyActivation._EventCategory[iCategory]);
		memcpy(Message + iCategory * 12, MyString, 12);
	}
	Message[iCategory * 12] = 0;
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVENT_AGENT_Init() Read event_conf.conf configuration file successfull %u category found %s",
				  ptr_this->MyActivation._nbCategory, Message));
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVENT_AGENT_Init() The defined event state are C_EVENT_STATE_INIT=%u C_EVENT_STATE_RUN=%u C_EVENT_STATE_STOP=%u",
				  C_EVENT_STATE_INIT, C_EVENT_STATE_RUN, C_EVENT_STATE_STOP));
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVENT_AGENT_Init() The defined event are C_EVENT_INIT_REF=%u C_EVENT_END_SIMU=%u C_EVENT_COMP_STATE=%u",
				  C_EVENT_INIT_REF, C_EVENT_END_SIMU, C_EVENT_COMP_STATE));
#endif

	/* Allocate the generic packet */
  /*-----------------------------*/
	rid =
		GENERIC_PACKET_Create(&(ptr_this->_ptr_genPacket),
									 C_MAX_EVENT_PKT_ELT_NB);
	if(rid != C_ERROR_OK)
	{
		ERROR_AGENT_SetLastErrorErrno(ptr_errorAgent,
												C_ERROR_CRITICAL, C_ERROR_SOCK_OPEN, rid);
		JUMP_ERROR(FIN, rid, rid);
	}

	/* Init generic packet header */
  /*----------------------------*/
	ptr_this->_ptr_genPacket->_componentId =
		MAKE_COMPONENT_ID(ComponentId, InstanceId);

	/* Init Generic Packet Socket */
  /*----------------------------*/
	rid =
		GENERIC_PORT_InitSender(&(ptr_this->_genericPort), ptr_ipAddr,
										C_MAX_EVENT_PKT_ELT_NB * C_MAX_EVENT_ON_PERIOD);
	if(rid != C_ERROR_OK)
	{
		ERROR_AGENT_SetLastErrorErrno(ptr_errorAgent,
												C_ERROR_CRITICAL, C_ERROR_SOCK_WRITE, rid);
		JUMP_ERROR(FIN, rid, rid);
	}

	/* Init reference to FRSFrame number and FSM number */
  /*--------------------------------------------------*/
	ptr_this->_FRSFramecount = FRSRef;
	ptr_this->_FSMIdentifier = FSMRef;
	ptr_this->_FSM_OfLastSent = 0;

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVENT_AGENT_Init() successful for component=%s instance=%d",
				  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
														(((ptr_this->_ptr_genPacket->
															_componentId >> 4) & 0xF))),
				  (ptr_this->_ptr_genPacket->_componentId) & 0xF));

	/* Init ptr to 0 */
	for(iEvent = 0; iEvent < C_MAX_EVENT_ON_PERIOD; iEvent++)
	{
		ptr_this->_Event[iEvent] = 0;
	}

 FIN:
	return rid;
}


T_ERROR EVENT_AGENT_SendAllEvents(
												/* INOUT */ T_EVENT_AGENT * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 iEvent;
	T_UINT32 jEvent = 0;
	T_ELT_GEN_PKT *eltGenPkt;
	T_UINT32 nbEvents = 0;

	/* Tag event to be sent */
	for(iEvent = 0; iEvent < C_MAX_EVENT_ON_PERIOD; iEvent++)
	{
		if(ptr_this->_Event[iEvent] != 0)
		{
			ptr_this->_eventValueIsGoingToBeSent[iEvent] = TRUE;
			nbEvents++;
		}
	}

	if(ptr_this->_ptr_genPacket == NULL)
		return rid;

	for(iEvent = 0; iEvent < nbEvents; iEvent++)
	{

		/* Find the next probe value to be sent */
		while((jEvent < C_MAX_EVENT_ON_PERIOD)
				&& (!ptr_this->_eventValueIsGoingToBeSent[jEvent]))
		{
			jEvent++;
		}

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
					  "FRS=%u FSMId=%u EVENT_AGENT_SendEvent() for Component=%s_%u called for event tag on frame(%d) FSMid(%d)",
					  *(ptr_this->_FRSFramecount),
					  *(ptr_this->_FSMIdentifier),
					  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
															(((ptr_this->_ptr_genPacket->
																_componentId >> 4) & 0xF))),
					  (ptr_this->_ptr_genPacket->_componentId) & 0xF,
					  ptr_this->_FRSNbr[jEvent], ptr_this->_FSMId[jEvent]));
		JUMP_ERROR(FIN, rid,
					  GENERIC_PACKET_GetEltPkt(ptr_this->_ptr_genPacket, 0,
														&eltGenPkt));
		/* Init the generic packet fields : _ptr_genPacket */
	 /*--------------------------------*/
		ptr_this->_ptr_genPacket->_elementNumber = 1;
		ptr_this->_ptr_genPacket->_FSMNumber = 0;
		ptr_this->_ptr_genPacket->_frameNumber = ptr_this->_FRSNbr[jEvent];
		eltGenPkt->_id = ptr_this->_Event[jEvent];
		eltGenPkt->_categoryId = ptr_this->_LastEventCat[jEvent];
		eltGenPkt->_index = ptr_this->_LastEventIndex[jEvent];
		eltGenPkt->_value = ((T_UINT32) ptr_this->_FSMId[jEvent]) << 24;
		eltGenPkt->_value |= (ptr_this->_LastEventValue[jEvent] & 0x00FFFFFF);	/* only the last 24 bits */

		/* Trace generic packet */
		TRACE_LOG_GENERIC_PACKET((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
										  C_TRACE_VALID, stdout, ptr_this->_ptr_genPacket,
										  "GENERIC PACKET before sending to Event controller"));
		/* Sent packet to Event Controller */
		rid = GENERIC_PORT_SendGenPacket(&(ptr_this->_genericPort),
													ptr_this->_ptr_genPacket);
		if(rid != C_ERROR_OK)
		{
			TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
						  "FRS=%u FSMId=%u EVENT_AGENT_SendAllEvents() for Component=%s_%u cannot send packet",
						  *(ptr_this->_FRSFramecount),
						  *(ptr_this->_FSMIdentifier),
						  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
																(((ptr_this->_ptr_genPacket->
																	_componentId >> 4) & 0xF))),
						  (ptr_this->_ptr_genPacket->_componentId) & 0xF));
			/* Send error to error agent */
			ERROR_AGENT_SetLastErrorErrno(ptr_this->ptr_errorAgent,
													C_ERROR_CRITICAL,
													C_ERROR_SOCK_WRITE, rid);
			JUMP_ERROR(FIN, rid, rid);
		}
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
					  "FRS=%u FSMId=%u EVENT_AGENT_SendEvent() for Component=%s_%u send packet done for cmpt(%#x) id (%d) cat(%d) "
					  "index(%d) value(%d) frame(%d) FSMid(%d)",
					  *(ptr_this->_FRSFramecount),
					  *(ptr_this->_FSMIdentifier),
					  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
															(((ptr_this->_ptr_genPacket->
																_componentId >> 4) & 0xF))),
					  (ptr_this->_ptr_genPacket->_componentId) & 0xF,
					  ptr_this->_ptr_genPacket->_componentId, eltGenPkt->_id,
					  eltGenPkt->_categoryId, eltGenPkt->_index, eltGenPkt->_value,
					  ptr_this->_ptr_genPacket->_frameNumber,
					  ptr_this->_FSMId[jEvent]));

		/* Reset event ptr */
		ptr_this->_Event[jEvent] = 0;
		ptr_this->_eventValueIsGoingToBeSent[jEvent] = FALSE;
	}

 FIN:
	return rid;
}


/*  @ROLE    : This function terminates the Event agent
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_Terminate(
										  /* INOUT */ T_EVENT_AGENT * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	JUMP_ERROR(FIN, rid, EVENT_AGENT_SendAllEvents(ptr_this));
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVENT_AGENT_Terminate() successful for component=%s instance=%u",
				  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
														(((ptr_this->_ptr_genPacket->
															_componentId >> 4) & 0xF))),
				  (ptr_this->_ptr_genPacket->_componentId) & 0xF));

	JUMP_ERROR(FIN, rid, GENERIC_PACKET_Delete(&(ptr_this->_ptr_genPacket)));
	JUMP_ERROR(FIN, rid, GENERIC_PORT_Terminate(&(ptr_this->_genericPort)));

 FIN:
	return rid;
}


/*  @ROLE    : This function set the Event category/index/value
    @RETURN  : None */
T_ERROR EVENT_AGENT_SetLastEvent(
											  /* INOUT */ T_EVENT_AGENT * ptr_this,
											  /* IN    */ T_EVENT_CATEGORY cat,
											  /* IN    */ T_EVENT_INDEX index,
											  /* IN    */ T_EVENT_VALUE value,
											  /* IN    */ T_EVENT Event)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT8 iCategory;
	T_BOOL TheCategoryIsFound;


	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVENT_AGENT_SetLastEvent() called for component=%s instance=%u category=%u, index=%u value=%u",
				  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
														(((ptr_this->_ptr_genPacket->
															_componentId >> 4) & 0xF))),
				  (ptr_this->_ptr_genPacket->_componentId) & 0xF, cat, index,
				  value));

	/* Find category in the small array of event category */
	TheCategoryIsFound = FALSE;
	for(iCategory = 0; iCategory < ptr_this->MyActivation._nbCategory;
		 iCategory++)
	{
		if(cat ==
			(T_EVENT_CATEGORY) (ptr_this->MyActivation._EventCategory[iCategory]))
		{
			TheCategoryIsFound = TRUE;
		}
	}

	/* The category is not found in file, don't send the event */
	if(!TheCategoryIsFound)
		return rid;


	while((ptr_this->_nbEventIndex < C_MAX_EVENT_ON_PERIOD) &&
			(ptr_this->_Event[ptr_this->_nbEventIndex] != 0))
	{
		ptr_this->_nbEventIndex++;
	}
	if(ptr_this->_nbEventIndex == C_MAX_EVENT_ON_PERIOD)
	{
		/* Reset to 0 */
		ptr_this->_nbEventIndex = 0;
		/* Try again */
		while((ptr_this->_nbEventIndex < C_MAX_EVENT_ON_PERIOD) &&
				(ptr_this->_Event[ptr_this->_nbEventIndex] != 0))
		{
			ptr_this->_nbEventIndex++;
		}
	}

	if(ptr_this->_nbEventIndex == C_MAX_EVENT_ON_PERIOD)
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
					  "EVENT_AGENT_SetLastEvent() Nb MAX Event is reached ==> Send all events to Controller"));
		JUMP_ERROR(FIN, rid, EVENT_AGENT_SendAllEvents(ptr_this));
	}

	ptr_this->_LastEventCat[ptr_this->_nbEventIndex] = cat;
	ptr_this->_LastEventIndex[ptr_this->_nbEventIndex] = index;
	ptr_this->_LastEventValue[ptr_this->_nbEventIndex] = value;
	ptr_this->_Event[ptr_this->_nbEventIndex] = Event;

	/* Set the date when event occurs */
	ptr_this->_FRSNbr[ptr_this->_nbEventIndex] = *(ptr_this->_FRSFramecount);
	ptr_this->_FSMId[ptr_this->_nbEventIndex] = *(ptr_this->_FSMIdentifier);
	ptr_this->_nbEventIndex++;

 FIN:
	return rid;
}


/*  @ROLE    : This function sends Event to Event Controller
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_SendEvent(
										  /* INOUT */ T_EVENT_AGENT * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	ptr_this->_IsToSend = TRUE;
	/* The method to send event from components */
	return rid;
}

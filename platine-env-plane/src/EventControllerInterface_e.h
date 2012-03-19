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
 * @file EventControllerInterface_e.h
 * @author TAS
 * @brief The EventController class implements the event controller
 */

#ifndef EventController_e
#   define EventController_e

/* SYSTEM RESOURCES */
#   include <stdio.h>

/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Event_e.h"
#   include "Error_e.h"
#   include "GenericPacket_e.h"
#   include "GenericPort_e.h"
#   include "UDPSocket_e.h"
#   include "ComParameters_e.h"
#   include "EventsDef_e.h"
#   include "EventsActivation_e.h"
#   include "EventOutputFormatter_e.h"
#   include "ErrorAgent_e.h"

#   include "CircularBuffer_e.h"


typedef struct
{

	T_ERROR_AGENT _errorAgent;	  /* the error agent */

	T_EVENT _Event;
	T_BOOL _DisplayFlag;

	FILE *_TraceFile;

	T_GENERIC_PORT _ServerEvtPort;
	T_GENERIC_PKT *_ptr_genPacket;
	T_UDP_SOCKET _DisplayPort;

	T_BOOL _simuIsRunning;		  /* the simulation is running */

	T_EVENT_OUTPUT_FORMATTER _OutputFormat;

	T_COM_PARAMETERS _ComParams;

	T_EVENTS_DEF _EventsDefinition;
	T_EVENTS_ACTIVATION _EventActiv;

} T_EVT_CTRL;

/*  @ROLE    : This function starts controller's interface
    @RETURN  : Error code */
T_INT32 startEventControllerInterface(T_INT32 argc, T_CHAR * argv[]);

/*  @ROLE    : This function intialises Event Controller process
    @RETURN  : Error code */
T_ERROR EVT_CTRL_Init(
/*  INOUT  */ T_EVT_CTRL * ptr_this,
/*  IN     */ T_BOOL display);


/*  @ROLE    : This function intialises Event Controller for current simulation
    @RETURN  : Error code */
T_ERROR EVT_CTRL_InitSimulation(
/*  IN     */ T_EVT_CTRL * ptr_this);


/*  @ROLE    : This function sets Event Controller in a proper state 
               at the end of current simulation
    @RETURN  : Error code */
T_ERROR EVT_CTRL_EndSimulation(
											/* INOUT */ T_EVT_CTRL * ptr_this,
											/* IN    */ T_BOOL storeEvent);


/*  @ROLE    : This function stops Event controller properly.
    @RETURN  : Error code */
T_ERROR EVT_CTRL_Terminate(
/*  IN     */ T_EVT_CTRL * ptr_this);


/*  @ROLE    : This function writes the event message into log file
               and, if expected, sends it to display.
    @RETURN  : Error code */
T_ERROR EVT_CTRL_SendTrace(
/*  IN     */ T_EVT_CTRL * ptr_this,
/*  IN     */ T_ELT_GEN_PKT * eltGenPkt);


/*  @ROLE    : This function creates event messages and writes them to log file
    @RETURN  : Error code */
T_ERROR EVT_CTRL_DoPacket(
/*  IN     */ T_EVT_CTRL * ptr_this);



#endif

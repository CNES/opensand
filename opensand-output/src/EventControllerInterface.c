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
 * @file EventControllerInterface.c
 * @author TAS
 * @brief The EventController class implements the event controller process
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>

/* PROJECT RESOURCES */
#include "Time_e.h"
#include "Trace_e.h"
#include "FilePath_e.h"
#include "EventsActivation_e.h"
#include "IPAddr_e.h"
#include "EventControllerInterface_e.h"
#include "EventController_e.h"
#include "unused.h"

#include <string.h>

#define C_MAX_EVENT_PKT_ELT_NB 1	/* Maximum Number of Elements in 1 Event Packet */
#define C_MAX_EVENT_ON_PERIOD 128	/* Maximum number of event sent by one component in one period */
#define C_LOG_FILE_NAME_DEFAULT "event_log.txt"	/* Name of Event log File created for each simulation */

#define C_EVENT_DISPLAY_MAX_SIZE 256	/* Maximum size of UDP packet sent to event display */


T_INT32 startEventControllerInterface(T_INT32 argc, T_CHAR * argv[])
{
	T_EVT_CTRL _ctrl;
	T_ELT_GEN_PKT * eltGenPkt;

	T_ERROR rid = C_ERROR_OK;

	extern T_CHAR *optarg;

	T_CHAR command[50], *str;
	T_INT16 cmptId, level, opt;
	T_BOOL display = FALSE;
	T_UINT64 flag;

	/* activate TRACE option */
  /*-----------------------*/
	while((opt = getopt(argc, argv, "-T:ht:d")) != EOF)
	{
		switch (opt)
		{
		case 'T':
			/* get component id */
			strcpy(command, optarg);
			str = strtok(command, ":");
			if(str == NULL)
			{
				fprintf(stderr, "bad parameter: -T%s\n", optarg);
				exit(-1);
			}
			cmptId = atoi(str);
			flag = 0x1L;
			flag <<= C_TRACE_THREAD_MAX + cmptId;

			/* get level id */
			str = strtok(NULL, ":");
			if(str == NULL)
			{
				level = 0xff;
				fprintf(stdout, "activate all traces for component id %d\n",
						  cmptId);
			}
			else
			{
				level = atoi(str);
				fprintf(stdout, "activate trace level %d for component id %d\n",
						  level, cmptId);
			}

			/* activate trace */
			TRACE_ACTIVATE(flag, level);
			break;

		case 't':
			level = atoi(optarg);
			if(level == 0)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_0 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			if(level == 1)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_1 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			if(level == 2)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_2 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			if(level == 3)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_3 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			fprintf(stdout, "activate trace level %d for all components\n", level);
			break;

		case 'd':
			display = TRUE;
			break;
		case 'h':
		case '?':
			fprintf(stderr, "usage: %s [-h] [-d -T<cmptId> -T<cmptId> ...]\n",
					  argv[0]);
			fprintf(stderr, "\t-h                   print this message\n");
			fprintf(stderr,
					  "\t-d                   activate event external display\n");
			fprintf(stderr,
					  "\t-t<level>            activate <level> trace for all components\n");
			fprintf(stderr,
					  "\t                     -t0     activate debug trace for all components\n");
			fprintf(stderr,
					  "\t-T<cmptId:level>     activate trace for <cmptId>\n");
			fprintf(stderr,
					  "\t                     -T5     activate all traces for component id 5\n");
			fprintf(stderr,
					  "\t                     -T5:1   activate valid trace for component id 5\n");
			exit(-1);
			break;
		}
	}

	/* Initialise config path and output path */
  /*----------------------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, FILE_PATH_InitClass(),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_ERROR,
							"FILE_PATH_InitClass() failed"));

	/* init  event controller Session  */
  /*---------------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, EVT_CTRL_Init(&_ctrl, display),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_ERROR,
							"EVT_CTRL_Init() failed"));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "==============================================================="));


/* infinite main loop of generic packet reception */
/*------------------------------------------------*/
	while(1)
	{
		SEND_AG_ERRNO_JUMP(CLEAN, rid,
								 GENERIC_PORT_RecvGenPacket(&(_ctrl._ServerEvtPort),
																	 _ctrl._ptr_genPacket),
								 &(_ctrl._errorAgent), C_ERROR_CRITICAL, C_II_P_SOCKET,
								 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
								  C_TRACE_ERROR,
								  "GENERIC_PORT_RecvGenPacket() failed"));

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_DEBUG,
					  "Receive data"));

		/* check category id */
	 /*-------------------*/
		GENERIC_PACKET_GetEltPkt(_ctrl._ptr_genPacket, 0, &eltGenPkt);
		switch (eltGenPkt->_categoryId)
		{
		case C_CAT_INIT:			  /* Init packet */
			if(EVT_CTRL_InitSimulation(&_ctrl) != C_ERROR_OK)
			{
				TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
								 C_TRACE_ERROR, "EVT_CTRL_InitSimulation() failed"));
				EVT_CTRL_EndSimulation(&_ctrl, FALSE);
			}
			break;

		case C_CAT_END:
			EVT_CTRL_EndSimulation(&_ctrl, TRUE);
			break;
		default:
			if(_ctrl._simuIsRunning)
			{
				if(EVT_CTRL_DoPacket(&_ctrl) != C_ERROR_OK)
				{
					TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
									 C_TRACE_ERROR, "EVT_CTRL_DoPacket() failed"));
					EVT_CTRL_EndSimulation(&_ctrl, FALSE);
				}
			}
			else
			{
				SEND_AG_ERRNO(rid, C_ERROR_INIT_REF,
								  &(_ctrl._errorAgent), C_ERROR_CRITICAL, 0,
								  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
									C_TRACE_ERROR,
									"GENERIC_PORT_RecvGenPacket() receive data without start packet"));
			}
			break;
		}
	}

 CLEAN:
	/* close Session  */
  /*----------------*/
	EVT_CTRL_Terminate(&_ctrl);

 FIN:
	return rid;
}


/*  @ROLE    : This function intialises Event Controller process
    @RETURN  : Error code */
T_ERROR EVT_CTRL_Init( /*  INOUT  */ T_EVT_CTRL * ptr_this,
							 /*  IN     */ T_BOOL display)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT8 iEvent;

	/* Initialise Time at each simulation initialisation */
  /*---------------------------------------------------*/
	TIME_Init();

	/* read Communication definition file   */
  /*--------------------------------------*/
	/* !CB to solve error agent is not already created (line after) */
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_Init() read com_parameters.conf file from config/exec directory"));
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 COM_PARAMETERS_ReadConfigFile(&(ptr_this->_ComParams)),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_COM_PARAMETERS_FILE, (C_TRACE_THREAD_UNKNOWN,
															 C_TRACE_COMP_EVENT, C_TRACE_ERROR,
															 "COM_PARAMETERS_ReadConfigFile() failed for Event controller"));

	/* create the error agent */
  /*------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, ERROR_AGENT_Init(&(ptr_this->_errorAgent),
															  &(ptr_this->_ComParams.
																 _ControllersPorts.
																 _ErrorController._IpAddress),
															  C_COMP_EVENT_CTRL, 0, NULL,
															  NULL), (C_TRACE_THREAD_UNKNOWN,
																		 C_TRACE_COMP_EVENT,
																		 C_TRACE_ERROR,
																		 "ERROR_AGENT_Init() failed"));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_DEBUG,
				  "EVT_CTRL_Init() open error agent %s port %d",
				  ptr_this->_ComParams._ControllersPorts._ErrorController.
				  _IpAddress._addr,
				  ptr_this->_ComParams._ControllersPorts._ErrorController.
				  _IpAddress._port));

	/* open the error generic port to receive event generic packet */
  /*-------------------------------------------------------------*/
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_Init() open the generic port to receive generic packets"));
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 GENERIC_PORT_InitReceiver(&(ptr_this->_ServerEvtPort),
																&(ptr_this->_ComParams.
																  _ControllersPorts.
																  _EventController._IpAddress),
																C_MAX_EVENT_PKT_ELT_NB *
																C_MAX_EVENT_ON_PERIOD),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL, C_II_P_SOCKET,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
							  C_TRACE_ERROR, "GENERIC_PORT_InitReceiver() failed"));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_Init() open event receiver %s port %d  done for event reception",
				  ptr_this->_ComParams._ControllersPorts._EventController.
				  _IpAddress._addr,
				  ptr_this->_ComParams._ControllersPorts._EventController.
				  _IpAddress._port));

	/* Allocate the generic packet */
  /*-----------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 GENERIC_PACKET_Create(&(ptr_this->_ptr_genPacket),
														  C_MAX_EVENT_PKT_ELT_NB *
														  C_MAX_EVENT_ON_PERIOD),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL, 0,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
							  C_TRACE_ERROR, "GENERIC_PACKET_Create() failed"));

	/* Set the display flag */
  /*----------------------*/
	ptr_this->_DisplayFlag = display;

	/* If displayFlag is set to TRUE, open error Display port */
  /*--------------------------------------------------------*/
	if(ptr_this->_DisplayFlag)
	{
		/* open the UDP port to send data to display */
	 /*-------------------------------------------*/
		SEND_AG_ERRNO_JUMP(FIN, rid,
								 UDP_SOCKET_InitSender(&(ptr_this->_DisplayPort),
															  &(ptr_this->_ComParams.
																 _DisplayPorts._EventDisplay.
																 _IpAddress),
															  C_EVENT_DISPLAY_MAX_SIZE),
								 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
								 C_EI_PD_SOCKET, (C_TRACE_THREAD_UNKNOWN,
														C_TRACE_COMP_EVENT, C_TRACE_ERROR,
														"UDP_SOCKET_InitSender() failed for display"));

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
					  "EVT_CTRL_Init() open udp display socket %s port %d",
					  ptr_this->_ComParams._DisplayPorts._EventDisplay._IpAddress.
					  _addr,
					  ptr_this->_ComParams._DisplayPorts._EventDisplay._IpAddress.
					  _port));
	}

	/* Read Event definition configuration file */
  /*------------------------------------------*/
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_Init() Read event_def.conf file from config/exec directory"));
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 EVENTS_DEF_ReadConfigFile(&(ptr_this->_EventsDefinition)),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_EVENT_DEF_FILE, (C_TRACE_THREAD_UNKNOWN,
													  C_TRACE_COMP_EVENT, C_TRACE_ERROR,
													  "EVENT_DEF_ReadConfigFile() failed for Event controller"));
	for(iEvent = 0; iEvent < ptr_this->_EventsDefinition._nbEvent; iEvent++)
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
					  "EVT_CTRL_Init() Read event <%s> in event_def.conf file ",
					  ptr_this->_EventsDefinition._Event[iEvent]._Name));
	}


	/* Init internal data */
  /*--------------------*/
	ptr_this->_simuIsRunning = FALSE;

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_Init() sucessful"));

 FIN:
	return (rid);
}


/*  @ROLE    : This function initialises Event Controller for current simulation
    @RETURN  : Error code */
T_ERROR EVT_CTRL_InitSimulation(
/*  IN     */ T_EVT_CTRL * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 simReference = 0;
	T_UINT16 simRef = 0, simRun = 0;
	T_FILE_PATH logFileName;
	T_ELT_GEN_PKT * eltGenPkt;

	/* check the simulation running status */
  /*-------------------------------------*/
	if(ptr_this->_simuIsRunning == TRUE)
		EVT_CTRL_EndSimulation(ptr_this, FALSE);

	/* Initialise Time */
  /*-----------------*/
	TIME_Init();

	/* extract sim reference from initpacket */
  /*---------------------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 GENERIC_PACKET_GetEltPkt(ptr_this->_ptr_genPacket, 0,
															  &eltGenPkt),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_EVENT_COMMAND, (C_TRACE_THREAD_UNKNOWN,
													 C_TRACE_COMP_EVENT, C_TRACE_ERROR,
													 "GENERIC_PACKET_GetEltPkt() cannot get elt generic packet n°0"));

	simReference = eltGenPkt->_value;

	simRun = (T_UINT16) (simReference & 0x0000FFFF);
	simRef = (T_UINT16) ((simReference >> 16) & 0x0000FFFF);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "Init packet received with scenario_%d, run_%d (reference %ld)",
				  simRef, simRun, simReference));


	/* Get the output path */
  /*---------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 FILE_PATH_GetOutputPath(logFileName, simRef, simRun),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_EVENT_COMMAND, (C_TRACE_THREAD_UNKNOWN,
													 C_TRACE_COMP_EVENT, C_TRACE_ERROR,
													 "FILE_PATH_GetOutputPath() failed"));

	/* Get the complete file name */
  /*----------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 FILE_PATH_Concat(logFileName, C_LOG_FILE_NAME_DEFAULT),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_EVENT_COMMAND, (C_TRACE_THREAD_UNKNOWN,
													 C_TRACE_COMP_EVENT, C_TRACE_ERROR,
													 "FILE_PATH_Concat() failed"));

	/* create and open event log file using run path */
  /*-----------------------------------------------*/
	ptr_this->_TraceFile = fopen(logFileName, "w");
	if(ptr_this->_TraceFile == NULL)
	{
		SEND_AG_ERRNO_JUMP(FIN, rid, C_ERROR_FILE_OPEN,
								 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
								 C_EVENT_LOG_FILE, (C_TRACE_THREAD_UNKNOWN,
														  C_TRACE_COMP_EVENT, C_TRACE_ERROR,
														  "fopen() failed"));
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "Opened event_log.txt file %s", logFileName));

	/* Write init message to log file */
  /*-------------------------------*/
	JUMP_ERROR(FIN, rid, EVT_CTRL_DoPacket(ptr_this));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_SimulationInit() sucessful"));

	/* Simu is now running properly for event controller */
  /*---------------------------------------------------*/
	ptr_this->_simuIsRunning = TRUE;

 FIN:
	if(rid != C_ERROR_OK)
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
					  "Error %d during Event controller Simulation Init!", rid));
	}
	return (rid);
}


/*  @ROLE    : This function sets Event Controller in a proper state 
               at the end of current simulation
    @RETURN  : Error code */
T_ERROR EVT_CTRL_EndSimulation(
											/* INOUT */ T_EVT_CTRL * ptr_this,
											/* IN    */ T_BOOL storeEvent)
{
	T_ERROR rid = C_ERROR_OK;

	/* Write end message to log file */
  /*-------------------------------*/
	if(storeEvent == TRUE)
		JUMP_ERROR(FIN, rid, EVT_CTRL_DoPacket(ptr_this));

	/* Close log file       */
  /*----------------------*/
	if(ptr_this->_TraceFile != NULL)
		fclose(ptr_this->_TraceFile);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "Closed event_log.txt file "));

	/* close the udp port if display flag was set to true */
  /*----------------------------------------------------*/
	/* !CB no need to close the socket as long as it is only open at the beginnig 
	   if ( ptr_this->_DisplayFlag )  
	   UDP_SOCKET_Terminate(&(ptr_this->_DisplayPort));
	 */
	/* Re-Initialise internal data */
  /*--------------------------*/
	ptr_this->_simuIsRunning = FALSE;

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_SimulationEnd() sucessful"));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "==============================================================="));

 FIN:
	if(rid != C_ERROR_OK)
	{
		TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_ERROR,
						 "Error %d during EVT_CTRL_SimulationEnd() ", rid));
	}
	return rid;
}


/*  @ROLE    : This function creates event messages and writes them to log file
    @RETURN  : Error code */
T_ERROR EVT_CTRL_DoPacket(
/*  IN     */ T_EVT_CTRL * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_ELT_GEN_PKT * eltGenPkt;

	/* get current element of received generic packet */
  /*------------------------------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 GENERIC_PACKET_GetEltPkt(ptr_this->_ptr_genPacket, 0,
															  &eltGenPkt),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL, 0,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
							  C_TRACE_ERROR, "GENERIC_PACKET_GetEltPkt() failed"));

#ifdef _EEVT_MODE
	/* Store the component state */
  /*---------------------------*/
	if((eltGenPkt->_categoryId == C_EVENT_SIMU) && (eltGenPkt->_index == 0)
		&& (eltGenPkt->_id == C_EVENT_COMP_STATE))
	{
#   ifdef _ASP_PEA_CONF
		set_component_state((eltGenPkt->_value & 0x00FFFFFF));
#   else
		/* !CB     set_component_state(eltGenPkt->_value);  */

		set_component_state((eltGenPkt->_value & 0x00FFFFFF));
#   endif
		 /* _EEVT_MODE */

	}
#endif /* _EEVT_MODE */

	/* Format the error trace string for each element using */
  /*------------------------------------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 T_EVENT_OUTPUT_FORMATTER_Formatter(&
																			(ptr_this->
																			 _OutputFormat),
																			&(ptr_this->
																			  _EventsDefinition),
																			ptr_this->
																			_ptr_genPacket,
																			eltGenPkt),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL, 0,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT,
							  C_TRACE_ERROR,
							  "T_EVENT_OUTPUT_FORMATTER_Formatter() failed"));

	/* Send message corresponding to this error */
  /*------------------------------------------*/
	EVT_CTRL_SendTrace(ptr_this, eltGenPkt);

 FIN:
	return (rid);
}


/*  @ROLE    : This function writes the event message into log file
               and, if expected, sends it to display.
    @RETURN  : Error code */
T_ERROR EVT_CTRL_SendTrace(
/*  IN     */ T_EVT_CTRL * ptr_this,
/*  IN     */ T_ELT_GEN_PKT * UNUSED(eltGenPkt))
{
	T_CHAR eventMessage[C_EVENT_DISPLAY_MAX_SIZE];

	/* write error trace string to error log file */
  /*--------------------------------------------*/
	if(strcmp("END", ptr_this->_OutputFormat._category) == 0)
	{
		sprintf(eventMessage,
				  "FRSframe (%lu), FSM (%d), %s_%d, Category (%s), %s ",
				  ptr_this->_OutputFormat._event_date._frame_number,
				  ptr_this->_OutputFormat._event_date._FSM_number,
				  ptr_this->_OutputFormat._event_origin._componentType,
				  ptr_this->_OutputFormat._event_origin._InstanceId,
				  ptr_this->_OutputFormat._category,
				  ptr_this->_OutputFormat._event_name);
	}
	else
	{
		if(strcmp("0", ptr_this->_OutputFormat._index_signification) == 0)
		{
			sprintf(eventMessage,
					  "FRSframe (%lu), FSM (%d), %s_%d, Category (%s), %s, %s = %lu (Unit : %s) ",
					  ptr_this->_OutputFormat._event_date._frame_number,
					  ptr_this->_OutputFormat._event_date._FSM_number,
					  ptr_this->_OutputFormat._event_origin._componentType,
					  ptr_this->_OutputFormat._event_origin._InstanceId,
					  ptr_this->_OutputFormat._category,
					  ptr_this->_OutputFormat._event_name,
					  ptr_this->_OutputFormat._value_signification,
					  ptr_this->_OutputFormat._value, ptr_this->_OutputFormat._unit);
		}
		else
		{
			if(strcmp("cle", ptr_this->_OutputFormat._value_signification) == 0)
			{
				sprintf(eventMessage,
						  "FRSframe (%lu), FSM (%d), %s_%d, Category (%s), %s, %s = %lu, %s = 0x%08lX (Unit : %s) ",
						  ptr_this->_OutputFormat._event_date._frame_number,
						  ptr_this->_OutputFormat._event_date._FSM_number,
						  ptr_this->_OutputFormat._event_origin._componentType,
						  ptr_this->_OutputFormat._event_origin._InstanceId,
						  ptr_this->_OutputFormat._category,
						  ptr_this->_OutputFormat._event_name,
						  ptr_this->_OutputFormat._index_signification,
						  ptr_this->_OutputFormat._index_value,
						  ptr_this->_OutputFormat._value_signification,
						  ptr_this->_OutputFormat._value,
						  ptr_this->_OutputFormat._unit);
			}
			else
			{
				sprintf(eventMessage,
						  "FRSframe (%lu), FSM (%d), %s_%d, Category (%s), %s, %s = %lu, %s = %lu (Unit : %s) ",
						  ptr_this->_OutputFormat._event_date._frame_number,
						  ptr_this->_OutputFormat._event_date._FSM_number,
						  ptr_this->_OutputFormat._event_origin._componentType,
						  ptr_this->_OutputFormat._event_origin._InstanceId,
						  ptr_this->_OutputFormat._category,
						  ptr_this->_OutputFormat._event_name,
						  ptr_this->_OutputFormat._index_signification,
						  ptr_this->_OutputFormat._index_value,
						  ptr_this->_OutputFormat._value_signification,
						  ptr_this->_OutputFormat._value,
						  ptr_this->_OutputFormat._unit);
			}
		}
	}

	fprintf(ptr_this->_TraceFile, "EVENT : %s \n", eventMessage);
	fflush(ptr_this->_TraceFile);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "Received message : %s", eventMessage));

	/* if (Display UDP port open) send to Display UDP port */
  /*-----------------------------------------------------*/

	if(ptr_this->_DisplayFlag)
	{
		UDP_SOCKET_SendBytes(&(ptr_this->_DisplayPort), (T_BUFFER) eventMessage,
									(T_INT32) (strlen(eventMessage)));
	}

	return C_ERROR_OK;
}


/*  @ROLE    : This function stops Event controller properly.
    @RETURN  : Error code */
T_ERROR EVT_CTRL_Terminate(
									  /* INOUT */ T_EVT_CTRL * ptr_this)
{

	/* free the generic packet */
  /*-------------------------*/
	GENERIC_PACKET_Delete(&(ptr_this->_ptr_genPacket));

	/* close the generic port */
  /*------------------------*/
	GENERIC_PORT_Terminate(&(ptr_this->_ServerEvtPort));

	/* close the udp port if display flag was set to true */
  /*----------------------------------------------------*/
	/* !CB no need to close the socket as long as it is only open at the beginnig 
	   if ( ptr_this->_DisplayFlag )  
	   UDP_SOCKET_Terminate(&(ptr_this->_DisplayPort));
	 */
	/* Close log file       */
  /*----------------------*/
	if(ptr_this->_TraceFile != NULL)
	{
		fclose(ptr_this->_TraceFile);

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
					  "Closed event_log.txt file "));
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_EVENT, C_TRACE_VALID,
				  "EVT_CTRL_Terminate() sucessful"));

	return C_ERROR_OK;
}

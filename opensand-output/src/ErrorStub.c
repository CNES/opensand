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
 * @file ErrorStub.c
 * @author TAS
 * @brief The ErrorControllerInterface class implements the error controller
 *        process
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>

/* PROJECT RESOURCES */
#include "Error_e.h"
#include "Time_e.h"
#include "Trace_e.h"
#include "FilePath_e.h"
#include "ErrorControllerInterface_e.h"
#include "ErrorController_e.h"
#include "ErrorDef_e.h"
#include "IPAddr_e.h"


#include <string.h>
#include <stdio.h>

/* Handling kill signals to scheduling controller (using pid) on UNIX system */
#include <sys/types.h>
#include <signal.h>

#define C_MAX_ERROR_PKT_ELT_NB 1	/* Maximum Number of Elements in 1 Error Packet */
#define C_LOG_FILE_NAME_DEFAULT "error_log.txt"	/* Name of Error log File created for each simulation */

#define C_ERROR_DISPLAY_MAX_SIZE 256	/* Maximum size of UDP packet sent to eror display */

T_ERROR ERR_CTRL_InitSimulation(T_ERR_CTRL * ptr_this);

int main(T_INT32 argc, T_CHAR * argv[])
{
	T_ERR_CTRL _ctrl;
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
		case 'd':
			display = TRUE;
			break;
		case 'h':
		case '?':
			fprintf(stderr, "usage: %s [-h] [-T<cmptId> -T<cmptId> ...]\n",
					  argv[0]);
			fprintf(stderr, "\t-h                   print this message\n");
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
	JUMP_ERROR(FIN, rid, FILE_PATH_InitClass());

	/* init  error controller Session  */
  /*---------------------------------*/
	JUMP_ERROR(FIN, rid, ERR_CTRL_Init(&_ctrl, display));	/* config path and output path are arguments set when starting error controller */

	fprintf(stdout,
			  "===============================================================");

/* infinite main loop of generic packet reception */
/* End of loop happens when maximum number of allowed consecutive errors is reached */
/*----------------------------------------------------------------------------------*/
	while(1)
	{
		fprintf(stdout, "waiting  data\n");
		if(GENERIC_PORT_RecvGenPacket
			(&(_ctrl._ServerErrPort), _ctrl._ReceivedPacket) == C_ERROR_OK)
		{

			fprintf(stdout, "Receive data");
			/* Get first element of received packet */
		/*--------------------------------------*/
			GENERIC_PACKET_GetEltPkt(_ctrl._ReceivedPacket, 0, &eltGenPkt);
			fprintf(stdout, "recv data %d\n", eltGenPkt->_categoryId);

			/* check category id */
		/*-------------------*/
			if(eltGenPkt->_categoryId == C_CAT_INIT)
			{							  /* Init packet */
				if(ERR_CTRL_InitSimulation(&_ctrl) != C_ERROR_OK)
				{
					fprintf(stderr, "ERR_CTRL_InitSimulation() failed");
					//          ERR_CTRL_EndSimulation(&_ctrl,FALSE);
					/* !CB the simulation stop must be done using the run script 
					   kill((T_INT32)(_ctrl._Pid), SIGALRM); 
					   fprintf (stderr, (C_TRACE_THREAD_UNKNOWN,C_TRACE_COMP_ERROR,C_TRACE_ERROR,
					   "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
					   _ctrl._Pid));
					 */
				}
			}
			else
			{
				if(_ctrl._simuIsRunning)
				{
					if(ERR_CTRL_DoPacket(&_ctrl) != C_ERROR_OK)
					{
						fprintf(stderr, "ERR_CTRL_DoPacket() failed");
						//            ERR_CTRL_EndSimulation(&_ctrl,FALSE);
						/* !CB the simulation stop must be done using the run script 
						   kill((T_INT32)(_ctrl._Pid), SIGALRM); 
						   fprintf (stderr, (C_TRACE_THREAD_UNKNOWN,C_TRACE_COMP_ERROR,C_TRACE_ERROR,
						   "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
						   _ctrl._Pid));
						 */
					}
				}
				else
				{
					fprintf(stderr,
							  "GENERIC_PORT_RecvGenPacket() receive data without init packet");
				}
			}
		}
		else
		{
			fprintf(stderr, "GENERIC_PORT_RecvGenPacket() failed");
			if(_ctrl._simuIsRunning)
			{
				//        ERR_CTRL_EndSimulation(&_ctrl,FALSE);
				/* !CB the simulation stop must be done using the run script 
				   kill((T_INT32)(_ctrl._Pid), SIGALRM); 
				   fprintf (stderr, (C_TRACE_THREAD_UNKNOWN,C_TRACE_COMP_ERROR,C_TRACE_ERROR,
				   "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
				   _ctrl._Pid));
				 */
			}
		}
	}

 FIN:
	return rid;
}


/*  @ROLE    : This function intialises Error Controller process
    @RETURN  : Error code */
T_ERROR ERR_CTRL_Init( /*  IN/OUT */ T_ERR_CTRL * ptr_this,
							 /*  IN     */ T_BOOL display)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 iError;

	/* Initialise Time at each simulation initialisation */
  /*---------------------------------------------------*/
	TIME_Init();

	/* read Communication definition file   */
  /*--------------------------------------*/
	fprintf(stdout, "read com_parameters.conf file from config/exec directory");
	JUMP_ERROR(FIN, rid, COM_PARAMETERS_ReadConfigFile(&(ptr_this->_ComParams)));

	/* open the error generic port to receive error generic packet */
  /*-------------------------------------------------------------*/
	fprintf(stdout, "open the generic port to receive generic packets");
	JUMP_ERROR(FIN, rid, GENERIC_PORT_InitReceiver(&(ptr_this->_ServerErrPort),
																  &(ptr_this->_ComParams.
																	 _ControllersPorts.
																	 _ErrorController.
																	 _IpAddress),
																  C_MAX_ERROR_PKT_ELT_NB));
	fprintf(stdout, "open error receiver %s port %d  done for error reception",
			  ptr_this->_ComParams._ControllersPorts._ErrorController._IpAddress.
			  _addr,
			  ptr_this->_ComParams._ControllersPorts._ErrorController._IpAddress.
			  _port);

	/* Allocate the generic packet */
  /*-----------------------------*/
	JUMP_ERROR(FIN, rid,
				  GENERIC_PACKET_Create(&(ptr_this->_ReceivedPacket),
												C_MAX_ERROR_PKT_ELT_NB));

	/* Set the display flag */
  /*----------------------*/
	ptr_this->_DisplayFlag = display;

	/* If displayFlag is set to TRUE, open error Display port */
  /*--------------------------------------------------------*/
	if(ptr_this->_DisplayFlag)
	{
		/* open the UDP port to send data to display */
	 /*-------------------------------------------*/
		JUMP_ERROR(FIN, rid, UDP_SOCKET_InitSender(&(ptr_this->_DisplayPort),
																 &(ptr_this->_ComParams.
																	_DisplayPorts._ErrorDisplay.
																	_IpAddress),
																 C_ERROR_DISPLAY_MAX_SIZE));
	}

	/* Init internal data */
  /*--------------------*/
	ptr_this->_simuIsRunning = FALSE;

	fprintf(stdout, "ERR_CTRL_Init() sucessful");

 FIN:
	if(rid != C_ERROR_OK)
	{
		fprintf(stderr, "Error %d during Error controller Init!", rid);
	}
	return (rid);
}


/*  @ROLE    : This function initialises Error Controller for current simulation
    @RETURN  : Error code */
T_ERROR ERR_CTRL_InitSimulation(
/*  IN     */ T_ERR_CTRL * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 simReference = 0;
	T_UINT16 simRef = 0, simRun = 0;
	T_FILE_PATH logFileName;
	T_ELT_GEN_PKT *eltGenPkt;
	fprintf(stdout, "init simu\n");
	/* Initialise Time */
  /*-----------------*/
	TIME_Init();

	/* check the simulation running status */
  /*-------------------------------------*/
	if(ptr_this->_simuIsRunning == TRUE)
		ERR_CTRL_EndSimulation(ptr_this, FALSE);

	/* extract sim reference from initpacket */
  /*---------------------------------------*/
	JUMP_ERROR(FIN, rid,
				  GENERIC_PACKET_GetEltPkt(ptr_this->_ReceivedPacket, 0,
													&eltGenPkt));
	simReference = eltGenPkt->_value;

	simRun = (T_UINT16) (simReference & 0x0000FFFF);
	simRef = (T_UINT16) ((simReference >> 16) & 0x0000FFFF);

	fprintf(stdout, "Init packet received with scenario_%d, run_%d",
			  simRef, simRun);


	/* Simu is now running properly for error controller */
  /*---------------------------------------------------*/
	ptr_this->_simuIsRunning = TRUE;

 FIN:
	if(rid != C_ERROR_OK)
	{
		fprintf(stdout, "Error %d during Error controller Simulation Init!", rid);
	}
	return (rid);
}


/*  @ROLE    : This function creates error messages and writes them to log file
    @RETURN  : Error code */
T_ERROR ERR_CTRL_DoPacket(
/*  IN     */ T_ERR_CTRL * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT16 i = 0;
	T_ELT_GEN_PKT *eltGenPkt;

	/* loop on element_number of received generic packet */
  /*---------------------------------------------------*/
	for(i = 0; i < ptr_this->_ReceivedPacket->_elementNumber; i++)
	{
		/* get current element of received generic packet */
		/*------------------------------------------------*/
		JUMP_ERROR(FIN, rid,
					  GENERIC_PACKET_GetEltPkt(ptr_this->_ReceivedPacket, i,
														&eltGenPkt));

		/* Format the error trace string for each element using */
		/*------------------------------------------------------*/
		JUMP_ERROR(FIN, rid,
					  T_ERROR_OUTPUT_FORMATTER_Formatter(&(ptr_this->_OutputFormat),
																	 &(ptr_this->
																		_ErrorsDefinition),
																	 ptr_this->_ReceivedPacket,
																	 eltGenPkt));

		/* Send message corresponding to this error */
		/*------------------------------------------*/
		ERR_CTRL_SendTrace(ptr_this, eltGenPkt);

		/* If error is a critical error category close Session  */
		/*------------------------------------------------------*/
		if(eltGenPkt->_categoryId == C_ERROR_CRITICAL)
		{
/* Send signal to end Scheduling controller */
/*------------------------------------------*/
/* Send kill signals to scheduling controller (using pid) on UNIX system */
/* Scheduling controller must implement signal handler corresponding to SIGALRM signal */
/*-------------------------------------------------------------------------------------*/
/* !CB signal not sent (change in the archiecture)
  kill((T_INT32)(ptr_this->_Pid), SIGALRM); 
*/
			fprintf(stdout,
					  "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
					  ptr_this->_Pid);
		}

	}

 FIN:
	return (rid);
}


/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ErrorControllerInterface class implements the error controller process 
    @HISTORY :
    03-02-24 : Creation
*/
/*--------------------------------------------------------------------------*/

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
#include "ErrorControllerInterface_e.h"
#include "ErrorController_e.h"
#include "ErrorDef_e.h"
#include "IPAddr_e.h"
#include "FilePath_e.h"
#include "unused.h"

#include <string.h>
#include <stdio.h>

/* Handling kill signals to scheduling controller (using pid) on UNIX system */
#   include <sys/types.h>
#   include <signal.h>

/* !CB set size to 6 pk because the socket is not read otherwise */
#define C_MAX_ERROR_PKT_ELT_NB 64	/* Maximum Number of Elements in 1 Error Packet */
#define C_LOG_FILE_NAME_DEFAULT "error_log.txt"	/* Name of Error log File created for each simulation */

#define C_ERROR_DISPLAY_MAX_SIZE 256	/* Maximum size of UDP packet sent to eror display */


T_ERROR startErrorControllerInterface(T_INT32 argc, T_CHAR * argv[])
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
					  "\t-d                   activate error external display\n");
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

	/* extract the pid of the start script */
  /*-------------------------------------*/
	_ctrl._Pid = getppid();

	/* Initialise config path and output path */
  /*----------------------------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_InitClass());

	/* init  error controller Session  */
  /*---------------------------------*/
	JUMP_ERROR(FIN, rid, ERR_CTRL_Init(&_ctrl, display));	/* config path and output path are arguments set when starting error controller */

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "==============================================================="));

/* infinite main loop of generic packet reception */
/* End of loop happens when maximum number of allowed consecutive errors is reached */
/*----------------------------------------------------------------------------------*/
	while(1)
	{
		if(GENERIC_PORT_RecvGenPacket
			(&(_ctrl._ServerErrPort), _ctrl._ReceivedPacket) == C_ERROR_OK)
		{

			TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_DEBUG,
						  "Receive data"));
			/* Get first element of received packet */
		/*--------------------------------------*/
			GENERIC_PACKET_GetEltPkt(_ctrl._ReceivedPacket, 0, &eltGenPkt);

			/* check category id */
		/*-------------------*/
			if(eltGenPkt->_categoryId == C_CAT_INIT)
			{							  /* Init packet */
				if(ERR_CTRL_InitSimulation(&_ctrl) != C_ERROR_OK)
				{
					TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR,
									 C_TRACE_ERROR, "ERR_CTRL_InitSimulation() failed"));
					ERR_CTRL_EndSimulation(&_ctrl, FALSE);
					/* sent SIGALRM signal to the start script */
					kill((T_INT32) (_ctrl._Pid), SIGALRM);
					TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR,
									 C_TRACE_ERROR,
									 "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
									 _ctrl._Pid));
				}
			}
			else if(eltGenPkt->_categoryId == C_CAT_END)
			{							  /* End simu packet */
				ERR_CTRL_EndSimulation(&_ctrl, TRUE);
			}
			else
			{
				if(_ctrl._simuIsRunning)
				{
					if(ERR_CTRL_DoPacket(&_ctrl) != C_ERROR_OK)
					{
						TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR,
										 C_TRACE_ERROR, "ERR_CTRL_DoPacket() failed"));
						ERR_CTRL_EndSimulation(&_ctrl, FALSE);
						/* sent SIGALRM signal to the start script */
						kill((T_INT32) (_ctrl._Pid), SIGALRM);
						TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR,
										 C_TRACE_ERROR,
										 "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
										 _ctrl._Pid));
					}
				}
				else
				{
					TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR,
									 C_TRACE_ERROR,
									 "GENERIC_PORT_RecvGenPacket() receive data without init packet"));
				}
			}
		}
		else
		{
			TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_ERROR,
							 "GENERIC_PORT_RecvGenPacket() failed"));
			if(_ctrl._simuIsRunning)
			{
				ERR_CTRL_EndSimulation(&_ctrl, FALSE);
				/* sent SIGALRM signal to the start script */
				kill((T_INT32) (_ctrl._Pid), SIGALRM);
				TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR,
								 C_TRACE_ERROR,
								 "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
								 _ctrl._Pid));
			}
		}
	}

 FIN:
	/* close Session  */
  /*----------------*/
	ERR_CTRL_Terminate(&_ctrl);

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
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "read com_parameters.conf file from config/exec directory"));
	JUMP_ERROR(FIN, rid, COM_PARAMETERS_ReadConfigFile(&(ptr_this->_ComParams)));

	/* Read Error definition configuration file */
  /*------------------------------------------*/
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "read Error_def.conf file from config/exec directory"));
	JUMP_ERROR(FIN, rid,
				  ERROR_DEF_ReadConfigFile(&(ptr_this->_ErrorsDefinition)));

	for(iError = 0; iError < ptr_this->_ErrorsDefinition._nbError; iError++)
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
					  "Read error definition <%s> in error_def.conf file ",
					  ptr_this->_ErrorsDefinition._Error[iError]._Name));
	}

	/* open the error generic port to receive error generic packet */
  /*-------------------------------------------------------------*/
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "open the generic port to receive generic packets"));
	JUMP_ERROR(FIN, rid, GENERIC_PORT_InitReceiver(&(ptr_this->_ServerErrPort),
																  &(ptr_this->_ComParams.
																	 _ControllersPorts.
																	 _ErrorController.
																	 _IpAddress),
																  C_MAX_ERROR_PKT_ELT_NB));
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "open error receiver %s port %d  done for error reception",
				  ptr_this->_ComParams._ControllersPorts._ErrorController.
				  _IpAddress._addr,
				  ptr_this->_ComParams._ControllersPorts._ErrorController.
				  _IpAddress._port));

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

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERR_CTRL_Init() sucessful"));

 FIN:
	if(rid != C_ERROR_OK)
	{
		TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_ERROR,
						 "Error %d during Error controller Init!", rid));
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

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "Init packet received with scenario_%d, run_%d", simRef, simRun));


	/* Get the output path */
  /*---------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_GetOutputPath(logFileName, simRef, simRun));

	/* Get the complete file name */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_Concat(logFileName, C_LOG_FILE_NAME_DEFAULT));

	/* create and open error log file using run path */
  /*-----------------------------------------------*/
	ptr_this->_TraceFile = fopen(logFileName, "w");
	if(ptr_this->_TraceFile == NULL)
	{
        TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
                    "cannot open file %s", logFileName))
		rid = C_ERROR_FILE_OPEN;
		goto FIN;
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "Opened error_log.txt file %s", logFileName));


	/* Write init message to log file */
  /*-------------------------------*/
	JUMP_ERROR(FIN, rid, ERR_CTRL_DoPacket(ptr_this));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERR_CTRL_SimulationInit() sucessful"));

	/* Simu is now running properly for error controller */
  /*---------------------------------------------------*/
	ptr_this->_simuIsRunning = TRUE;

 FIN:
	if(rid != C_ERROR_OK)
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
					  "Error %d during Error controller Simulation Init!", rid));
	}
	return (rid);
}


/*  @ROLE    : This function sets Error Controller in a proper state 
               at the end of current simulation
    @RETURN  : Error code */
T_ERROR ERR_CTRL_EndSimulation(
											/* INOUT */ T_ERR_CTRL * ptr_this,
											/* IN    */ T_BOOL storeError)
{
	T_ERROR rid = C_ERROR_OK;

	/* Write end message to log file */
  /*-------------------------------*/
	if(storeError == TRUE)
		JUMP_ERROR(FIN, rid, ERR_CTRL_DoPacket(ptr_this));

	/* Close log file       */
  /*----------------------*/
	if(ptr_this->_TraceFile != NULL)
		fclose(ptr_this->_TraceFile);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "Closed error_log.txt file "));

	/* close the udp port if display flag was set to true */
  /*----------------------------------------------------*/
	/* !CB no need to close the socket as long as it is only open at the beginnig 
	   if ( ptr_this->_DisplayFlag )  
	   UDP_SOCKET_Terminate(&(ptr_this->_DisplayPort));
	 */
	/* Re-Initialise internal data */
  /*--------------------------*/
	ptr_this->_simuIsRunning = FALSE;

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERR_CTRL_SimulationEnd() sucessful"));
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "==============================================================="));

 FIN:
	if(rid != C_ERROR_OK)
	{
		TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_ERROR,
						 "Error %d during ERR_CTRL_SimulationEnd() ", rid));
	}
	return rid;
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
			TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
						  "CRITICAL_ERROR : SIGALRM signal sent to Scheduling controller (pid=%d)",
						  ptr_this->_Pid));
		}

	}

 FIN:
	return (rid);
}


/*  @ROLE    : This function writes the error message into log file
               and, if expected, sends it to display.
    @RETURN  : Error code */
T_ERROR ERR_CTRL_SendTrace(
/*  IN     */ T_ERR_CTRL * ptr_this,
/*  IN     */ T_ELT_GEN_PKT * UNUSED(eltGenPkt)
	)
{
	T_CHAR errorMessage[C_ERROR_DISPLAY_MAX_SIZE];

	/* write error trace string to error log file */
  /*--------------------------------------------*/
	if(strcmp("END", ptr_this->_OutputFormat._category) == 0)
	{
		sprintf(errorMessage,
				  "FRSframe (%lu), FSM (%d), %s_%d, Category (%s), %s(%lu) ",
				  ptr_this->_OutputFormat._error_date._frame_number,
				  ptr_this->_OutputFormat._error_date._FSM_number,
				  ptr_this->_OutputFormat._error_origin._componentType,
				  ptr_this->_OutputFormat._error_origin._InstanceId,
				  ptr_this->_OutputFormat._category,
				  ptr_this->_OutputFormat._error_name,
				  ptr_this->_OutputFormat._error_index);
	}
	else
	{
		if(strcmp("0", ptr_this->_OutputFormat._index_signification) == 0)
		{
			sprintf(errorMessage,
					  "FRSframe (%lu), FSM (%d), %s_%d, Category (%s), %s(%lu), %s = %lu (Unit : %s) ",
					  ptr_this->_OutputFormat._error_date._frame_number,
					  ptr_this->_OutputFormat._error_date._FSM_number,
					  ptr_this->_OutputFormat._error_origin._componentType,
					  ptr_this->_OutputFormat._error_origin._InstanceId,
					  ptr_this->_OutputFormat._category,
					  ptr_this->_OutputFormat._error_name,
					  ptr_this->_OutputFormat._error_index,
					  ptr_this->_OutputFormat._value_signification,
					  ptr_this->_OutputFormat._value, ptr_this->_OutputFormat._unit);
		}
		else
		{
			sprintf(errorMessage,
					  "FRSframe (%lu), FSM (%d), %s_%d, Category (%s), %s(%lu), %s = %s(%lu), %s = %lu (Unit : %s) ",
					  ptr_this->_OutputFormat._error_date._frame_number,
					  ptr_this->_OutputFormat._error_date._FSM_number,
					  ptr_this->_OutputFormat._error_origin._componentType,
					  ptr_this->_OutputFormat._error_origin._InstanceId,
					  ptr_this->_OutputFormat._category,
					  ptr_this->_OutputFormat._error_name,
					  ptr_this->_OutputFormat._error_index,
					  ptr_this->_OutputFormat._index_signification,
					  ptr_this->_OutputFormat._index_value,
					  ptr_this->_OutputFormat._index,
					  ptr_this->_OutputFormat._value_signification,
					  ptr_this->_OutputFormat._value, ptr_this->_OutputFormat._unit);
		}

	}


	fprintf(ptr_this->_TraceFile, "ERROR : %s\n", errorMessage);
	fflush(ptr_this->_TraceFile);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "Received message : %s", errorMessage));


	/* if (Display UDP port open) send to Display UDP port */
  /*-----------------------------------------------------*/
	if(ptr_this->_DisplayFlag)
	{
		UDP_SOCKET_SendBytes(&(ptr_this->_DisplayPort), (T_BUFFER) errorMessage,
									(T_INT32) (strlen(errorMessage)));
	}

	return C_ERROR_OK;
}


/*  @ROLE    : This function stops Error controller properly.
    @RETURN  : Error code */
T_ERROR ERR_CTRL_Terminate(
									  /* INOUT */ T_ERR_CTRL * ptr_this)
{

	/* free the generic packet */
  /*-------------------------*/
	GENERIC_PACKET_Delete(&(ptr_this->_ReceivedPacket));

	/* close the generic port */
  /*------------------------*/
	GENERIC_PORT_Terminate(&(ptr_this->_ServerErrPort));

	/* close the udp port if display flag was set to true */
  /*----------------------------------------------------*/
	if(ptr_this->_DisplayFlag)
		UDP_SOCKET_Terminate(&(ptr_this->_DisplayPort));

	/* Send signal to end Scheduling controller */
  /*------------------------------------------*/
  /* Send kill signals to scheduling controller (using pid) on UNIX system */
  /* Scheduling controller must implement signal handler corresponding to SIGALRM signal */
  /*-------------------------------------------------------------------------------------*/
	kill((T_INT32) (ptr_this->_Pid), SIGALRM);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "SIGALRM signal sent to Scheduling controller (pid=%d)",
				  ptr_this->_Pid));

	/* Close log file       */
  /*----------------------*/
	if(ptr_this->_TraceFile != NULL)
	{
		fclose(ptr_this->_TraceFile);

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
					  "Closed error_log.txt file "));
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERR_CTRL_Terminate() sucessful"));

	return C_ERROR_OK;
}

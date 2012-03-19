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
 * @file Trace.c
 * @author TAS
 * @brief The Trace class implements the trace mechanism
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Trace_e.h"
#include "Time_e.h"

/* global declaration */
#if defined(_ASP_TRACE) || defined(_ASP_TESTER)
T_BOOL _trace_activationFlag[C_TRACE_MAX_INDEX];
T_UINT64 _trace_levelFlag[C_TRACE_MAX_INDEX];
#endif /*_ASP_TRACE  */

static T_STRING _trace_component[] = { "ST1", "ST2", "ST3", "ST4", "ST5", "ST6",
	"TG1", "TG2", "TG3", "TG4", "TG5", "TG6", "TG7", "TG8",
	"EIA_IN1", "EIA_IN2", "EIA_IN3", "EIA_IN4", "EIA_IN5",
	"EIA_IN6", "EIA_IN7",
	"EIA_OUT1", "EIA_OUT2", "EIA_OUT3", "EIA_OUT4", "EIA_OUT5",
	"EIA_OUT6", "EIA_OUT7",
	"ST_AGG",
	"NAT", "NCC_ALLOC", "NCC_UL", "NCC_DL", "OBP",
	"OBPC",
	"SCHED_MAIN", "SCHED_MASTER", "SCHED_SLAVE",
	"TESTER",
	"ST",
	"TG",
	"EIA_IN", "EIA_OUT", "ST_AGG", "NAT", "NCC",
	"OBP", "OBPC", "AIE_IN", "AIE_OUT", "CONFIG",
	"INTERFACES", "SHARED_MEMORY", "TRANSPORT", "UTILITIES", "SCHEDULER",
	"PROBE", "ERROR", "EVENT", "PROTOCOL", "TESTER", "OBPCTESTER", "UNKNOWN"
};

/*  @ROLE    : This function is used to print trace error in force mode
    @RETURN  : None */
void TRACE_ForcePrintf(
								 /* IN    */ T_TRACE_THREAD_TYPE traceThread,
								 /* IN    */ T_TRACE_COMPONENT_TYPE traceComponent,
								 /* IN    */ T_TRACE_LEVEL traceLevel,
								 /* IN    */ T_CHAR * format, ...)
{
	va_list args;
	T_CHAR text[C_TRACE_STR_MAX_SIZE] =
	{
	'\0'};

	/* start the variable list */
	va_start(args, format);

	/* format and send the message */
	if(vsnprintf((char *) &text[0], (C_TRACE_STR_MAX_SIZE - 60), format, args) <
		0)
	{
		fprintf(stderr, "T[%s] C[%s] l[%llx]:CANNOT PRINT MSG !!!!!\n",
				  (traceThread ==
					C_TRACE_THREAD_UNKNOWN ? "UNKNOWN" :
					_trace_component[traceThread]),
				  _trace_component[C_TRACE_THREAD_MAX + traceComponent],
				  traceLevel);
		fflush(stderr);
		exit(-1);
	}
	else
	{
		fprintf(stderr, "T[%s] C[%s] l[%llx]:%s\n",
				  (traceThread ==
					C_TRACE_THREAD_UNKNOWN ? "UNKNOWN" :
					_trace_component[traceThread]),
				  _trace_component[C_TRACE_THREAD_MAX + traceComponent], traceLevel,
				  text);

		fflush(stderr);
	}

	/* close the variable list */
	va_end(args);
}


/*  @ROLE    : This function is used to print trace information
    @RETURN  : None */
void TRACE_Printf(
						  /* IN    */ T_TRACE_THREAD_TYPE traceThread,
						  /* IN    */ T_TRACE_COMPONENT_TYPE traceComponent,
						  /* IN    */ T_TRACE_LEVEL traceLevel,
						  /* IN    */ T_CHAR * format, ...)
{
#ifdef _ASP_TRACE
	va_list args;
	T_CHAR text[C_TRACE_STR_MAX_SIZE] =
	{
	'\0'}, message[C_TRACE_STR_MAX_SIZE] =
	{
	'\0'};
	FILE * strm = stdout;

#   ifdef _ASP_TRACE_TIME
	T_TIME timeValue;
#   endif
		 /* _ASP_TRACE_TIME */

	/* check the activation flag */
	if(((traceThread != C_TRACE_THREAD_UNKNOWN)
		 && (_trace_activationFlag[traceThread] == TRUE))
		|| (_trace_activationFlag[C_TRACE_THREAD_MAX + traceComponent] == TRUE)
		|| (traceLevel == C_TRACE_ERROR) || (traceLevel == C_TRACE_MINOR))
	{

		/* check the level flag */
		if(((traceThread != C_TRACE_THREAD_UNKNOWN)
			 && (_trace_levelFlag[traceThread] & traceLevel))
			|| (_trace_levelFlag[C_TRACE_THREAD_MAX + traceComponent] & traceLevel)
			|| (traceLevel == C_TRACE_ERROR) || (traceLevel == C_TRACE_MINOR))
		{

#   ifdef _ASP_TRACE_TIME
			/* get time */
			timeValue = TIME_GetTime();
			sprintf((char *) &message[0], "[%.03lf]", timeValue);
#   else
		/* _ASP_TRACE_TIME */
			message[0] = '\0';
#   endif
		 /* _ASP_TRACE_TIME */
#   if ((defined(_ASP_TRACE) || defined(_ASP_TESTER)) && !defined(_ASP_TRACE_NO_FSM_COUNT))
			/* !CB
			   sprintf((char*)&text[0],"<%03d>",_SchedulingCtlr._MasterFSMCountTrace);
			 */
			strcat((char *) &message[0], (char *) &text[0]);
#   endif
			strcat((char *) &message[0], "\t");

			/* check trace level */
			if((traceLevel >= C_TRACE_DEBUG_0) && (traceLevel < C_TRACE_VALID_0))
			{
				T_UINT64 flag;
				T_UINT32 i = 0;
				for(flag = C_TRACE_DEBUG_0; flag < C_TRACE_VALID_0; flag <<= 1)
				{
					if(traceLevel & flag)
						break;
					i++;
				}
				sprintf((char *) &text[0], "D%lu/", i);
				strcat((char *) &message[0], (char *) &text[0]);
			}
			else if((traceLevel >= C_TRACE_VALID_0) && (traceLevel < C_TRACE_FUNC))
			{
				T_UINT64 flag;
				T_UINT32 i = 0;
				for(flag = C_TRACE_VALID_0; flag < C_TRACE_FUNC; flag <<= 1)
				{
					if(traceLevel & flag)
						break;
					i++;
				}
				sprintf((char *) &text[0], "V%lu/", i);
				strcat((char *) &message[0], (char *) &text[0]);
			}
			else if(traceLevel == C_TRACE_FUNC)
				strcat((char *) &message[0], "F/");
			else if(traceLevel == C_TRACE_MINOR)
				strcat((char *) &message[0], "M/");
			else if(traceLevel == C_TRACE_ERROR)
			{
				strcat((char *) &message[0], "E/");
				strm = stderr;
			}
			else
				strcat((char *) &message[0], "U/");

			/* print the thread or component name */
			if(traceThread != C_TRACE_THREAD_UNKNOWN)
				sprintf((char *) &text[0], "T(%s): ",
						  _trace_component[traceThread]);
			else
				sprintf((char *) &text[0], "C(%s): ",
						  _trace_component[C_TRACE_THREAD_MAX + traceComponent]);
			strcat((char *) &message[0], (char *) &text[0]);

			/* start the variable list */
			va_start(args, format);
			/* format and send the message */
			if(vsnprintf
				((char *) &text[0], (C_TRACE_STR_MAX_SIZE - 40), format, args) < 0)
			{
				fprintf(stderr, "T[%d] C[%d] l[%llx]:CANNOT PRINT MSG !!!!!\n",
						  traceThread, traceComponent, traceLevel);
				fflush(stderr);
				exit(-1);
			}
			else if(strlen(text) >= (C_TRACE_STR_MAX_SIZE - 40))
			{
				fprintf(stderr, "\n!!!!!! TRACE LENGTH TOO LONG !!!!!!!\n\n");
				fprintf(stderr, "TRACE :%s\n", text);
				fflush(stderr);
				exit(-1);
			}

			strcat((char *) &message[0], (char *) &text[0]);
			/* print the trace message */
			fprintf(strm, "%s\n", message);
			fflush(strm);
    		syslog(LOG_ERR,  "%s\n", message);

			/* close the variable list */
			va_end(args);
		}
	}
#else	/*_ASP_TRACE  */
	fprintf(stderr, "The _ASP_TRACE is not set:DO NOT USE TRACE_Printf\n");
#endif /*_ASP_TRACE  */
}




/*  @ROLE    : This function is used to print trace information to a stream
    @RETURN  : None */
void TRACE_Fprintf(
							/* IN    */ T_TRACE_THREAD_TYPE traceThread,
							/* IN    */ T_TRACE_COMPONENT_TYPE traceComponent,
							/* IN    */ T_TRACE_LEVEL traceLevel,
							/* IN    */ FILE * stream,
							/* IN    */ T_CHAR * format, ...)
{
#if defined(_ASP_TRACE) || defined(_ASP_TESTER)
	va_list args;
	T_CHAR text[C_TRACE_PACKET_STR_MAX_SIZE] =
	{
	'\0'}, message[C_TRACE_PACKET_STR_MAX_SIZE] =
	{
	'\0'};

#   ifdef _ASP_TRACE_TIME
	T_TIME timeValue;
#   endif
		 /* _ASP_TRACE_TIME */

	/* check the activation flag */
	if(((traceThread != C_TRACE_THREAD_UNKNOWN)
		 && (_trace_activationFlag[traceThread] == TRUE))
		|| (_trace_activationFlag[C_TRACE_THREAD_MAX + traceComponent] == TRUE)
		|| (traceLevel == C_TRACE_ERROR) || (traceLevel == C_TRACE_MINOR))
	{

		/* check the level flag */
		if(((traceThread != C_TRACE_THREAD_UNKNOWN)
			 && (_trace_levelFlag[traceThread] & traceLevel))
			|| (_trace_levelFlag[C_TRACE_THREAD_MAX + traceComponent] & traceLevel)
			|| (traceLevel == C_TRACE_ERROR) || (traceLevel == C_TRACE_MINOR))
		{

#   ifdef _ASP_TRACE_TIME
			/* get time */
			timeValue = TIME_GetTime();
			sprintf((char *) &message[0], "[%.03lf]", timeValue);
#   else
		/* _ASP_TRACE_TIME */
			message[0] = '\0';
#   endif
		 /* _ASP_TRACE_TIME */

#   if ((defined(_ASP_TRACE) || defined(_ASP_TESTER)) && !defined(_ASP_TRACE_NO_FSM_COUNT))
			/* !CB pas de FSM count
			   sprintf((char*)&text[0],"<%03d>",_SchedulingCtlr._MasterFSMCountTrace);
			   strcat((char*)&message[0],(char*)&text[0]);
			 */
#   endif
			strcat((char *) &message[0], "\t");

			/* check trace level */
			if((traceLevel >= C_TRACE_DEBUG_0) && (traceLevel < C_TRACE_VALID_0))
			{
				T_UINT64 flag;
				T_UINT32 i = 0;
				for(flag = C_TRACE_DEBUG_0; flag < C_TRACE_VALID_0; flag <<= 1)
				{
					if(traceLevel & flag)
						break;
					i++;
				}
				sprintf((char *) &text[0], "D%lu/", i);
				strcat((char *) &message[0], (char *) &text[0]);
			}
			else if((traceLevel >= C_TRACE_VALID_0) && (traceLevel < C_TRACE_FUNC))
			{
				T_UINT64 flag;
				T_UINT32 i = 0;
				for(flag = C_TRACE_VALID_0; flag < C_TRACE_FUNC; flag <<= 1)
				{
					if(traceLevel & flag)
						break;
					i++;
				}
				sprintf((char *) &text[0], "V%lu/", i);
				strcat((char *) &message[0], (char *) &text[0]);
			}
			else if(traceLevel == C_TRACE_FUNC)
				strcat((char *) &message[0], "F/");
			else if(traceLevel == C_TRACE_MINOR)
				strcat((char *) &message[0], "M/");
			else if(traceLevel == C_TRACE_ERROR)
				strcat((char *) &message[0], "E/");
			else
				strcat((char *) &message[0], "U/");

			/* print the thread or component name */
			if(traceThread != C_TRACE_THREAD_UNKNOWN)
				sprintf((char *) &text[0], "T(%s): ",
						  _trace_component[traceThread]);
			else
				sprintf((char *) &text[0], "C(%s): ",
						  _trace_component[C_TRACE_THREAD_MAX + traceComponent]);
			strcat((char *) &message[0], (char *) &text[0]);

			/* start the variable list */
			va_start(args, format);

			/* format and send the message */
			if(vsnprintf
				((char *) &text[0], (C_TRACE_PACKET_STR_MAX_SIZE - 40), format,
				 args) < 0)
			{
				fprintf(stderr, "T[%d] C[%d] l[%llx]:CANNOT PRINT MSG !!!!!\n",
						  traceThread, traceComponent, traceLevel);
				fflush(stderr);
				exit(-1);
			}
			else if(strlen(text) >= (C_TRACE_PACKET_STR_MAX_SIZE - 40))
			{
				fprintf(stderr, "\n!!!!!! TRACE LENGTH TOO LONG !!!!!!!\n\n");
				fprintf(stderr, "TRACE :%s\n", text);
				fflush(stderr);
				exit(-1);
			}

			strcat((char *) &message[0], (char *) &text[0]);

			/* print the trace message */
			fprintf(stream, "%s\n", message);
			fflush(stream);

			/* close the variable list */
			va_end(args);
		}
	}
#else	/*_ASP_TRACE  */
	fprintf(stderr, "The _ASP_TRACE is not set:DO NOT USE TRACE_Printf\n");
#endif /*_ASP_TRACE  */
}


/*  @ROLE    : This function activates some trace display
    @RETURN  : None */
void TRACE_ActivateTrace(
									/* IN    */ T_UINT64 traceType,
									/* IN    */ T_UINT64 traceLevel)
{
#if defined(_ASP_TRACE) || defined(_ASP_TESTER)
	T_UINT32 i;
	T_UINT64 flag = TRACE_INIT_ULL;

	for(i = 0; i < C_TRACE_MAX_INDEX; i++)
	{
		if(traceType & flag)
		{
			_trace_activationFlag[i] = TRUE;
			_trace_levelFlag[i] = traceLevel;
		}
		flag <<= 1;
	}
#else	/*_ASP_TRACE  */
	fprintf(stderr,
			  "The _ASP_TRACE is not set:DO NOT USE TRACE_ActivateTrace\n");
#endif /*_ASP_TRACE  */
}


/*  @ROLE    : This function activates all trace display
    @RETURN  : None */
void TRACE_ActivateAllTrace(
										/* IN    */ T_UINT64 traceLevel)
{
#ifdef _ASP_TRACE
	T_UINT32 i;
	memset(&_trace_activationFlag, TRUE, sizeof(_trace_activationFlag));
	for(i = 0; i < C_TRACE_MAX_INDEX; i++)
	{
		_trace_levelFlag[i] = traceLevel;
	}
#else	/*_ASP_TRACE  */
	fprintf(stderr,
			  "The _ASP_TRACE is not set:DO NOT USE TRACE_ActivateAllTrace\n");
#endif /*_ASP_TRACE  */
}


/*  @ROLE    : This function disactivates some trace display
    @RETURN  : None */
void TRACE_DisactivateTrace(
										/* IN    */ T_UINT64 traceType)
{
#ifdef _ASP_TRACE
	T_UINT32 i;
	T_UINT64 flag = TRACE_INIT_ULL;

	for(i = 0; i < C_TRACE_MAX_INDEX; i++)
	{
		if(traceType & flag)
		{
			_trace_activationFlag[i] = FALSE;
			_trace_levelFlag[i] = 0;
		}
		flag <<= 1;
	}
#else	/*_ASP_TRACE  */
	fprintf(stderr,
			  "The _ASP_TRACE is not set:DO NOT USE TRACE_DisactivateTrace\n");
#endif /*_ASP_TRACE  */
}


/*  @ROLE    : This function disactivates all trace display
    @RETURN  : None */
void TRACE_DisactivateAllTrace(void)
{
#ifdef _ASP_TRACE
	memset(&_trace_activationFlag, FALSE, sizeof(_trace_activationFlag));
	memset(&_trace_levelFlag, 0, sizeof(_trace_activationFlag));
#else	/*_ASP_TRACE  */
	fprintf(stderr,
			  "The _ASP_TRACE is not set:DO NOT USE TRACE_DisactivateAllTrace\n");
#endif /*_ASP_TRACE  */
}

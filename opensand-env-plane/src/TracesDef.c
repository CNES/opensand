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
 * @file TracesDef.c
 * @author TAS
 * @brief The TracesDefinition class implements the reading of Traces
 *        definition configuration file
 */

#include "FileReader_e.h"
#include "TracesDef_e.h"
#include "FilePath_e.h"
#include "FileInfos_e.h"
#include "Trace_e.h"

#include <string.h>


T_ERROR TRACES_DEF_Init(
								  /* INOUT */ T_TRACES_DEF * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_TRACES_DEF));

	strcpy(ptr_this->C_TRACE_MODE_choices[0]._StrValue, "C_TRACE_DEBUG");
	strcpy(ptr_this->C_TRACE_MODE_choices[1]._StrValue, "C_TRACE_DEBUG_0");
	strcpy(ptr_this->C_TRACE_MODE_choices[2]._StrValue, "C_TRACE_DEBUG_1");
	strcpy(ptr_this->C_TRACE_MODE_choices[3]._StrValue, "C_TRACE_DEBUG_2");
	strcpy(ptr_this->C_TRACE_MODE_choices[4]._StrValue, "C_TRACE_DEBUG_3");
	strcpy(ptr_this->C_TRACE_MODE_choices[5]._StrValue, "C_TRACE_DEBUG_4");
	strcpy(ptr_this->C_TRACE_MODE_choices[6]._StrValue, "C_TRACE_DEBUG_5");
	strcpy(ptr_this->C_TRACE_MODE_choices[7]._StrValue, "C_TRACE_DEBUG_6");
	strcpy(ptr_this->C_TRACE_MODE_choices[8]._StrValue, "C_TRACE_DEBUG_7");
	strcpy(ptr_this->C_TRACE_MODE_choices[29]._StrValue, "C_TRACE_VALID");
	strcpy(ptr_this->C_TRACE_MODE_choices[30]._StrValue, "C_TRACE_VALID_0");
	strcpy(ptr_this->C_TRACE_MODE_choices[31]._StrValue, "C_TRACE_VALID_1");
	strcpy(ptr_this->C_TRACE_MODE_choices[32]._StrValue, "C_TRACE_VALID_2");
	strcpy(ptr_this->C_TRACE_MODE_choices[33]._StrValue, "C_TRACE_VALID_3");
	strcpy(ptr_this->C_TRACE_MODE_choices[34]._StrValue, "C_TRACE_VALID_4");
	strcpy(ptr_this->C_TRACE_MODE_choices[35]._StrValue, "C_TRACE_VALID_5");
	strcpy(ptr_this->C_TRACE_MODE_choices[36]._StrValue, "C_TRACE_VALID_6");
	strcpy(ptr_this->C_TRACE_MODE_choices[37]._StrValue, "C_TRACE_VALID_7");
	strcpy(ptr_this->C_TRACE_MODE_choices[58]._StrValue, "C_TRACE_FUNC");
	strcpy(ptr_this->C_TRACE_MODE_choices[59]._StrValue, "C_TRACE_ERROR");
	ptr_this->C_TRACE_MODE_choices[60]._StrValue[0] = '\0';
	ptr_this->C_TRACE_MODE_choices[0]._IntValue = C_TRACE_DEBUG;
	ptr_this->C_TRACE_MODE_choices[1]._IntValue = C_TRACE_DEBUG_0;
	ptr_this->C_TRACE_MODE_choices[2]._IntValue = C_TRACE_DEBUG_1;
	ptr_this->C_TRACE_MODE_choices[3]._IntValue = C_TRACE_DEBUG_2;
	ptr_this->C_TRACE_MODE_choices[4]._IntValue = C_TRACE_DEBUG_3;
	ptr_this->C_TRACE_MODE_choices[5]._IntValue = C_TRACE_DEBUG_4;
	ptr_this->C_TRACE_MODE_choices[6]._IntValue = C_TRACE_DEBUG_5;
	ptr_this->C_TRACE_MODE_choices[7]._IntValue = C_TRACE_DEBUG_6;
	ptr_this->C_TRACE_MODE_choices[29]._IntValue = C_TRACE_VALID;
	ptr_this->C_TRACE_MODE_choices[30]._IntValue = C_TRACE_VALID_0;
	ptr_this->C_TRACE_MODE_choices[31]._IntValue = C_TRACE_VALID_1;
	ptr_this->C_TRACE_MODE_choices[32]._IntValue = C_TRACE_VALID_2;
	ptr_this->C_TRACE_MODE_choices[33]._IntValue = C_TRACE_VALID_3;
	ptr_this->C_TRACE_MODE_choices[34]._IntValue = C_TRACE_VALID_4;
	ptr_this->C_TRACE_MODE_choices[35]._IntValue = C_TRACE_VALID_5;
	ptr_this->C_TRACE_MODE_choices[36]._IntValue = C_TRACE_VALID_6;
	ptr_this->C_TRACE_MODE_choices[37]._IntValue = C_TRACE_VALID_7;
	ptr_this->C_TRACE_MODE_choices[58]._IntValue = C_TRACE_FUNC;
	ptr_this->C_TRACE_MODE_choices[59]._IntValue = C_TRACE_ERROR;
	ptr_this->C_TRACE_MODE_choices[60]._IntValue = 0;

	strcpy(ptr_this->C_TRACE_COMP_choices[0]._StrValue, "C_TRACE_THREAD_ST");
	strcpy(ptr_this->C_TRACE_COMP_choices[1]._StrValue, "C_TRACE_THREAD_ST_1");
	strcpy(ptr_this->C_TRACE_COMP_choices[2]._StrValue, "C_TRACE_THREAD_ST_2");
	strcpy(ptr_this->C_TRACE_COMP_choices[3]._StrValue, "C_TRACE_THREAD_ST_3");
	strcpy(ptr_this->C_TRACE_COMP_choices[4]._StrValue, "C_TRACE_THREAD_ST_4");
	strcpy(ptr_this->C_TRACE_COMP_choices[5]._StrValue, "C_TRACE_THREAD_ST_5");
	strcpy(ptr_this->C_TRACE_COMP_choices[6]._StrValue, "C_TRACE_THREAD_TG");
	strcpy(ptr_this->C_TRACE_COMP_choices[7]._StrValue, "C_TRACE_THREAD_TG_1");
	strcpy(ptr_this->C_TRACE_COMP_choices[8]._StrValue, "C_TRACE_THREAD_TG_2");
	strcpy(ptr_this->C_TRACE_COMP_choices[9]._StrValue, "C_TRACE_THREAD_TG_3");
	strcpy(ptr_this->C_TRACE_COMP_choices[10]._StrValue, "C_TRACE_THREAD_TG_4");
	strcpy(ptr_this->C_TRACE_COMP_choices[11]._StrValue, "C_TRACE_THREAD_TG_5");
	strcpy(ptr_this->C_TRACE_COMP_choices[12]._StrValue, "C_TRACE_THREAD_TG_6");
	strcpy(ptr_this->C_TRACE_COMP_choices[13]._StrValue, "C_TRACE_THREAD_TG_7");

	strcpy(ptr_this->C_TRACE_COMP_choices[28]._StrValue,
			 "C_TRACE_THREAD_ST_AGG");
	strcpy(ptr_this->C_TRACE_COMP_choices[29]._StrValue, "C_TRACE_THREAD_GW");

	strcpy(ptr_this->C_TRACE_COMP_choices[30]._StrValue, "C_TRACE_THREAD_NCC");

	strcpy(ptr_this->C_TRACE_COMP_choices[33]._StrValue, "C_TRACE_THREAD_OBP");
	strcpy(ptr_this->C_TRACE_COMP_choices[34]._StrValue, "C_TRACE_THREAD_OBPC");
	strcpy(ptr_this->C_TRACE_COMP_choices[38]._StrValue,
			 "C_TRACE_THREAD_TESTER");

	strcpy(ptr_this->C_TRACE_COMP_choices[39]._StrValue, "C_TRACE_COMP_ST");
	strcpy(ptr_this->C_TRACE_COMP_choices[40]._StrValue, "C_TRACE_COMP_TG");

	strcpy(ptr_this->C_TRACE_COMP_choices[43]._StrValue, "C_TRACE_COMP_ST_AGG");

	strcpy(ptr_this->C_TRACE_COMP_choices[44]._StrValue, "C_TRACE_COMP_GW");

	strcpy(ptr_this->C_TRACE_COMP_choices[45]._StrValue, "C_TRACE_COMP_NCC");

	strcpy(ptr_this->C_TRACE_COMP_choices[46]._StrValue, "C_TRACE_COMP_OBP");
	strcpy(ptr_this->C_TRACE_COMP_choices[47]._StrValue, "C_TRACE_COMP_OBPC");
	strcpy(ptr_this->C_TRACE_COMP_choices[50]._StrValue, "C_TRACE_COMP_CONFIG");
	strcpy(ptr_this->C_TRACE_COMP_choices[51]._StrValue,
			 "C_TRACE_COMP_INTERFACES");
	strcpy(ptr_this->C_TRACE_COMP_choices[52]._StrValue,
			 "C_TRACE_COMP_SHARED_MEMORY");
	strcpy(ptr_this->C_TRACE_COMP_choices[53]._StrValue,
			 "C_TRACE_COMP_TRANSPORT");
	strcpy(ptr_this->C_TRACE_COMP_choices[54]._StrValue,
			 "C_TRACE_COMP_UTILITIES");
	strcpy(ptr_this->C_TRACE_COMP_choices[56]._StrValue, "C_TRACE_COMP_PROBE");
	strcpy(ptr_this->C_TRACE_COMP_choices[57]._StrValue, "C_TRACE_COMP_ERROR");
	strcpy(ptr_this->C_TRACE_COMP_choices[58]._StrValue, "C_TRACE_COMP_EVENT");
	strcpy(ptr_this->C_TRACE_COMP_choices[59]._StrValue,
			 "C_TRACE_COMP_PROTOCOL");
	strcpy(ptr_this->C_TRACE_COMP_choices[60]._StrValue, "C_TRACE_COMP_TESTER");

	ptr_this->C_TRACE_COMP_choices[62]._StrValue[0] = '\0';

	ptr_this->C_TRACE_COMP_choices[0]._IntValue = C_TRACE_TT_THREAD_ST;
	ptr_this->C_TRACE_COMP_choices[1]._IntValue = C_TRACE_TT_THREAD_ST_1;
	ptr_this->C_TRACE_COMP_choices[2]._IntValue = C_TRACE_TT_THREAD_ST_2;
	ptr_this->C_TRACE_COMP_choices[3]._IntValue = C_TRACE_TT_THREAD_ST_3;
	ptr_this->C_TRACE_COMP_choices[4]._IntValue = C_TRACE_TT_THREAD_ST_4;
	ptr_this->C_TRACE_COMP_choices[5]._IntValue = C_TRACE_TT_THREAD_ST_5;
	ptr_this->C_TRACE_COMP_choices[28]._IntValue = C_TRACE_TT_THREAD_ST_AGG;

	ptr_this->C_TRACE_COMP_choices[6]._IntValue = C_TRACE_TT_THREAD_TG;
	ptr_this->C_TRACE_COMP_choices[7]._IntValue = C_TRACE_TT_THREAD_TG_1;
	ptr_this->C_TRACE_COMP_choices[8]._IntValue = C_TRACE_TT_THREAD_TG_2;
	ptr_this->C_TRACE_COMP_choices[9]._IntValue = C_TRACE_TT_THREAD_TG_3;
	ptr_this->C_TRACE_COMP_choices[10]._IntValue = C_TRACE_TT_THREAD_TG_4;
	ptr_this->C_TRACE_COMP_choices[11]._IntValue = C_TRACE_TT_THREAD_TG_5;
	ptr_this->C_TRACE_COMP_choices[12]._IntValue = C_TRACE_TT_THREAD_TG_6;
	ptr_this->C_TRACE_COMP_choices[13]._IntValue = C_TRACE_TT_THREAD_TG_7;
	ptr_this->C_TRACE_COMP_choices[29]._IntValue = C_TRACE_TT_THREAD_GW;

	ptr_this->C_TRACE_COMP_choices[30]._IntValue = C_TRACE_TT_THREAD_NCC;

	ptr_this->C_TRACE_COMP_choices[33]._IntValue = C_TRACE_TT_THREAD_OBP;
	ptr_this->C_TRACE_COMP_choices[34]._IntValue = C_TRACE_TT_THREAD_OBPC;
	ptr_this->C_TRACE_COMP_choices[38]._IntValue = C_TRACE_TT_THREAD_TESTER;

	ptr_this->C_TRACE_COMP_choices[39]._IntValue = C_TRACE_TT_COMP_ST;
	ptr_this->C_TRACE_COMP_choices[43]._IntValue = C_TRACE_TT_COMP_ST_AGG;

	ptr_this->C_TRACE_COMP_choices[40]._IntValue = C_TRACE_TT_COMP_TG;
	ptr_this->C_TRACE_COMP_choices[44]._IntValue = C_TRACE_TT_COMP_GW;

	ptr_this->C_TRACE_COMP_choices[45]._IntValue = C_TRACE_TT_COMP_NCC;

	ptr_this->C_TRACE_COMP_choices[46]._IntValue = C_TRACE_TT_COMP_OBP;
	ptr_this->C_TRACE_COMP_choices[47]._IntValue = C_TRACE_TT_COMP_OBPC;
	ptr_this->C_TRACE_COMP_choices[50]._IntValue = C_TRACE_TT_COMP_CONFIG;
	ptr_this->C_TRACE_COMP_choices[51]._IntValue = C_TRACE_TT_COMP_INTERFACES;
	ptr_this->C_TRACE_COMP_choices[52]._IntValue = C_TRACE_TT_COMP_SHARED_MEMORY;
	ptr_this->C_TRACE_COMP_choices[53]._IntValue = C_TRACE_TT_COMP_TRANSPORT;
	ptr_this->C_TRACE_COMP_choices[54]._IntValue = C_TRACE_TT_COMP_UTILITIES;
	ptr_this->C_TRACE_COMP_choices[56]._IntValue = C_TRACE_TT_COMP_PROBE;
	ptr_this->C_TRACE_COMP_choices[57]._IntValue = C_TRACE_TT_COMP_ERROR;
	ptr_this->C_TRACE_COMP_choices[58]._IntValue = C_TRACE_TT_COMP_EVENT;
	ptr_this->C_TRACE_COMP_choices[59]._IntValue = C_TRACE_TT_COMP_PROTOCOL;
	ptr_this->C_TRACE_COMP_choices[60]._IntValue = C_TRACE_TT_COMP_TESTER;
	ptr_this->C_TRACE_COMP_choices[62]._IntValue = 0;

	return rid;
}


static T_ERROR ReadTraces(
									 /* IN    */ T_FILE_READER * ptr_reader,
									 /* OUT */ T_TRACES_DEF * ptr_traces,
									 /* IN    */ T_INT32 traceIndex)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE];
	T_UINT32 iLoop;

/* initialise line parsing */
	LINE_PARSER_Init(&(ptr_reader->_Parser));

/* Read current line */
/* read Trace Mode  value */
	rid = FILE_READER_ReadLine(ptr_reader, ptr_reader->_Parser._LineBuffer);
	if((rid == C_ERROR_FILE_READ) && (traceIndex != 0))
	{
		rid = C_ERROR_BAD_PARAM;
	}

	for(iLoop = 0; iLoop < C_FR_MAX_LINE; iLoop++)
	{
		readString[iLoop] = '\0';
	}

	rid =
		LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
										readString);

	if((strlen(readString) != 0) &&
		(strstr(readString, "{") == NULL) &&
		(strstr(readString, "}") == NULL) &&
		(strstr(readString, "Trace_number") == NULL))
	{

		/* Get integer value from enum read */
		JUMP_ERROR(FIN, rid, ENUM_PARSER_ParseLong(readString,
																 ptr_traces->
																 C_TRACE_COMP_choices,
																 &(ptr_traces->
																	_Trace[traceIndex]._Name)));

		/* read Trace Mode */
		JUMP_ERROR(FIN, rid, LINE_PARSER_ParseEnumLong(&(ptr_reader->_Parser),
																	  ptr_traces->
																	  C_TRACE_MODE_choices,
																	  (T_INT64 *) & (ptr_traces->
																						  _Trace
																						  [traceIndex].
																						  _Mode)));

	}
	else
	{
		if(strlen(readString) == 0)
		{
			rid = C_ERROR_ALLOC;
		}
		else
		{
			rid = C_ERROR_BAD_PARAM;
		}
	}

 FIN:
	return rid;
}


T_ERROR TRACES_DEF_ReadConfigNamedFile(
													  /* INOUT */ T_TRACES_DEF * ptr_this,
													  /* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_READER config_reader;
	T_INT32 read_nb_loop = 0;

	/* Initialise config_reader */
	JUMP_ERROR(FIN1, rid, FILE_READER_Init(&config_reader));

	/* Initialise T_EVENTS_DEF structure */
	JUMP_ERROR(FIN1, rid, TRACES_DEF_Init(ptr_this));

	/* Begin file reading */
	JUMP_ERROR(FIN1, rid, FILE_READER_OpenFile(&config_reader, name));

	/* Loop reading of current block */
  /*-------------------------------*/
	read_nb_loop = 0;
	while(rid != C_ERROR_ALLOC)
	{
		rid = ReadTraces(&config_reader, ptr_this, read_nb_loop);
		if(rid == C_ERROR_OK)
		{
			read_nb_loop++;
		}
		else
		{
			if((rid != C_ERROR_BAD_PARAM) && (rid != C_ERROR_ALLOC))
			{
				JUMP_ERROR(FIN2, rid, rid);
			}
		}
	}
	rid = C_ERROR_OK;

	ptr_this->_nbTrace = read_nb_loop;

 FIN2:
	FILE_READER_CloseFile(&config_reader);

 FIN1:
	return rid;
}


/* Get Traces definition complete file name */
T_ERROR TRACES_DEF_ReadConfigFile(
												/* INOUT */ T_TRACES_DEF * ptr_this,
												/* IN    */ T_UINT16 SimReference,
												/* IN    */ T_UINT16 SimRun)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH file_name;

	/* Get the exec path */
    // FIXME: does not work anymore, use FILE_PATH_GetConfPath instead ?
	JUMP_ERROR(FIN, rid, FILE_PATH_GetRunPath(file_name, SimReference, SimRun));

	/* Get the complete file name */
	JUMP_ERROR(FIN, rid, FILE_PATH_Concat(file_name,
													  FILE_INFOS_GetFileName
													  (C_TRACE_DEF_FILE)));

	/* Call Read function */
	JUMP_ERROR(FIN, rid, TRACES_DEF_ReadConfigNamedFile(ptr_this, file_name));

 FIN:
	return rid;
}

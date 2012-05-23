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
 * @file ProbesDef.c
 * @author TAS
 * @brief The ProbesDef class implements the reading of statistics definition
 *        configuration file
 */

#include <string.h>

#include "FileReader_e.h"
#include "ProbesDef_e.h"
#include "FilePath_e.h"
#include "FileInfos_e.h"
#include "EnumParser_e.h"
#include <ctype.h>


/*********************/
/* MACRO DEFINITIONS */
/*********************/
#define   C_STAT_CAT_MAX_NB    50


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/

T_ERROR PROBES_DEF_Init(
								  /* INOUT */ T_PROBES_DEF * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_PROBES_DEF));

	/* Initialise Enum structures associating Character strings with Integer values */
  /*------------------------------------------------------------------------------*/
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_GW]._StrValue, "GW");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_SAT]._StrValue, "SAT");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_ST]._StrValue, "ST");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_ST_AGG]._StrValue,
			 "AGGREGATE_ST");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_OBPC]._StrValue, "OBPC");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_TG]._StrValue,
			 "TRAFFIC");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_PROBE_CTRL]._StrValue,
			 "PROBE_CONTROLLER");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_EVENT_CTRL]._StrValue,
			 "EVENT_CONTROLLER");
	strcpy(ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_ERROR_CTRL]._StrValue,
			 "ERROR_CONTROLLER");
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_MAX]._StrValue[0] = '\0';

	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_GW]._IntValue = C_COMP_GW;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_SAT]._IntValue = C_COMP_SAT;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_ST]._IntValue = C_COMP_ST;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_ST_AGG]._IntValue = C_COMP_ST_AGG;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_OBPC]._IntValue = C_COMP_OBPC;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_TG]._IntValue = C_COMP_TG;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_PROBE_CTRL]._IntValue =
		C_COMP_PROBE_CTRL;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_EVENT_CTRL]._IntValue =
		C_COMP_EVENT_CTRL;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_ERROR_CTRL]._IntValue =
		C_COMP_ERROR_CTRL;
	ptr_this->C_PROBES_DEFINITION_ComponentChoices[C_COMP_MAX]._IntValue = 0;

	strcpy(ptr_this->C_PROBE_TYPE_choices[0]._StrValue, "INT");
	strcpy(ptr_this->C_PROBE_TYPE_choices[1]._StrValue, "FLOAT");
	ptr_this->C_PROBE_TYPE_choices[2]._StrValue[0] = '\0';

	ptr_this->C_PROBE_TYPE_choices[0]._IntValue = C_PROBE_TYPE_INT;
	ptr_this->C_PROBE_TYPE_choices[1]._IntValue = C_PROBE_TYPE_FLOAT;
	ptr_this->C_PROBE_TYPE_choices[2]._IntValue = 0;

	/* Nb Statistics initial value is set to C_PROB_MAX_STAT_NUMBER */
  /*--------------------------------------------------------------*/
	ptr_this->_nbStatistics = C_PROB_MAX_STAT_NUMBER;

	return rid;
}


static T_ERROR ReadStatLabel(
										 /* IN    */ T_FILE_READER * ptr_reader,
										 /*   OUT */ T_PROBE_DEF * ptr_probe,
										 /* IN    */ T_INT32 statLabelIndex)
{
	T_ERROR rid = C_ERROR_OK;

	/* Read current statistic label */
  /*------------------------------*/
	if(fscanf
		(ptr_reader->_File, "%s\n",
		 ptr_probe->_StatLabels._StatLabelValue[statLabelIndex]) != 1)
		rid = C_ERROR_FILE_READ;

	return rid;
}


static T_ERROR ReadStat(
								  /* IN    */ T_FILE_READER * ptr_reader,
								  /*   OUT */ T_PROBES_DEF * ptr_probes,
								  /* IN    */ T_INT32 statIndex)
{
	T_ERROR rid = C_ERROR_OK;
	T_PROBE_DEF * ptr_probe = &(ptr_probes->_Statistic[statIndex]);

	/* Initialize line parsing */
  /*-------------------------*/
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	/* Parse read line */
  /*-----------------*/
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_PROB_DEF_MAX_CAR_NAME,
												  ptr_probe->_Name));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseInteger(&(ptr_reader->_Parser), 0, 4,
													&(ptr_probe->_Category)));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
												ptr_probes->C_PROBE_TYPE_choices,
												&(ptr_probe->_Type)));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_PROB_DEF_MAX_CAR_UNIT,
												  ptr_probe->_Unit));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_PROB_DEF_MAX_CAR_GRAPH_TYPE,
												  ptr_probe->_Graph_Type));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_PROB_DEF_MAX_CAR_COMMENT,
												  ptr_probe->_Comment));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser), 0,
													 C_PROB_MAX_LABEL_VALUE,
													 &(ptr_probe->_StatLabels._nbLabels)));

	/* Read statistic label loop */
  /*---------------------------*/

	/* If nbLabels is 0 or 1, no label is described in the file (so there is no loop to read) */
  /*----------------------------------------------------------------------------------------*/
	if(ptr_probe->_StatLabels._nbLabels != 0
		&& ptr_probe->_StatLabels._nbLabels != 1)
	{
		JUMP_ERROR(FIN, rid, FILE_READER_ReadLoop(ptr_reader,
																(T_READ_ITEM_FUNC)
																ReadStatLabel,
																ptr_probe->_StatLabels.
																_nbLabels,
																(T_ITEM_TAB *) ptr_probe));
	}
	else
	{
		strcpy(ptr_probe->_StatLabels._StatLabelValue[0], "\0");
	}

	/* Probe Id corresponds to the probe position in the file */
  /*--------------------------------------------------------*/
	ptr_probe->_probeId = statIndex + 1;

 FIN:
	return rid;
}


/* Read Probes Definition file */
/*-----------------------------*/
T_ERROR PROBES_DEF_ReadConfigNamedFile(
													  /* INOUT */ T_PROBES_DEF * ptr_this,
													  /* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_READER config_reader;

	/* Initialise config_reader */
  /*--------------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_Init(&config_reader));

	/* Open config file */
  /*------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_OpenFile(&config_reader, name));

	/* Read Statistic definition loop */
  /*--------------------------------*/
	JUMP_ERROR(FIN2, rid, FILE_READER_ReadNamedLoop(&config_reader,
																	"Statistic_number",
																	(T_READ_ITEM_FUNC) ReadStat,
																	C_PROB_MAX_STAT_NUMBER,
																	(T_ITEM_TAB *) ptr_this));

 FIN2:
	FILE_READER_CloseFile(&config_reader);

 FIN1:
	return rid;
}


/* Get Probes Definition complete file name */
/*------------------------------------------*/
T_ERROR PROBES_DEF_ReadConfigFile(
												/* INOUT */ T_PROBES_DEF * ptr_this,
												/* IN    */ T_COMPONENT_TYPE ComponentLabel)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH file_name;
	T_FILE_PATH temp_file_name1, temp_file_name2, componentName, componentNameLow;
	T_UINT8 i;


	/* Initialize T_PROBES_DEF structure */
  /*-----------------------------------*/
	JUMP_ERROR(FIN, rid, PROBES_DEF_Init(ptr_this));

	/* Get the configuration path */
  /*-------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_GetConfPath(file_name));

	/* Get the complete file name */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid, ENUM_PARSER_ParseString((T_INT32) ComponentLabel,
																ptr_this->
																C_PROBES_DEFINITION_ComponentChoices,
																componentName));

	strcpy(temp_file_name1, FILE_INFOS_GetFileName(C_PROBE_DEF_FILE));

	i = 0;
	while(i < strlen(componentName))
	{
		componentNameLow[i] = tolower(componentName[i]);
		i++;
	}
	componentNameLow[i] = '\0';
	sprintf(temp_file_name2, temp_file_name1, componentNameLow);

	JUMP_ERROR(FIN, rid, FILE_PATH_Concat(file_name, temp_file_name2));


	/* Call Read function */
  /*--------------------*/
	JUMP_ERROR(FIN, rid, PROBES_DEF_ReadConfigNamedFile(ptr_this, file_name));

 FIN:

	return rid;
}

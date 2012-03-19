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
 * @file ComParameters.c
 * @author TAS
 * @brief The ComParameters class implements the reading of communication
 *        parameters configuration file
 */

/********************/
/* SYSTEM RESOURCES */
/********************/
#include <string.h>
#include <sys/socket.h>


/*********************/
/* PROJECT RESOURCES */
/*********************/
#include "FileReader_e.h"
#include "ComParameters_e.h"
#include "FilePath_e.h"
#include "FileInfos_e.h"
#include "unused.h"


/*********************/
/* MACRO DEFINITIONS */
/*********************/

T_ERROR COM_PARAMETERS_Init(
										/* INOUT */ T_COM_PARAMETERS * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_COM_PARAMETERS));

	strcpy(ptr_this->C_PORT_FAMILY_choices[0]._StrValue, "INET");
	strcpy(ptr_this->C_PORT_FAMILY_choices[1]._StrValue, "UNIX");
	ptr_this->C_PORT_FAMILY_choices[2]._StrValue[0] = '\0';

	ptr_this->C_PORT_FAMILY_choices[0]._IntValue = AF_INET;
	ptr_this->C_PORT_FAMILY_choices[1]._IntValue = AF_UNIX;
	ptr_this->C_PORT_FAMILY_choices[2]._IntValue = 0;

	return rid;
}


void COM_PARAMETERS_Terminate(
										  /* INOUT */ T_COM_PARAMETERS * ptr_this)
{
	/* Terminate allocated Controllers ports IP_ADDR structures */
  /*----------------------------------------------------------*/
	IP_ADDR_Terminate(&
							(ptr_this->_ControllersPorts._ErrorController._IpAddress));
	IP_ADDR_Terminate(&
							(ptr_this->_ControllersPorts._EventController._IpAddress));
	IP_ADDR_Terminate(&
							(ptr_this->_ControllersPorts._ProbeController._IpAddress));

	/* Terminate allocated DISPLAY ports IP_ADDR structures */
  /*------------------------------------------------------*/
	IP_ADDR_Terminate(&(ptr_this->_DisplayPorts._EventDisplay._IpAddress));
	IP_ADDR_Terminate(&(ptr_this->_DisplayPorts._ErrorDisplay._IpAddress));
	IP_ADDR_Terminate(&(ptr_this->_DisplayPorts._ProbeDisplay._IpAddress));

}


/* Read Controllers ports function */
/*---------------------------------*/
static T_ERROR COM_PARAMETERS_ReadControllerPorts(
																	 /* IN */ T_FILE_READER *
																	 ptr_reader,
																	 /* INOUT */
																	 T_COM_PARAMETERS * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE], readHost[C_FR_MAX_LINE];
	T_UINT32 readPort = 0;


	/* Read First Line containing "Controllers_ports" - which is not used */
  /*--------------------------------------------------------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_ReadLine(ptr_reader, readString));

	/* Open controller ports definition block */
  /*----------------------------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_OpenBlock(ptr_reader));

	/* Read 1st controller port number : Scheduling */
  /*----------------------------------------------*/

	/* Initialise line parsing */
  /*-------------------------*/
	LINE_PARSER_Init(&(ptr_reader->_Parser));


	/* Read 1st controller port number : Error */
  /*-----------------------------------------*/

	/* Initialise line parsing */
  /*-------------------------*/
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readString));
	if(strcmp(readString, "Error") != 0)
	{
		rid = C_ERROR_FILE_READ;
		return rid;
	}

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
															 ptr_this->C_PORT_FAMILY_choices,
															 (T_INT32 *) ((unsigned char *) &ptr_this->
																				 _ControllersPorts.
																				 _ErrorController.
																				 _Family)));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readHost));

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser),
																  0, C_UINT32_MAX_VALUE,
																  &readPort));

	/* Create IPAddr strucure */
  /*------------------------*/
	JUMP_ERROR(FIN, rid,
				  IP_ADDR_Init(&
									(ptr_this->_ControllersPorts._ErrorController.
									 _IpAddress), readHost,
									(T_UINT16) (readPort & 0xFFFF),
									ptr_this->_ControllersPorts._ErrorController.
									_Family));

	/* Read 2nd controller port number : Event */
  /*-----------------------------------------*/
	/* initialise line parsing */
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readString));
	if(strcmp(readString, "Event") != 0)
	{
		rid = C_ERROR_FILE_READ;
		return rid;
	}

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
															 ptr_this->C_PORT_FAMILY_choices,
															 (T_INT32 *) ((unsigned char *) &ptr_this->
																				 _ControllersPorts.
																				 _EventController.
																				 _Family)));


	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readHost));

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser),
																  0, C_UINT32_MAX_VALUE,
																  &readPort));

	/* Create IPAddr strucure */
  /*------------------------*/
	JUMP_ERROR(FIN, rid,
				  IP_ADDR_Init(&
									(ptr_this->_ControllersPorts._EventController.
									 _IpAddress), readHost,
									(T_UINT16) (readPort & 0xFFFF),
									ptr_this->_ControllersPorts._EventController.
									_Family));

	/* Read 3rd controller port number : Probe */
  /*-----------------------------------------*/
	/* initialise line parsing */
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readString));
	if(strcmp(readString, "Probe") != 0)
	{
		rid = C_ERROR_FILE_READ;
		return rid;
	}

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
															 ptr_this->C_PORT_FAMILY_choices,
															 (T_INT32 *) ((unsigned char *) &ptr_this->
																				 _ControllersPorts.
																				 _ProbeController.
																				 _Family)));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readHost));

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser),
																  0, C_UINT32_MAX_VALUE,
																  &readPort));

	/* Create IPAddr strucure */
  /*------------------------*/
	JUMP_ERROR(FIN, rid,
				  IP_ADDR_Init(&
									(ptr_this->_ControllersPorts._ProbeController.
									 _IpAddress), readHost,
									(T_UINT16) (readPort & 0xFFFF),
									ptr_this->_ControllersPorts._ProbeController.
									_Family));


	/* Close controller ports definition block */
  /*-----------------------------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_CloseBlock(ptr_reader));

 FIN:
	return rid;
}



/* Read DISPLAY ports number */
/*---------------------------*/
static T_ERROR COM_PARAMETERS_ReadDisplayPorts(
																 /* IN */ T_FILE_READER *
																 ptr_reader,
																 /* INOUT */
																 T_COM_PARAMETERS * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE], readHost[C_FR_MAX_LINE];
	T_UINT32 readPort = 0;

	/* Read First Line containing "DISPLAY_ports" - which is not used */
  /*----------------------------------------------------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_ReadLine(ptr_reader, readString));

	/* Open DL ports definition block */
  /*--------------------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_OpenBlock(ptr_reader));


	/* Read 1st DISPLAY port numbers : Error */
  /*---------------------------------------*/

	/* Initialise line parsing */
  /*-------------------------*/
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readString));
	if(strcmp(readString, "Error") != 0)
	{
		rid = C_ERROR_FILE_READ;
		return rid;
	}

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
															 ptr_this->C_PORT_FAMILY_choices,
															 (T_INT32 *) ((unsigned char *) &ptr_this->
																				 _DisplayPorts.
																				 _ErrorDisplay.
																				 _Family)));


	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readHost));

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser),
																  0, C_UINT32_MAX_VALUE,
																  &readPort));

	/* Create IPAddr strucure */
  /*------------------------*/
	JUMP_ERROR(FIN, rid,
				  IP_ADDR_Init(&(ptr_this->_DisplayPorts._ErrorDisplay._IpAddress),
									readHost, (T_UINT16) (readPort & 0xFFFF),
									ptr_this->_DisplayPorts._ErrorDisplay._Family));


	/* Read 2nd DISPLAY port numbers : Event */
  /*---------------------------------------*/

	/* Initialise line parsing */
  /*-------------------------*/
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readString));

	if(strcmp(readString, "Event") != 0)
	{
		rid = C_ERROR_FILE_READ;
		return rid;
	}

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
															 ptr_this->C_PORT_FAMILY_choices,
															 (T_INT32 *) ((unsigned char *) &ptr_this->
																				 _DisplayPorts.
																				 _EventDisplay.
																				 _Family)));


	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readHost));

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser),
																  0, C_UINT32_MAX_VALUE,
																  &readPort));

	/* Create IPAddr strucure */
  /*------------------------*/
	JUMP_ERROR(FIN, rid,
				  IP_ADDR_Init(&(ptr_this->_DisplayPorts._EventDisplay._IpAddress),
									readHost, (T_UINT16) (readPort & 0xFFFF),
									ptr_this->_DisplayPorts._EventDisplay._Family));


	/* read 3rd DISPLAY port numbers : Probe */
  /*---------------------------------------*/
	/* initialise line parsing */
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readString));
	if(strcmp(readString, "Probe") != 0)
	{
		return C_ERROR_FILE_READ;
	}


	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
															 ptr_this->C_PORT_FAMILY_choices,
															 (T_INT32 *) ((unsigned char *) &ptr_this->
																				 _DisplayPorts.
																				 _ProbeDisplay.
																				 _Family)));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser), C_FR_MAX_LINE,
												  readHost));

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser),
																  0, C_UINT32_MAX_VALUE,
																  &readPort));

	/* Create IPAddr strucure */
  /*------------------------*/
	JUMP_ERROR(FIN, rid,
				  IP_ADDR_Init(&(ptr_this->_DisplayPorts._ProbeDisplay._IpAddress),
									readHost, (T_UINT16) (readPort & 0xFFFF),
									ptr_this->_DisplayPorts._ProbeDisplay._Family));


	/* Close controller ports definition block */
  /*-----------------------------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_CloseBlock(ptr_reader));

 FIN:
	return rid;
}



/* Read Communication parameters file */
/*------------------------------------*/
T_ERROR COM_PARAMETERS_ReadConfigNamedFile(
															/* INOUT */ T_COM_PARAMETERS *
															ptr_this,
															/* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_READER config_reader;

	/* Initialise config_reader */
  /*--------------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_Init(&config_reader));

	/* Initialise T_COM_PARAMETERS structure */
  /*---------------------------------------*/

	JUMP_ERROR(FIN1, rid, COM_PARAMETERS_Init(ptr_this));

	/* Try to open the file */
  /*----------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_OpenFile(&config_reader, name));

	/* Read Controllers ports */
  /*------------------------*/
	JUMP_ERROR(FIN2, rid,
				  COM_PARAMETERS_ReadControllerPorts(&config_reader, ptr_this));

	/* Read DISPLAY ports */
  /*--------------------*/
	JUMP_ERROR(FIN2, rid,
				  COM_PARAMETERS_ReadDisplayPorts(&config_reader, ptr_this));

 FIN2:
	FILE_READER_CloseFile(&config_reader);

 FIN1:
	return rid;
}


/* Get Communication Parameters complete file name */
/*-------------------------------------------------*/
T_ERROR COM_PARAMETERS_ReadConfigFile(
													 /* INOUT */ T_COM_PARAMETERS * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH file_name;
	/* Get the configuration path */
  /*------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_GetConfPath(file_name));

	/* Get the complete file name */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_PATH_Concat(file_name,
										 FILE_INFOS_GetFileName(C_COM_PARAMETERS_FILE)));


	/* Call Read function */
  /*--------------------*/
	JUMP_ERROR(FIN, rid,
				  COM_PARAMETERS_ReadConfigNamedFile(ptr_this, file_name));


	ptr_this->_ControllersPorts._ErrorController._IpAddress._family =
		ptr_this->_ControllersPorts._ErrorController._Family;
	ptr_this->_ControllersPorts._EventController._IpAddress._family =
		ptr_this->_ControllersPorts._ErrorController._Family;
	ptr_this->_ControllersPorts._ProbeController._IpAddress._family =
		ptr_this->_ControllersPorts._ErrorController._Family;

	ptr_this->_DisplayPorts._EventDisplay._IpAddress._family =
		ptr_this->_DisplayPorts._EventDisplay._Family;
	ptr_this->_DisplayPorts._ErrorDisplay._IpAddress._family =
		ptr_this->_DisplayPorts._ErrorDisplay._Family;
	ptr_this->_DisplayPorts._ProbeDisplay._IpAddress._family =
		ptr_this->_DisplayPorts._ProbeDisplay._Family;


 FIN:
	return rid;
}


T_ERROR COM_PARAMETERS_PrintConfigFile(
													  /* INOUT */ T_COM_PARAMETERS *
													  UNUSED(ptr_this))
{
	T_ERROR rid = C_ERROR_OK;


	return rid;
}

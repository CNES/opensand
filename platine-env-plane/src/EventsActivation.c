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
 * @file EventsActivation.c
 * @author TAS
 * @brief The EventsActivation class implements the reading of events
 *        activation configuration file
 */

#include "FileReader_e.h"
#include "EventsActivation_e.h"
#include "FilePath_e.h"
#include "FileInfos_e.h"
#include "unused.h"

#include <string.h>

/********************/
/* MACRO DEFINITION */
/********************/


T_ERROR EVENTS_ACTIVATION_Init(
											/* INOUT */ T_EVENTS_ACTIVATION * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_EVENTS_ACTIVATION));

	return rid;
}


static T_ERROR ReadEventActivation(
												 /* IN    */ T_FILE_READER * ptr_reader,
												 /*   OUT */
												 T_EVENTS_ACTIVATION * ptr_events,
												 /* IN    */ T_INT32 eventActivationIndex)
{
	T_ERROR rid = C_ERROR_OK;

	/* Read current Event category */
  /*-----------------------------*/
	if(fscanf
		(ptr_reader->_File, "%ld\n",
		 &(ptr_events->_EventCategory[eventActivationIndex])) != 1)
		rid = C_ERROR_FILE_READ;

	else if(ptr_events->_EventCategory[eventActivationIndex] > 4
			  || ptr_events->_EventCategory[eventActivationIndex] < 0)
		rid = C_ERROR_CONF_INVAL;

	return rid;
}


T_ERROR EVENTS_ACTIVATION_ReadConfigNamedFile(
																/* INOUT */ T_EVENTS_ACTIVATION
																* ptr_this,
																/* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_READER config_reader;

	/* Initialise config_reader */
  /*--------------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_Init(&config_reader));

	/* Initialise T_COM_PARAMETERS structure */
  /*---------------------------------------*/
	JUMP_ERROR(FIN1, rid, EVENTS_ACTIVATION_Init(ptr_this));

	/* Open config file */
  /*------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_OpenFile(&config_reader, name));

	/* Read event activation loop */
  /*----------------------------*/
	JUMP_ERROR(FIN2, rid, FILE_READER_ReadNamedLoop(&config_reader,
																	"Event_category_number",
																	(T_READ_ITEM_FUNC)
																	ReadEventActivation,
																	C_EVT_CATEGORY_MAX_NB,
																	(T_ITEM_TAB *) ptr_this));

 FIN2:
	FILE_READER_CloseFile(&config_reader);

 FIN1:
	return rid;
}


T_ERROR EVENTS_ACTIVATION_ReadConfigFile(
														 /* INOUT */ T_EVENTS_ACTIVATION *
														 ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH file_name;


	/* Get the conf path */
  /*------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_GetConfPath(file_name));

	/* Get the complete file name */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_Concat(file_name,
													  FILE_INFOS_GetFileName
													  (C_EVENT_ACT_FILE)));


	/* Call Read function */
  /*--------------------*/
	JUMP_ERROR(FIN, rid,
				  EVENTS_ACTIVATION_ReadConfigNamedFile(ptr_this, file_name));

 FIN:
	return rid;
}


T_ERROR EVENTS_ACTIVATION_PrintConfigFile(
														  /* INOUT */ T_EVENTS_ACTIVATION *
														  UNUSED(ptr_this))
{
	T_ERROR rid = C_ERROR_OK;


	return rid;
}

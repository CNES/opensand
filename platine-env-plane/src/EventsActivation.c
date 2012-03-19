/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The EventsActivation class implements the reading of 
               events activation configuration file 
    @HISTORY :
    03-10-17 : Add XML data (GM)
*/
/*--------------------------------------------------------------------------*/

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

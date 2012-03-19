/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The EventsDefinition class implements the reading of 
               events definition configuration file 
    @HISTORY :
    03-02-26 : Creation
    03-10-13 : Add XML data
*/
/*--------------------------------------------------------------------------*/

#include "FileReader_e.h"
#include "EventsDef_e.h"
#include "FilePath_e.h"
#include "FileInfos_e.h"

#include <string.h>

/*********************/
/* MACRO DEFINITIONS */
/*********************/

#define C_CATEGORY_MAX_NB          32



/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/


T_ERROR EVENTS_DEF_Init(
								  /* INOUT */ T_EVENTS_DEF * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_EVENTS_DEF));

	return rid;
}


static T_ERROR ReadEvent(
									/* IN    */ T_FILE_READER * ptr_reader,
									/*   OUT */ T_EVENTS_DEF * ptr_events,
									/* IN    */ T_INT32 eventIndex)
{
	T_ERROR rid = C_ERROR_OK;
	T_EVENT_DEF * ptr_event = &(ptr_events->_Event[eventIndex]);

	/* initialise line parsing */
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
												  C_EVT_DEF_MAX_CAR_NAME,
												  ptr_event->_Name));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseInteger(&(ptr_reader->_Parser), 0, 4,
													&(ptr_event->_Category)));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_EVT_DEF_MAX_CAR_IDX_SIGN,
												  ptr_event->_IndexSignification));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_EVT_DEF_MAX_CAR_VAL_SIGN,
												  ptr_event->_ValueSignification));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_EVT_DEF_MAX_CAR_UNIT,
												  ptr_event->_Unit));

	/* Event Id is the rank of the event in the file */
  /*-----------------------------------------------*/
	ptr_event->_EventId = eventIndex + 1;

 FIN:
	return rid;
}


T_ERROR EVENTS_DEF_ReadConfigNamedFile(
													  /* INOUT */ T_EVENTS_DEF * ptr_this,
													  /* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_READER config_reader;

	/* Initialise config_reader */
  /*--------------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_Init(&config_reader));

	/* Initialise T_EVENTS_DEF structure */
  /*-----------------------------------*/
	JUMP_ERROR(FIN1, rid, EVENTS_DEF_Init(ptr_this));

	/* Begin file reading */
  /*--------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_OpenFile(&config_reader, name));

	JUMP_ERROR(FIN2, rid, FILE_READER_ReadNamedLoop(&config_reader,
																	"Event_number",
																	(T_READ_ITEM_FUNC) ReadEvent,
																	C_EVT_DEF_MAX_EVENTS,
																	(T_ITEM_TAB *) ptr_this));

 FIN2:
	FILE_READER_CloseFile(&config_reader);

 FIN1:
	return rid;
}

/* Get Events definition complete file name */
T_ERROR EVENTS_DEF_ReadConfigFile(
												/* INOUT */ T_EVENTS_DEF * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH file_name;

	/* Get the configuration path */
  /*-------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_GetConfPath(file_name));

	/* Get the complete file name */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_Concat(file_name,
													  FILE_INFOS_GetFileName
													  (C_EVENT_DEF_FILE)));


	/* Call Read function */
  /*--------------------*/
	JUMP_ERROR(FIN, rid, EVENTS_DEF_ReadConfigNamedFile(ptr_this, file_name));

 FIN:

	return rid;
}



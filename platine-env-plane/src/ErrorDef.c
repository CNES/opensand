/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ErrorDef class implements the error definition 
               configuration file reading
    @HISTORY :
    03-02-24 : Creation
    03-10-20 : Add XML data (GM)
*/
/*--------------------------------------------------------------------------*/

#include "FileReader_e.h"
#include "ErrorDef_e.h"
#include "FilePath_e.h"
#include "FileInfos_e.h"

#include <string.h>

/*********************/
/* MACRO DEFINITIONS */
/*********************/



/**************************/
/* STRUCTURES DEFINITIONS */
/**************************/



T_ERROR ERRORS_DEF_Init(
								  /* INOUT */ T_ERRORS_DEF * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_ERRORS_DEF));

	return rid;
}

static T_ERROR ReadOneIndex(
										/* IN    */ T_FILE_READER * ptr_reader,
										/*   OUT */ T_INDEX_TAB * ptr_index,
										/* IN    */ T_INT32 currentIndexValueIndex)
{
	T_ERROR rid = C_ERROR_OK;

	/* Read current index value for current type */
  /*-------------------------------------------*/
	/* Read expected string in config file */
	if(fscanf
		(ptr_reader->_File, "%s\n",
		 ptr_index->_IndexValues[currentIndexValueIndex]) != 1)
		rid = C_ERROR_FILE_READ;

	return rid;
}


static T_ERROR ReadError(
									/* IN    */ T_FILE_READER * ptr_reader,
									/*   OUT */ T_ERRORS_DEF * ptr_errors,
									/* IN    */ T_INT32 errorIndex)
{
	T_ERROR rid = C_ERROR_OK;
	T_ERROR_DEF * ptr_error = &(ptr_errors->_Error[errorIndex]);
	T_UINT32 NumberOfIndex = 0;

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
												  C_ERR_DEF_MAX_CAR_NAME,
												  ptr_error->_Name));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseInteger(&(ptr_reader->_Parser), 0, C_ERROR_MINOR,
													&(ptr_error->_Category)));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_ERR_DEF_MAX_CAR_IDX_SIGN,
												  ptr_error->_IndexSignification));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_ERR_DEF_MAX_CAR_VAL_SIGN,
												  ptr_error->_ValueSignification));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_ERR_DEF_MAX_CAR_UNIT,
												  ptr_error->_Unit));
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseUInteger(&(ptr_reader->_Parser), 0,
													 C_INDEX_DEF_MAX_NB - 1,
													 &(NumberOfIndex)));

	/* Error Id is the rank of the event in the file */
  /*-----------------------------------------------*/
	ptr_error->_ErrorId = errorIndex + 1;

	/* Read loop of all current index type applicable values */
  /*-------------------------------------------------------*/
	if(NumberOfIndex != 0)
	{
		JUMP_ERROR(FIN, rid, FILE_READER_ReadLoop(ptr_reader,
																(T_READ_ITEM_FUNC) ReadOneIndex,
																NumberOfIndex,
																(T_ITEM_TAB *) & (ptr_error->
																						_IndexTab)));
	}
	else
	{
		strcpy(ptr_error->_IndexTab._IndexValues[0], "\0");
	}

 FIN:
	return rid;
}


T_ERROR ERROR_DEF_ReadConfigNamedFile(
													 /* INOUT */ T_ERRORS_DEF * ptr_this,
													 /* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_READER config_reader;

	/* Initialise config_reader */
  /*--------------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_Init(&config_reader));

	/* Initialise T_ERRORS_DEF structure */
  /*-----------------------------------*/
	JUMP_ERROR(FIN1, rid, ERRORS_DEF_Init(ptr_this));

	/* Begin file reading */
  /*--------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_OpenFile(&config_reader, name));

	/* read Errors definition loop */
  /*-----------------------------*/
	JUMP_ERROR(FIN2, rid, FILE_READER_ReadNamedLoop(&config_reader,
																	"Error_number",
																	(T_READ_ITEM_FUNC) ReadError,
																	C_ERR_DEF_MAX_ERRORS,
																	(T_ITEM_TAB *) ptr_this));

 FIN2:
	FILE_READER_CloseFile(&config_reader);

 FIN1:
	return rid;
}


/* Get Events definition complete file name */
T_ERROR ERROR_DEF_ReadConfigFile(
											  /* INOUT */ T_ERRORS_DEF * ptr_this)
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
													  (C_ERROR_DEF_FILE)));

	/* Call Read function */
  /*--------------------*/
	JUMP_ERROR(FIN, rid, ERROR_DEF_ReadConfigNamedFile(ptr_this, file_name));

 FIN:


	return rid;
}


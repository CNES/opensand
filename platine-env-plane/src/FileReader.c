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
 * @file FileReader.c
 * @author TAS
 * @brief The FileReader class implements methods to read formatted data in
 *        configuration files
 */

#include "FileReader_e.h"
#include "Trace_e.h"
#include <string.h>

T_ERROR FILE_READER_Init(
									/* INOUT */ T_FILE_READER * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_FILE_READER));

	ptr_this->_File = NULL;
	LINE_PARSER_Init(&(ptr_this->_Parser));
	ptr_this->_Buffer[0] = '\0';

	return rid;
}

T_ERROR FILE_READER_OpenFile(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR lineRead[C_FR_MAX_LINE] =
	{
	'\0'};

	/* Open File */
  /*-----------*/
	ptr_this->_File = fopen(name, "r");
	if(ptr_this->_File == NULL)
		rid = C_ERROR_FILE_OPEN;

	/* Check end of file header : end of header is a line wih # character */
  /*--------------------------------------------------------------------*/
	else
	{
		while(strncmp("#", lineRead, 1) != 0)
		{
			JUMP_ERROR(FIN, rid, FILE_READER_ReadLine(ptr_this, lineRead));
		}
	}

 FIN:
	if(rid != C_ERROR_OK)
	{
		TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_CONFIG, C_TRACE_ERROR,
						 "Error while opening file %s", name));
	}
	return rid;
}


T_ERROR FILE_READER_CloseFile(
										  /* IN    */ T_FILE_READER * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	if(ptr_this->_File != NULL)
		fclose(ptr_this->_File);

	return rid;
}


T_ERROR FILE_READER_OpenBlock(
										  /* IN    */ T_FILE_READER * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE] =
	{
	'\0'};

	/* Check for start of block marker " {\n" */
  /*----------------------------------------*/
	if(fscanf(ptr_this->_File, "%s\n", readString) != 1)
		rid = C_ERROR_FILE_READ;
	else if(strcmp(readString, "{") != 0)
		rid = C_ERROR_FILE_READ;

	return rid;
}


T_ERROR FILE_READER_CloseBlock(
											/* IN    */ T_FILE_READER * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE] =
	{
	'\0'};

	/* Check for end of block marker " }\n" */
  /*--------------------------------------*/
	if(fscanf(ptr_this->_File, "%s\n", readString) != 1)
		rid = C_ERROR_FILE_READ;
	else if(strcmp(readString, "}") != 0)
		rid = C_ERROR_FILE_READ;

	return rid;
}


T_ERROR FILE_READER_ReadLine(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /*   OUT */ T_STRING ptr_value)
{
	T_ERROR rid = C_ERROR_OK;

	/* Check the reading of current line */
  /*-----------------------------------*/
	if((fgets((char *) ptr_this->_Buffer, C_FR_MAX_LINE, ptr_this->_File)) ==
		NULL)
		rid = C_ERROR_FILE_READ;

	/* Check if line is too long! */
  /*----------------------------*/
	else if(strlen((char *) ptr_this->_Buffer) > (C_FR_MAX_LINE - 1))
		rid = C_ERROR_CONF_INVAL;
	else
	{
		ptr_this->_Buffer[strlen((char *) ptr_this->_Buffer) - 1] = '\0';
		if(ptr_value != NULL)
			strcpy(ptr_value, (char *) ptr_this->_Buffer);
	}

	return rid;
}


T_ERROR FILE_READER_ReadName(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE] = " ";

	/* Check <name> is being read in the file */
  /*----------------------------------------*/
	if(fscanf(ptr_this->_File, "%s ", readString) == 0)
		rid = C_ERROR_FILE_READ;
	else if(strcasecmp(readString, name) != 0)
		rid = C_ERROR_FILE_READ;

	return rid;
}


T_ERROR FILE_READER_ReadNamedString(
												  /* IN    */ T_FILE_READER * ptr_this,
												  /* IN    */ T_STRING name,
												  /* IN    */ T_UINT32 max_len,
												  /*   OUT */ T_STRING ptr_value)
{
	T_ERROR rid = C_ERROR_OK;

	JUMP_ERROR(FIN, rid, FILE_READER_ReadName(ptr_this, name));

	/* Read expected string in config file */
  /*-------------------------------------*/
	if(fscanf(ptr_this->_File, " : %s\n", ptr_value) != 1)
		rid = C_ERROR_FILE_READ;

	/* Check read string length */
  /*--------------------------*/
	else if(strlen(ptr_value) > max_len)
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadNamedInteger(
													/* IN    */ T_FILE_READER * ptr_this,
													/* IN    */ T_STRING name,
													/* IN    */ T_INT32 min_value,
													/* IN    */ T_INT32 max_value,
													/*   OUT */ T_INT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;

	JUMP_ERROR(FIN, rid, FILE_READER_ReadName(ptr_this, name));
	if(fscanf(ptr_this->_File, " : %lu\n", ptr_value) != 1)
		rid = C_ERROR_FILE_READ;

	/* Check read value is within [min_value, max_value] boundaries */
  /*--------------------------------------------------------------*/
	else if((*ptr_value > max_value) || (*ptr_value < min_value))
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadNamedIntegerDefault(
															 /* IN    */ T_FILE_READER *
															 ptr_this,
															 /* IN    */ T_STRING name,
															 /* IN    */ T_INT32 min_value,
															 /* IN    */ T_INT32 max_value,
															 /* IN    */ T_INT32 default_value,
															 /*   OUT */ T_INT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;

	JUMP_ERROR(FIN, rid, FILE_READER_ReadName(ptr_this, name));

	/* If no value is read in config file, then default value is affected */
  /*--------------------------------------------------------------------*/
	if(fscanf(ptr_this->_File, " : %ld\n", ptr_value) == 0)
		*ptr_value = default_value;

	/* Check read value is within [min_value, max_value] boundaries */
  /*--------------------------------------------------------------*/
	if(*ptr_value > max_value || *ptr_value < min_value)
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadNamedUInteger(
													 /* IN    */ T_FILE_READER * ptr_this,
													 /* IN    */ T_STRING name,
													 /* IN    */ T_UINT32 min_value,
													 /* IN    */ T_UINT32 max_value,
													 /*   OUT */ T_UINT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_INT64 MyReadValue;

	JUMP_ERROR(FIN, rid, FILE_READER_ReadName(ptr_this, name));

	if(fscanf(ptr_this->_File, " : %lld\n", &MyReadValue) != 1)
		rid = C_ERROR_FILE_READ;

	/* Check read value is within [min_value, max_value] boundaries */
  /*--------------------------------------------------------------*/
	else if((MyReadValue > max_value) || (MyReadValue < min_value))
		rid = C_ERROR_CONF_INVAL;

	*ptr_value = (T_UINT32) MyReadValue;

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadNamedUIntegerDefault(
															  /* IN    */ T_FILE_READER *
															  ptr_this,
															  /* IN    */ T_STRING name,
															  /* IN    */ T_UINT32 min_value,
															  /* IN    */ T_UINT32 max_value,
															  /* IN    */
															  T_UINT32 default_value,
															  /*   OUT */ T_UINT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_INT16 i = 0;
	T_INT64 MyReadValue;

	JUMP_ERROR(FIN, rid, FILE_READER_ReadName(ptr_this, name));

	i = fscanf(ptr_this->_File, " : %lld\n", &MyReadValue);

	/* If no value is read in config file, then default value is affected */
  /*--------------------------------------------------------------------*/
	if(i == 0)
		*ptr_value = default_value;
	else
	{
		*ptr_value = (T_UINT32) MyReadValue;

		/* If several values are read, return error */
		/*------------------------------------------*/
		if(i != 1)
			rid = C_ERROR_FILE_READ;

		/* Check read value is within [min_value, max_value] boundaries */
		/*--------------------------------------------------------------*/
		else if((MyReadValue > max_value) || (MyReadValue < min_value))
			rid = C_ERROR_CONF_INVAL;
	}

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadNamedEnum(
												/* IN    */ T_FILE_READER * ptr_this,
												/* IN    */ T_STRING name,
												/* IN    */ T_ENUM_COUPLE choices[],
												/*   OUT */ T_INT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR string_value[C_FR_MAX_LINE] = "";

	/* Read string in config file corresponding to <name> value */
  /*----------------------------------------------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadNamedString(ptr_this, name, C_FR_MAX_LINE,
														string_value));

	/* Extract int value corresponding to the string read in confg file */
  /*------------------------------------------------------------------*/
	JUMP_ERROR(FIN, rid, ENUM_PARSER_Parse(string_value, choices, ptr_value));

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadNamedFloat(
												 /* IN    */ T_FILE_READER * ptr_this,
												 /* IN    */ T_STRING name,
												 /* IN    */ T_FLOAT min_value,
												 /* IN    */ T_FLOAT max_value,
												 /*   OUT */ T_FLOAT * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;

	JUMP_ERROR(FIN, rid, FILE_READER_ReadName(ptr_this, name));

	if(fscanf(ptr_this->_File, " : %g\n", ptr_value) != 1)
		rid = C_ERROR_FILE_READ;

	/* Check read value is within [min_value, max_value] boundaries */
  /*--------------------------------------------------------------*/
	else if((*ptr_value > max_value) || (*ptr_value < min_value))
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadNamedFloatDefault(
														  /* IN    */ T_FILE_READER *
														  ptr_this,
														  /* IN    */ T_STRING name,
														  /* IN    */ T_FLOAT min_value,
														  /* IN    */ T_FLOAT max_value,
														  /* IN    */ T_FLOAT default_value,
														  /*   OUT */ T_FLOAT * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_INT16 i = 0;

	JUMP_ERROR(FIN, rid, FILE_READER_ReadName(ptr_this, name));

	i = fscanf(ptr_this->_File, " : %g\n", ptr_value);

	/* if no value has been read in config file, then use default_value */
  /*------------------------------------------------------------------*/
	if(i == 0)
		*ptr_value = default_value;
	else if(i != 1)
		rid = C_ERROR_FILE_READ;

	/* Check read value is within [min_value, max_value] boundaries */
  /*--------------------------------------------------------------*/
	else if((*ptr_value > max_value) || (*ptr_value < min_value))
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR FILE_READER_ReadLoop(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /* IN    */ T_READ_ITEM_FUNC read_item_func,
										 /* IN    */ T_UINT32 nb_loop,
										 /*   OUT */ T_ITEM_TAB * ptr_config)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i = 0;

	/* Beginning of current block */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_OpenBlock(ptr_this));

	/* Loop reading of current block */
  /*-------------------------------*/
	for(i = 0; i < nb_loop; i++)
	{
		JUMP_ERROR(FIN, rid, read_item_func(ptr_this, ptr_config, i));
	}

	/* End of current block */
  /*----------------------*/
	JUMP_ERROR(FIN, rid, FILE_READER_CloseBlock(ptr_this));

 FIN:
	ptr_config->_nbItem = i;
	return rid;
}


T_ERROR FILE_READER_ReadNamedLoop(
												/* IN    */ T_FILE_READER * ptr_this,
												/* IN    */ T_STRING loop_name,
												/* IN    */ T_READ_ITEM_FUNC read_item_func,
												/* IN    */ T_UINT32 max_loop,
												/*   OUT */ T_ITEM_TAB * ptr_config)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 read_nb_loop = 0;

	/* read iteration number */
  /*-----------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadNamedUInteger(ptr_this, loop_name,
														  C_UINT32_MIN_VALUE, max_loop,
														  &read_nb_loop));

	/* Read all loop values */
  /*----------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLoop(ptr_this, read_item_func, read_nb_loop,
											  ptr_config));

 FIN:
	return rid;
}

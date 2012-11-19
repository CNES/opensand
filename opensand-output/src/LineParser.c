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
 * @file LineParser.c
 * @author TAS
 * @brief The LineParser class implements methods to read different data on
 *        the same line in configuration files
 */

#include "LineParser_e.h"
#include <string.h>
#include <stdlib.h>

#define C_CONFIG_CHAR_DELIMITER1 ','	/* Delimiters that may be used for parsing */
#define C_CONFIG_CHAR_DELIMITER2 ':'	/* Delimiters that may be used for parsing */
#define C_CONFIG_CHAR_WHITE_SPACE   ' '
#define C_CONFIG_CHAR_TABULATION    '\t'
#define C_CONFIG_CHAR_CR            '\n'
#define C_CONFIG_CHAR_NULL          '\0'
#define C_CONFIG_CHAR_CTRL_M        '\r'



T_ERROR LINE_PARSER_Init(
									/* INOUT */ T_LINE_PARSER * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_LINE_PARSER));

	return rid;
}


T_ERROR LINE_PARSER_Terminate()
{
	T_ERROR rid = C_ERROR_OK;

	return rid;
}


T_ERROR LINE_PARSER_ParseFloat(
											/* IN    */ T_LINE_PARSER * ptr_this,
											/* IN    */ T_FLOAT min_value,
											/* IN    */ T_FLOAT max_value,
											/*   OUT */ T_FLOAT * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE];
	T_UINT32 iLoop;

	for(iLoop = 0; iLoop < C_FR_MAX_LINE; iLoop++)
	{
		readString[iLoop] = '\0';
	}

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(ptr_this, C_FR_MAX_LINE, readString));

	/* Convert read value to T_FLOAT */
	*ptr_value = atof(readString);
	if(*ptr_value == 0 && strcmp(readString, "0") != 0
		&& strcmp(readString, "0.0") != 0)
		rid = C_ERROR_FILE_READ;
	else if(*ptr_value < min_value || *ptr_value > max_value)
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR LINE_PARSER_ParseInteger(
											  /* IN    */ T_LINE_PARSER * ptr_this,
											  /* IN    */ T_INT32 min_value,
											  /* IN    */ T_INT32 max_value,
											  /*   OUT */ T_INT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE], ReturnedValue[C_FR_MAX_LINE];
	T_UINT32 iLoop;

	for(iLoop = 0; iLoop < C_FR_MAX_LINE; iLoop++)
	{
		readString[iLoop] = '\0';
		ReturnedValue[iLoop] = '\0';
	}

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(ptr_this, C_FR_MAX_LINE, readString));

	/* Convert read value to T_INT32 */
	*ptr_value = strtol(readString, (T_CHAR **) (&ReturnedValue), 0);

	if(*ptr_value == 0 && strcmp(readString, "0") != 0)
		rid = C_ERROR_FILE_READ;
	else if(*ptr_value < min_value || *ptr_value > max_value)
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR LINE_PARSER_ParseUInteger(
												/* IN    */ T_LINE_PARSER * ptr_this,
												/* IN    */ T_UINT32 min_value,
												/* IN    */ T_UINT32 max_value,
												/*   OUT */ T_UINT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE], ReturnedValue[C_FR_MAX_LINE];
	T_UINT32 iLoop;

	for(iLoop = 0; iLoop < C_FR_MAX_LINE; iLoop++)
	{
		readString[iLoop] = '\0';
		ReturnedValue[iLoop] = '\0';
	}

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(ptr_this, C_FR_MAX_LINE, readString));

	/* Convert read value to T_UINT32 */
	*ptr_value = strtoul(readString, (T_CHAR **) (&ReturnedValue), 0);
	if(*ptr_value == 0 && strcmp(readString, "0") != 0)
		rid = C_ERROR_FILE_READ;
	else if(*ptr_value < min_value || *ptr_value > max_value)
		rid = C_ERROR_CONF_INVAL;

 FIN:
	return rid;
}


T_ERROR LINE_PARSER_ParseString(
											 /* IN    */ T_LINE_PARSER * ptr_this,
											 /* IN    */ T_INT32 max_len,
											 /*   OUT */ T_STRING ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 index = 0, stringIndex = 0, sizeLine = 0;

	/* Get current position in line being parsed */
  /*-------------------------------------------*/
	index = ptr_this->_LineCurrentPosition;

	/* get current line size */
  /*-----------------------*/
	sizeLine = strlen(ptr_this->_LineBuffer);

	/* start parsing characters until next delimiter or end of line */
  /*--------------------------------------------------------------*/
	while(index < sizeLine)
	{
		if(ptr_this->_LineBuffer[index] == C_CONFIG_CHAR_WHITE_SPACE
			|| ptr_this->_LineBuffer[index] == C_CONFIG_CHAR_TABULATION
			|| ptr_this->_LineBuffer[index] == C_CONFIG_CHAR_CTRL_M)
		{
			/* Remove all White, Tab & CrtlM characters from parsed line */
		/*-----------------------------------------------------------*/
			index++;
		}
		else if(ptr_this->_LineBuffer[index] == C_CONFIG_CHAR_DELIMITER1
				  || ptr_this->_LineBuffer[index] == C_CONFIG_CHAR_DELIMITER2)
		{
			/* DELIMITER CHARACTER FOUND : end of current parsing */
		/*----------------------------------------------------*/
			ptr_value[stringIndex] = C_CONFIG_CHAR_NULL;
			ptr_this->_LineCurrentPosition = index + 1;
			index = C_FR_MAX_LINE;
		}
		else if(ptr_this->_LineBuffer[index] == C_CONFIG_CHAR_CR)
		{
			/* End of line */
		/*-------------*/
			ptr_value[stringIndex] = C_CONFIG_CHAR_NULL;
			ptr_this->_LineCurrentPosition = 0;
			index = C_FR_MAX_LINE;

			ptr_this->_TokenLeft = 0;	/* used when variable number of elements in 1 line */
		}
		else
		{
			/* Copy current parsed character to output string */
		/*------------------------------------------------*/
			ptr_value[stringIndex] = ptr_this->_LineBuffer[index];

			/* Increment current line position and output string current index */
		/*-----------------------------------------------------------------*/
			index++;
			stringIndex++;
		}
	}

	/* Error cases */
  /*-------------*/
	if(stringIndex > (T_UINT32)max_len)
	{
		rid = C_ERROR_CONF_INVAL; /* parsed string is too long! */
		printf
			("ERROR : Parsed String %s is too long (size = %lu, max_len = %lu)\n",
			 ptr_value, stringIndex, max_len);
	}
	else if(stringIndex < 1)
	{
		rid = C_ERROR_FILE_READ;  /* empty string was read! */
		printf("Parsed string is empty\n");
	}

	return rid;
}


T_ERROR LINE_PARSER_ParseEnum(
										  /* IN    */ T_LINE_PARSER * ptr_this,
										  /* IN    */ T_ENUM_COUPLE choices[],
										  /*   OUT */ T_INT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE];
	T_UINT32 iLoop;

	for(iLoop = 0; iLoop < C_FR_MAX_LINE; iLoop++)
	{
		readString[iLoop] = '\0';
	}

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(ptr_this, C_FR_MAX_LINE, readString));

	/* Get integer value from enum read */
	JUMP_ERROR(FIN, rid, ENUM_PARSER_Parse(readString, choices, ptr_value));

 FIN:
	return rid;
}

T_ERROR LINE_PARSER_ParseEnumLong(
												/* IN    */ T_LINE_PARSER * ptr_this,
												/* IN    */ T_ENUM_LONGCOUPLE choices[],
												/*   OUT */ T_INT64 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_CHAR readString[C_FR_MAX_LINE];
	T_UINT32 iLoop;

	for(iLoop = 0; iLoop < C_FR_MAX_LINE; iLoop++)
	{
		readString[iLoop] = '\0';
	}

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(ptr_this, C_FR_MAX_LINE, readString));

	/* Get integer value from enum read */
	JUMP_ERROR(FIN, rid, ENUM_PARSER_ParseLong(readString, choices, ptr_value));

 FIN:
	return rid;
}

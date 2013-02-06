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
 * @file LineParser_e.h
 * @author TAS
 * @brief The LineParser class implements methods to read
 *        different data on the same line in configuration files
 */

#ifndef LineParser_e
#   define LineParser_e

#   include "EnumParser_e.h"

#   define C_FR_MAX_LINE  124	  /* Maximum size of one line in config file */


typedef struct
{

	T_CHAR _LineBuffer[C_FR_MAX_LINE];

	T_UINT32 _LineCurrentPosition;

	T_UINT32 _TokenLeft;

} T_LINE_PARSER;

/******************************/
/* Only static variables used */
/* nit & terminate methods    */
T_ERROR LINE_PARSER_Init(
/* INOUT */ T_LINE_PARSER * ptr_this);


T_ERROR LINE_PARSER_ParseFloat(
											/* IN    */ T_LINE_PARSER * ptr_this,
											/* IN    */ T_FLOAT min_value,
											/* IN    */ T_FLOAT max_value,
											/*   OUT */ T_FLOAT * ptr_value);


T_ERROR LINE_PARSER_ParseInteger(
											  /* IN    */ T_LINE_PARSER * ptr_this,
											  /* IN    */ T_INT32 min_value,
											  /* IN    */ T_INT32 max_value,
											  /*   OUT */ T_INT32 * ptr_value);


T_ERROR LINE_PARSER_ParseUInteger(
												/* IN    */ T_LINE_PARSER * ptr_this,
												/* IN    */ T_UINT32 min_value,
												/* IN    */ T_UINT32 max_value,
												/*   OUT */ T_UINT32 * ptr_value);


T_ERROR LINE_PARSER_ParseString(
											 /* IN    */ T_LINE_PARSER * ptr_this,
											 /* IN    */ T_INT32 max_len,
											 /*   OUT */ T_STRING ptr_value);


T_ERROR LINE_PARSER_ParseEnum(
										  /* IN    */ T_LINE_PARSER * ptr_this,
										  /* IN    */ T_ENUM_COUPLE choices[],
										  /*   OUT */ T_INT32 * ptr_value);

T_ERROR LINE_PARSER_ParseEnumLong(
												/* IN    */ T_LINE_PARSER * ptr_this,
												/* IN    */ T_ENUM_LONGCOUPLE choices[],
												/*   OUT */ T_INT64 * ptr_value);


#endif

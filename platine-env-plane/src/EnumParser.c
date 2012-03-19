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
 * @file EnumParser.c
 * @author TAS
 * @brief this class implements methods handling (String, Int32) couple
 *        arrays corresponding to enum (string + associated INT32 value)
 */

#include "EnumParser_e.h"
#include "Error_e.h"
#include <string.h>


/*  @ROLE    : get index corresponding to a STRING in an enumCouple array
    @RETURN  : Error code */
T_ERROR ENUM_PARSER_Parse(
									 /* IN    */ T_STRING str,
									 /* IN    */ T_ENUM_COUPLE choices[],
									 /*   OUT */ T_INT32 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i = 0;

	/* choices last value shall always be NULL */
  /*-----------------------------------------*/
	while(choices[i]._StrValue[0] != '\0')
	{
		if(strcasecmp(choices[i]._StrValue, str) == 0)
		{
			/* Update T_INT32 corresponding to str STRING */
		/*--------------------------------------------*/
			(*ptr_value) = choices[i]._IntValue;
			return rid;
		}
		else
			i++;
	}

	/* if expected string not found then CONFIG file may be wrong */
  /*------------------------------------------------------------*/
	rid = C_ERROR_CONF_INVAL;

	return rid;
}


/*  @ROLE    : get string corresponding to an index in an enumCouple array
    @RETURN  : Error code */
T_ERROR ENUM_PARSER_ParseString(
											 /* IN    */ T_INT32 integerValue,
											 /* IN    */ T_ENUM_COUPLE choices[],
											 /*   OUT */ T_STRING ptr_string_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i = 0;

	/* choices last value shall always be NULL */
  /*-----------------------------------------*/
	while(choices[i]._StrValue[0] != '\0')
	{
		if(choices[i]._IntValue == integerValue)
		{
			/* Update STRING corresponding to integerValue */
		/*---------------------------------------------*/
			strcpy(ptr_string_value, choices[i]._StrValue);
			return rid;
		}
		else
			i++;
	}

	/* if expected string not found then CONFIG file may be wrong */
  /*------------------------------------------------------------*/
	rid = C_ERROR_CONF_INVAL;

	return rid;
}

/*  @ROLE    : get index corresponding to a STRING in an enumCouple array
    @RETURN  : Error code */
T_ERROR ENUM_PARSER_ParseLong(
										  /* IN    */ T_STRING str,
										  /* IN    */ T_ENUM_LONGCOUPLE choices[],
										  /*   OUT */ T_INT64 * ptr_value)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i = 0;

	/* choices last value shall always be NULL */
  /*-----------------------------------------*/
	while(choices[i]._StrValue[0] != '\0')
	{
		if(strcasecmp(choices[i]._StrValue, str) == 0)
		{
			/* Update T_INT64 corresponding to str STRING */
		/*--------------------------------------------*/
			*ptr_value = (T_INT64) (choices[i]._IntValue);
			return rid;
		}
		else
			i++;
	}

	/* if expected string not found then CONFIG file may be wrong */
  /*------------------------------------------------------------*/
	rid = C_ERROR_CONF_INVAL;

	return rid;
}

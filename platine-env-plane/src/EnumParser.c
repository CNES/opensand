/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : this class implements methods handling (String, Int32) couple 
               arrays corresponding to enum (string + associated INT32 value) 
    @HISTORY :
    03-02-26 : Creation
    03-06-20 : Using strcasecmp instead of strcmp to avoid UpperCase/LowerCase
               differences when checking string values (P.Lo)
*/
/*--------------------------------------------------------------------------*/

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

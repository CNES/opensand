/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : this class implements methods handling (String, Int32) couple 
               arrays corresponding to enum (string + associated INT32 value) 
    @HISTORY :
    03-02-26 : Creation
*/
/*--------------------------------------------------------------------------*/

#ifndef EnumParser_e
#   define EnumParser_e

#   include "EnumCouple_e.h"
#   include "Error_e.h"


/*  @ROLE    : get index corresponding to a STRING in an enumCouple array
    @RETURN  : Error code */
T_ERROR ENUM_PARSER_Parse(
									 /* IN    */ T_STRING str,
									 /* IN    */ T_ENUM_COUPLE choices[],
									 /*   OUT */ T_INT32 * ptr_value);

/*  @ROLE    : get index corresponding to a STRING in an enumLongCouple array
    @RETURN  : Error code */
T_ERROR ENUM_PARSER_ParseLong(
										  /* IN    */ T_STRING str,
										  /* IN    */ T_ENUM_LONGCOUPLE choices[],
										  /*   OUT */ T_INT64 * ptr_value);


/*  @ROLE    : get string corresponding to an index in an enumCouple array
    @RETURN  : Error code */
T_ERROR ENUM_PARSER_ParseString(
											 /* IN    */ T_INT32 integerValue,
											 /* IN    */ T_ENUM_COUPLE choices[],
											 /*   OUT */ T_STRING ptr_string_value);


#endif

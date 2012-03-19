/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : this class implements a (String, Int32) couple value 
               corresponding to enum (string + associated INT32 value) 
    @HISTORY :
    03-02-26 : Creation
*/
/*--------------------------------------------------------------------------*/

#ifndef EnumCouple_e
#   define EnumCouple_e

#   include "Types_e.h"

/* When defining an array of EnumCouple, an additional Couple shall
   be inserted as the last array parameter, with String value set to NULL.
   For example : T_ENUM_COUPLE toto[] = { "Value1", C_VALUE_1,
                                          "Value2", C_VALUE_2,
                                          NULL};                     */

#   define C_ENUM_COUPLE_MAX_STRING_LEN 32
#   define C_ENUM_COUPLE_NULL {"\0", 0}

typedef struct
{

	T_CHAR _StrValue[C_ENUM_COUPLE_MAX_STRING_LEN];
	T_INT32 _IntValue;

} T_ENUM_COUPLE;

typedef struct
{

	T_CHAR _StrValue[C_ENUM_COUPLE_MAX_STRING_LEN];
	T_INT64 _IntValue;

} T_ENUM_LONGCOUPLE;


#endif

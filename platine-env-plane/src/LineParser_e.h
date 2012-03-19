#ifndef LineParser_e
#   define LineParser_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The LineParser class implements methods to read 
               different data on the same line in configuration files. 
    @HISTORY :
    03-02-27 : Creation
*/
/*--------------------------------------------------------------------------*/

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

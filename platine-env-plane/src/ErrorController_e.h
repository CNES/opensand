/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe RENAUDY - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    :  
    @HISTORY :
*/
/*--------------------------------------------------------------------------*/

#ifndef _ERROR_CONTROLLER_H
#   define _ERROR_CONTROLLER_H

#   include "Types_e.h"

#   define ERROR_BUFFER_SIZE       1023
#   define ERROR_BUFFER_NB_FIELDS  8

typedef struct
{
	T_UINT8 telemetry_error_category;
	T_UINT32 telemetry_error_date;
	T_CHAR telemetry_error_name[SPRINT_MAX_LEN];
	T_CHAR telemetry_error_index_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_error_index_value;
	T_CHAR telemetry_error_value_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_error_value;
	T_CHAR telemetry_error_unit[SPRINT_MAX_LEN];
} T_ERROR_BUFFER_ELEMENT;


typedef T_ERROR_BUFFER_ELEMENT T_ERROR_BUFFER[ERROR_BUFFER_SIZE];

T_UINT16 get_error_counter(void);

T_UINT8 get_error_category(void);

T_UINT32 get_error_date(void);

T_STRING get_error_name(void);

T_STRING get_error_index_sign(void);

T_UINT32 get_error_index_value(void);

T_STRING get_error_value_sign(void);

T_UINT32 get_error_value(void);

T_STRING get_error_unit(void);


#endif /* _ERROR_CONTROLLER_H */

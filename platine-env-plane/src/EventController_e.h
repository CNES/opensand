/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe RENAUDY - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    :  
    @HISTORY :
*/
/*--------------------------------------------------------------------------*/

#ifndef _EVENT_CONTROLLER_H
#   define _EVENT_CONTROLLER_H

#   include "Types_e.h"
#   define EVENT_BUFFER_SIZE       1023
#   define EVENT_BUFFER_NB_FIELDS  8

typedef struct
{
	T_UINT8 telemetry_event_category;
	T_UINT32 telemetry_event_date;
	T_CHAR telemetry_event_name[SPRINT_MAX_LEN];
	T_CHAR telemetry_event_index_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_event_index_value;
	T_CHAR telemetry_event_value_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_event_value;
	T_CHAR telemetry_event_unit[SPRINT_MAX_LEN];
} T_EVENT_BUFFER_ELEMENT;


typedef T_EVENT_BUFFER_ELEMENT T_EVENT_BUFFER[EVENT_BUFFER_SIZE];


T_UINT16 get_event_counter(void);

T_UINT8 get_event_category(void);

T_UINT32 get_event_date(void);

T_STRING get_event_name(void);

T_STRING get_event_index_sign(void);

T_UINT32 get_event_index_value(void);

T_STRING get_event_value_sign(void);

T_UINT32 get_event_value(void);

T_STRING get_event_unit(void);


#endif /* _EVENT_CONTROLLER_H */

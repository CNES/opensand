/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The OutputFormatter class implements the event message fields 
    @HISTORY :
    03-03-04 : Creation
*/
/*--------------------------------------------------------------------------*/

#ifndef EventOutputFormatter_e
#   define EventOutputFormatter_e

#   include "Types_e.h"
#   include "Error_e.h"
#   include "GenericPacket_e.h"
#   include "EventsDef_e.h"


#   define C_MAX_CAR_EVT_TRACE_FIELD   32
													/* Maximum number of characters in one output message field */


typedef struct
{
	T_UINT32 _frame_number;
	T_UINT8 _FSM_number;
} T_OF_EVENT_DATE;

typedef struct
{
	T_CHAR _componentType[C_MAX_CAR_EVT_TRACE_FIELD];
	T_UINT8 _InstanceId;
} T_OF_EVENT_ORIGIN;


typedef struct
{
	T_OF_EVENT_DATE _event_date;
	T_OF_EVENT_ORIGIN _event_origin;
	T_CHAR _event_name[C_EVT_DEF_MAX_CAR_NAME];
	T_CHAR _category[C_MAX_CAR_EVT_TRACE_FIELD];
	T_CHAR _index_signification[C_EVT_DEF_MAX_CAR_IDX_SIGN];
	T_UINT32 _index_value;
	T_CHAR _value_signification[C_EVT_DEF_MAX_CAR_VAL_SIGN];
	T_UINT32 _value;
	T_CHAR _unit[C_EVT_DEF_MAX_CAR_UNIT];
} T_EVENT_OUTPUT_FORMATTER;


/*  @ROLE    : Initialise output formatter class  
               configuration file
    @RETURN  : Error code */
T_ERROR T_EVENT_OUTPUT_FORMATTER_Init(
/* INOUT */ T_EVENT_OUTPUT_FORMATTER *
													 ptr_this);


/*  @ROLE    : create event message corresponding to a given element of 
               event generic packet
    @RETURN  : Error code */
T_ERROR T_EVENT_OUTPUT_FORMATTER_Formatter(
/* INOUT */
															T_EVENT_OUTPUT_FORMATTER
															* ptr_this,
/* IN    */
															T_EVENTS_DEF * ptr_eventsDef,
/* IN    */
															T_GENERIC_PKT * ptr_gen_pkt,
/* IN    */
															T_ELT_GEN_PKT * ptr_elt_pkt);


#endif

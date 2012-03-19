/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The OutputFormatter class implements the error message fields 
    @HISTORY :
    03-02-24 : Creation
*/
/*--------------------------------------------------------------------------*/

#ifndef ErrorOutputFormatter_e
#   define ErrorOutputFormatter_e

#   include "Types_e.h"
#   include "Error_e.h"
#   include "GenericPacket_e.h"
#   include "ErrorDef_e.h"


#   define C_MAX_CAR_ERR_TRACE_FIELD   32
													/* Maximum number of characters in one output message field */


typedef struct
{
	T_UINT32 _frame_number;
	T_UINT8 _FSM_number;
} T_OF_ERR_DATE;

typedef struct
{
	T_CHAR _componentType[C_MAX_CAR_ERR_TRACE_FIELD];
	T_UINT8 _InstanceId;
} T_OF_ERR_ORIGIN;


typedef struct
{
	T_OF_ERR_DATE _error_date;
	T_OF_ERR_ORIGIN _error_origin;
	T_CHAR _error_name[C_ERR_DEF_MAX_CAR_NAME];
	T_UINT32 _error_index;
	T_CHAR _category[C_MAX_CAR_ERR_TRACE_FIELD];
	T_CHAR _index_signification[C_ERR_DEF_MAX_CAR_IDX_SIGN];
	T_INDEX_VALUE _index_value;
	T_UINT32 _index;
	T_CHAR _value_signification[C_ERR_DEF_MAX_CAR_VAL_SIGN];
	T_UINT32 _value;
	T_CHAR _unit[C_ERR_DEF_MAX_CAR_UNIT];
} T_ERROR_OUTPUT_FORMATTER;


/*  @ROLE    : Initialise output formatter class  
               configuration file
    @RETURN  : Error code */
T_ERROR T_ERROR_OUTPUT_FORMATTER_Init(
/* INOUT */ T_ERROR_OUTPUT_FORMATTER *
													 ptr_this);


/*  @ROLE    : create error message corresponding to a given element of 
               error generic packet
    @RETURN  : Error code */
T_ERROR T_ERROR_OUTPUT_FORMATTER_Formatter(
/* INOUT */
															T_ERROR_OUTPUT_FORMATTER
															* ptr_this,
/* IN    */
															T_ERRORS_DEF * ptr_errorsDef,
/* IN    */
															T_GENERIC_PKT * ptr_gen_pkt,
/* IN    */
															T_ELT_GEN_PKT * ptr_elt_pkt);


#endif

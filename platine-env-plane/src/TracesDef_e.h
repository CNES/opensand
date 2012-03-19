#ifndef TracesDef_e
#   define TracesDef_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Paul LAFARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The TraceDefinition class implements the reading of 
               trace definition configuration file 
    @HISTORY :
    03-02-26 : Creation
*/
/*--------------------------------------------------------------------------*/
#   include "Types_e.h"
#   include "Error_e.h"
#   include "EnumCouple_e.h"

/* All these limits shall be reconsidered at integration-time  */
#   define C_TRACE_DEF_MAX_CAR_NAME     64
#   define C_TRACE_DEF_MAX_CAR_MODE     64

typedef struct
{
	T_INT64 _Name;
	T_INT64 _Mode;
} T_TRACE_DEF;

#   define C_TRACE_DEF_MAX_TRACES  500

typedef struct
{
	T_UINT32 _nbTrace;
	T_TRACE_DEF _Trace[C_TRACE_DEF_MAX_TRACES];

	T_ENUM_LONGCOUPLE C_TRACE_MODE_choices[C_TRACE_DEF_MAX_TRACES];
	T_ENUM_LONGCOUPLE C_TRACE_COMP_choices[C_TRACE_DEF_MAX_TRACES];

} T_TRACES_DEF;


T_ERROR TRACES_DEF_ReadConfigFile(
												/* INOUT */ T_TRACES_DEF * ptr_this,
												/* IN    */ T_UINT16 SimReference,
												/* IN    */ T_UINT16 SimRun);


#endif

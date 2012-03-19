#ifndef EventsDef_e
#   define EventsDef_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The EventsDefinition class implements the reading of 
               events definition configuration file 
    @HISTORY :
    03-02-26 : Creation
    03-10-13 : Add XML data (GM)
*/
/*--------------------------------------------------------------------------*/

#   include "Error_e.h"

/* All these limits shall be reconsidered at integration-time  */
#   define C_EVT_DEF_MAX_CAR_NAME     32
#   define C_EVT_DEF_MAX_CAR_IDX_SIGN 32
#   define C_EVT_DEF_MAX_CAR_VAL_SIGN 32
#   define C_EVT_DEF_MAX_CAR_UNIT     32
#   define C_EVT_DEF_MAX_EVENTS       50


typedef struct
{										  /* LEVEL 1 */
	T_INT32 _EventId;
	T_CHAR _Name[C_EVT_DEF_MAX_CAR_NAME];
	T_INT32 _Category;
	T_CHAR _IndexSignification[C_EVT_DEF_MAX_CAR_IDX_SIGN];
	T_CHAR _ValueSignification[C_EVT_DEF_MAX_CAR_VAL_SIGN];
	T_CHAR _Unit[C_EVT_DEF_MAX_CAR_UNIT];
} T_EVENT_DEF;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _nbEvent;
	T_EVENT_DEF _Event[C_EVT_DEF_MAX_EVENTS];
} T_EVENTS_DEF;


T_ERROR EVENTS_DEF_ReadConfigFile(
												/* INOUT */ T_EVENTS_DEF * ptr_this);


#endif /* EventsDef_e */

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ErrorAgent class implements the error agent services
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/
#ifndef ErrorAgent_e
#   define ErrorAgent_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "DominoConstants_e.h"
#   include "IPAddr_e.h"
#   include "GenericPort_e.h"
#   include "GenericPacket_e.h"

#   define C_MAX_ERROR_PKT_ELT_NB 1
											/* Maximum Number of Elements in 1 Error Packet */

typedef struct
{

	T_GENERIC_PORT _genericPort;
	T_GENERIC_PKT *_ptr_genPacket;
	T_ERROR_CATEGORY _LastErrorCat;
	T_ERROR_INDEX _LastErrorIndex;
	T_ERROR_VALUE _LastErrorValue;
	T_ERROR _rid;

	T_UINT32 *_FRSFramecount;	  /* Used to retrieve FRSFrame from execution context */
	T_UINT8 *_FSMNumber;			  /* Used to retrieve FSM number from execution context */

} T_ERROR_AGENT;


/*  @ROLE    : This function initialises the error agent
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_Init(
									/* INOUT */ T_ERROR_AGENT * ptr_this,
									/* IN    */ T_IP_ADDR * ptr_ipAddr,
									/* IN    */ T_INT32 ComponentId,
									/* IN    */ T_INT32 InstanceId,
									/* IN    */ T_UINT32 * FRSRef,
									/* IN    */ T_UINT8 * FSMRef);


/*  @ROLE    : This function terminates the error agent
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_Terminate(
										  /* INOUT */ T_ERROR_AGENT * ptr_this);


/*  @ROLE    : This function set the error category/index, the error value is set to errno
    @RETURN  : None */
T_ERROR ERROR_AGENT_SetLastErrorErrno(
													 /* INOUT */ T_ERROR_AGENT * ptr_this,
													 /* IN    */ T_ERROR_CATEGORY cat,
													 /* IN    */ T_ERROR_INDEX index,
													 /* IN    */ T_ERROR error);

/*  @ROLE    : This function set the error category/index/value
    @RETURN  : None */
T_ERROR ERROR_AGENT_SetLastError(
											  /* INOUT */ T_ERROR_AGENT * ptr_this,
											  /* IN    */ T_ERROR_CATEGORY cat,
											  /* IN    */ T_ERROR_INDEX index,
											  /* IN    */ T_ERROR_VALUE value,
											  /* IN    */ T_ERROR error);


/*  @ROLE    : This function sends Error to Error Controller
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_SendError(
										  /* INOUT */ T_ERROR_AGENT * ptr_this);


#endif

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The EventAgent class implements the event agent services
    @HISTORY :
    03-03-21 : Creation
*/
/*--------------------------------------------------------------------------*/

#ifndef EventAgent_e
#   define EventAgent_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "ErrorAgent_e.h"
#   include "Error_e.h"
#   include "Event_e.h"
#   include "DominoConstants_e.h"
#   include "IPAddr_e.h"
#   include "GenericPort_e.h"
#   include "GenericPacket_e.h"
#   include "EventsActivation_e.h"

#   define C_MAX_EVENT_PKT_ELT_NB 1
											/* Maximum Number of Elements in 1 Event Packet */
#   define C_MAX_EVENT_ON_PERIOD 128/* Maximum number of error sent by one component in one period */


typedef struct
{
	T_GENERIC_PORT _genericPort;
	T_GENERIC_PKT *_ptr_genPacket;
	T_EVENT_CATEGORY _LastEventCat[C_MAX_EVENT_ON_PERIOD];
	T_EVENT_INDEX _LastEventIndex[C_MAX_EVENT_ON_PERIOD];
	T_EVENT_VALUE _LastEventValue[C_MAX_EVENT_ON_PERIOD];
	T_BOOL _eventValueIsGoingToBeSent[C_MAX_EVENT_ON_PERIOD];
	T_EVENT _Event[C_MAX_EVENT_ON_PERIOD];
	T_UINT32 _nbEventIndex;
	T_UINT32 _FRSNbr[C_MAX_EVENT_ON_PERIOD];	/* Frame of the event occuration */
	T_UINT8 _FSMId[C_MAX_EVENT_ON_PERIOD];	/* FSM Id of the Event occuration */
	T_UINT32 _FSM_OfLastSent;	  /* FSM Id of the Event occuration */

	T_UINT32 *_FRSFramecount;	  /* Used to retrieve FRSFrame from execution context */
	T_UINT8 *_FSMIdentifier;	  /* Used to retrieve FSM number from execution context */
	T_EVENTS_ACTIVATION MyActivation;
	T_ERROR_AGENT *ptr_errorAgent;	/* pointer on error agent */
	T_BOOL _IsToSend;
} T_EVENT_AGENT;


/*  @ROLE    : This function initialises the Event agent
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_Init(
									/* INOUT */ T_EVENT_AGENT * ptr_this,
									/* IN    */ T_ERROR_AGENT * ptr_errorAgent,
									/* IN    */ T_IP_ADDR * ptr_ipAddr,
									/* IN    */ T_INT32 ComponentId,
									/* IN    */ T_INT32 InstanceId,
									/* IN    */ T_UINT16 SimReference,
									/* IN    */ T_UINT16 SimRun,
									/* IN    */ T_UINT32 * FRSRef,
									/* IN    */ T_UINT8 * FSMRef);


/*  @ROLE    : This function terminates the Event agent
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_Terminate(
										  /* INOUT */ T_EVENT_AGENT * ptr_this);


/*  @ROLE    : This function set the Event category/index/value
    @RETURN  : None */
T_ERROR EVENT_AGENT_SetLastEvent(
											  /* INOUT */ T_EVENT_AGENT * ptr_this,
											  /* IN    */ T_EVENT_CATEGORY cat,
											  /* IN    */ T_EVENT_INDEX index,
											  /* IN    */ T_EVENT_VALUE value,
											  /* IN    */ T_EVENT Event);


/*  @ROLE    : This function sends Event to Event Controller
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_SendEvent(
										  /* INOUT */ T_EVENT_AGENT * ptr_this);

/*  @ROLE    : This function sends Event to Event Controller
    @RETURN  : Error code */
T_ERROR EVENT_AGENT_SendAllEvents(
												/* INOUT */ T_EVENT_AGENT * ptr_this);

#endif

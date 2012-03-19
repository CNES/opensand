/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The GenericPort class implements the reveiver and sender
               functions for the generic packet
    @HISTORY :
    03-02-20 : Creation
    04-05-05 : Set MAX_PKG to 2 (init of socket buffer) 
*/
/*--------------------------------------------------------------------------*/
#ifndef GenericPort_e
#   define GenericPort_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "IPAddr_e.h"
#   include "UDPSocket_e.h"
#   include "GenericPacket_e.h"

#   define C_GEN_PORT_RCV_MAX_PKG    2
												/* Max packet in the receiver socket buffer */
#   define C_GEN_PORT_SND_MAX_PKG    2
												/* Max packet in the sender socket buffer */
#   define C_SOCKET_HEADER_SIZE      16/* The socket header size */

typedef struct
{
	T_UDP_SOCKET _udpSocket;
	T_UINT32 _maxRecvSize;
	T_UINT32 _maxSendSize;
} T_GENERIC_PORT;


/*  @ROLE    : This function initialises a generic packet sender
    @RETURN  : Error code */
extern T_ERROR GENERIC_PORT_InitSender(
													  /* INOUT */ T_GENERIC_PORT * ptr_this,
													  /* IN    */ T_IP_ADDR * ptr_ipAddr,
													  /* IN    */ T_UINT32 maxGenElt);

/*  @ROLE    : This function initialises a generic packet receiver
    @RETURN  : Error code */
extern T_ERROR GENERIC_PORT_InitReceiver(
														 /* INOUT */ T_GENERIC_PORT *
														 ptr_this,
														 /* IN    */ T_IP_ADDR * ptr_ipAddr,
														 /* IN    */ T_UINT32 maxGenElt);

/*  @ROLE    : This function terminates a generic packet sender/receiver
    @RETURN  : Error code */
extern T_ERROR GENERIC_PORT_Terminate(
													 /* INOUT */ T_GENERIC_PORT * ptr_this);

/*  @ROLE    : is used to send a generic packet
    @RETURN  : Error code */
extern T_ERROR GENERIC_PORT_SendGenPacket(
														  /* INOUT */ T_GENERIC_PORT *
														  ptr_this,
														  /* IN    */
														  T_GENERIC_PKT * ptr_genPacket);

/*  @ROLE    : is used to receive a generic packet
    @RETURN  : Error code */
extern T_ERROR GENERIC_PORT_RecvGenPacket(
														  /* INOUT */ T_GENERIC_PORT *
														  ptr_this,
														  /*   OUT */
														  T_GENERIC_PKT * ptr_genPacket);

#endif /* GenericPort_e */

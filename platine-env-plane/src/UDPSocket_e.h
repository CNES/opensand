/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The UDPSocket class implements the reveiver and sender
               functions for the UDP socket
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/
#ifndef UDPSocket_e
#   define UDPSocket_e

/* SYSTEM RESOURCES */
#   include <sys/socket.h>
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "IPAddr_e.h"

#   define C_UDP_PAD_READ_SIZE_INET 16
#   define C_UDP_PAD_READ_SIZE_UNIX 3

typedef struct
{
	T_INT32 _socket;
	T_IP_ADDR _ipAddr;
} T_UDP_SOCKET;


/*  @ROLE    : This function initialises a UDP socket sender
    @RETURN  : Error code */
extern T_ERROR UDP_SOCKET_InitSender(
													/* INOUT */ T_UDP_SOCKET * ptr_this,
													/* IN    */ T_IP_ADDR * ptr_ipAddr,
													/* IN    */ T_UINT32 bufSize);

/*  @ROLE    : This function initialises a UDP socket receiver
    @RETURN  : Error code */
extern T_ERROR UDP_SOCKET_InitReceiver(
													  /* INOUT */ T_UDP_SOCKET * ptr_this,
													  /* IN    */ T_IP_ADDR * ptr_ipAddr,
													  /* IN    */ T_UINT32 bufSize,
													  /* IN    */ T_BOOL blockingIO);

/*  @ROLE    : This function terminates the UDP socket sender/receiver
    @RETURN  : Error code */
extern T_ERROR UDP_SOCKET_Terminate(
												  /* INOUT */ T_UDP_SOCKET * ptr_this);

/*  @ROLE    : is used to send data to the UDP socket
    @RETURN  : Error code */
extern T_ERROR UDP_SOCKET_SendBytes(
												  /* INOUT */ T_UDP_SOCKET * ptr_this,
												  /* IN    */ T_BUFFER buffer,
												  /* IN    */ T_INT32 size);

/*  @ROLE    : is used to receive data from the UDP socket
    @RETURN  : Error code */
extern T_ERROR UDP_SOCKET_RecvBytes(
												  /* INOUT */ T_UDP_SOCKET * ptr_this,
												  /*   OUT */ T_BUFFER buffer,
												  /* IN    */ T_INT32 size,
												  /*   OUT */ T_IP_ADDR * ptr_ipAddr,
												  /*   OUT */ T_INT32 * ptr_recvSize);

/*  @ROLE    : get the number of data bytes in UDP socket
    @RETURN  : Error code */
extern T_ERROR UDP_SOCKET_CheckRecvData(
														/* INOUT */ T_UDP_SOCKET * ptr_this,
														/*   OUT */ T_UINT32 * ptr_nbBytes);

#   define UDP_SOCKET_DecreaseRecvSize(ptr_this,ptr_nbBytes,size) \
{ \
      if ((((T_UDP_SOCKET *)ptr_this)->_ipAddr._family)==AF_INET) { \
        *(ptr_nbBytes) -= ((size) + C_UDP_PAD_READ_SIZE_INET); \
      } \
      if ((((T_UDP_SOCKET *)ptr_this)->_ipAddr._family)==AF_UNIX) { \
        *(ptr_nbBytes) -= ((size) + C_UDP_PAD_READ_SIZE_UNIX); \
      } \
      if((T_INT32)*((T_INT32*)(ptr_nbBytes)) < 0) { \
         TRACE_ERROR((C_TRACE_THREAD_UNKNOWN,C_TRACE_COMP_TRANSPORT,C_TRACE_ERROR, \
                     "UDP_SOCKET_DecreaseRecvSize() bad decreased size %d (Neg value=%d)",size,(T_INT32)*((T_INT32*)(ptr_nbBytes)))); \
         *(ptr_nbBytes) = 0; \
      } \
}

#endif /* UDPSocket_e */

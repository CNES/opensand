/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file UDPSocket_e.h
 * @author TAS
 * @brief The UDPSocket class implements the reveiver and sender
 *        functions for the UDP socket
 */

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

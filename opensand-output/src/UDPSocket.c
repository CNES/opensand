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
 * @file UDPSocket.c
 * @author TAS
 * @brief The UDPSocket class implements the receiver and sender functions
 *        for the UDP socket
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <errno.h>

/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "IPAddr_e.h"
#include "UDPSocket_e.h"


/*  @ROLE    : This function initialises a UDP socket sender
    @RETURN  : Error code */
T_ERROR UDP_SOCKET_InitSender(
										  /* INOUT */ T_UDP_SOCKET * ptr_this,
										  /* IN    */ T_IP_ADDR * ptr_ipAddr,
										  /* IN    */ T_UINT32 bufSize)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 realSize;
	socklen_t optLength;
	int on = 1;

	memset(ptr_this, 0, sizeof(T_UDP_SOCKET));

	/* Create sender IP address */
  /*--------------------------*/
	if(IP_ADDR_Init
		(&(ptr_this->_ipAddr), ptr_ipAddr->_addr, ptr_ipAddr->_port,
		 ptr_ipAddr->_family) != C_ERROR_OK)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitSender() cannot create sender IP address"));
	}


	/* create UDP Internet socket */
  /*----------------------------*/
	ptr_this->_socket = socket(ptr_this->_ipAddr._family, SOCK_DGRAM, 0);
	if(ptr_this->_socket < 0)
	{
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "socket");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitSender() cannot create socket"));
	}

	/* set socket buffer size */
  /*------------------------*/
	if(setsockopt
		(ptr_this->_socket, SOL_SOCKET, SO_SNDBUF, (void *) &bufSize,
		 sizeof(T_UINT32)) < 0)
	{
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							"setsockopt");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitSender() cannot set socket buffer size"));
	}

	/* get socket buffer size */
  /*------------------------*/
	optLength = (socklen_t) sizeof(T_UINT32);
	if(getsockopt
		(ptr_this->_socket, SOL_SOCKET, SO_SNDBUF, (void *) &realSize,
		 &optLength) < 0)
	{
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							"getsockopt");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitSender() cannot get socket buffer size"));
	}
	if(realSize < bufSize)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitSender() real buffer size %d is less than asked buffer size %d",
						realSize, bufSize));
	}


	/* set non blocking socket */
  /*-------------------------*/
	if(ioctl(ptr_this->_socket, FIONBIO, &on) < 0)
	{
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "ioctl");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitSender() cannot set ioctl"));
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
				  "UDP_SOCKET_InitSender() successful socket id %d buf size %d",
				  ptr_this->_socket, bufSize));

 FIN:
	return rid;
}


/*  @ROLE    : This function initialises a UDP socket receiver
    @RETURN  : Error code */
T_ERROR UDP_SOCKET_InitReceiver(
											 /* INOUT */ T_UDP_SOCKET * ptr_this,
											 /* IN    */ T_IP_ADDR * ptr_ipAddr,
											 /* IN    */ T_UINT32 bufSize,
											 /* IN    */ T_BOOL blockingIO)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 realSize;
	socklen_t optLength;
	T_CHAR MySystemString[255];
	int on = 1;



	/* Create receiver IP address */
  /*----------------------------*/
	if(IP_ADDR_Init
		(&(ptr_this->_ipAddr), NULL, ptr_ipAddr->_port,
		 ptr_ipAddr->_family) != C_ERROR_OK)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitReceiver() cannot create receiver IP address"));
	}

	/* Clean  socket file for */
	if(ptr_this->_ipAddr._family == AF_UNIX)
	{
		sprintf(MySystemString, "rm -f /tmp/tmp_socket_%u",
				  ptr_this->_ipAddr._port);
		if(system(MySystemString) < 0)
		{
			JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitReceiver() cannot execute system command"));
		}
	}

	/* create UDP Internet socket */
  /*----------------------------*/
	ptr_this->_socket = socket(ptr_this->_ipAddr._family, SOCK_DGRAM, 0);
	if(ptr_this->_socket < 0)
	{
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "socket");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitReceiver() cannot create socket"));
	}

	/* set socket buffer size */
  /*------------------------*/
	if(setsockopt
		(ptr_this->_socket, SOL_SOCKET, SO_RCVBUF, (char *) &bufSize,
		 sizeof(T_UINT32)) < 0)
	{
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							"setsockopt");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitReceiver() cannot set socket buffer size"));
	}

	/* get socket buffer size */
  /*------------------------*/
	optLength = (socklen_t) sizeof(T_UINT32);
	if(getsockopt
		(ptr_this->_socket, SOL_SOCKET, SO_RCVBUF, (char *) &realSize,
		 &optLength) < 0)
	{
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							"getsockopt");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitReceiver() cannot get socket buffer size"));
	}
	if(realSize < bufSize)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitReceiver() real buffer size %d is less than asked buffer size %d",
						realSize, bufSize));
	}

	/* set non blocking socket */
  /*-------------------------*/
	if(blockingIO == FALSE)
	{
		if(ioctl(ptr_this->_socket, FIONBIO, &on) < 0)
		{
			TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
								"ioctl");
			JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"UDP_SOCKET_InitReceiver() cannot set ioctl"));
		}
	}

	/* bind the receive UDP Internet socket */
  /*--------------------------------------*/
	switch (ptr_this->_ipAddr._family)
	{
	case AF_INET:
		/* !CB
		   if(bind(ptr_this->_socket,(SOCKET_ADDR_CAST_INET)&(ptr_this->_ipAddr._addrInet),
		   sizeof(ptr_this->_ipAddr._addrInet)) < 0) {
		 */
		if(bind
			(ptr_this->_socket, (struct sockaddr *) &(ptr_this->_ipAddr._addrInet),
			 sizeof(ptr_this->_ipAddr._addrInet)) < 0)
		{
			TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "bind");
			JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"UDP_SOCKET_InitReceiver() AF_INET family, cannot bind socket %s port %d",
							ptr_ipAddr->_addr, ptr_ipAddr->_port));
		}
		break;
	case AF_UNIX:
		/* !CB
		   if(bind(ptr_this->_socket,(SOCKET_ADDR_CAST_UNIX)&(ptr_this->_ipAddr._addrUnix),
		   sizeof(ptr_this->_ipAddr._addrUnix)) < 0) {
		 */
		if(bind
			(ptr_this->_socket, (struct sockaddr *) &(ptr_this->_ipAddr._addrUnix),
			 sizeof(ptr_this->_ipAddr._addrUnix)) < 0)
		{
			TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "bind");
			JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"UDP_SOCKET_InitReceiver() AF_UNIX family, cannot bind socket %s port %d",
							ptr_ipAddr->_addr, ptr_ipAddr->_port));
		}
		break;
	default:
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "bind");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_InitReceiver() Unknown family=%d cannot bind socket %s port %d",
						ptr_this->_ipAddr._family, ptr_ipAddr->_addr,
						ptr_ipAddr->_port));
	}
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
				  "UDP_SOCKET_InitReceiver() successful socket id %d buf size %d",
				  ptr_this->_socket, bufSize));

 FIN:
	return rid;
}


/*  @ROLE    : This function terminates the UDP socket sender/receiver
    @RETURN  : Error code */
T_ERROR UDP_SOCKET_Terminate(
										 /* INOUT */ T_UDP_SOCKET * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	/* close UDP Internet socket */
  /*---------------------------*/
#ifdef WIN32
	if(closesocket(ptr_this->_socket) < 0)
	{
#else
	if(close(ptr_this->_socket) < 0)
	{
#endif
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "close");
		TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
						 C_TRACE_ERROR,
						 "UDP_SOCKET_Terminate() cannot close the socket"));
		rid = C_ERROR_SOCK_OPEN;
	}
	ptr_this->_socket = -1;

	/* Terminate the IP address */
  /*--------------------------*/
	if(IP_ADDR_Terminate(&(ptr_this->_ipAddr)) != C_ERROR_OK)
	{
		TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
						 C_TRACE_ERROR,
						 "UDP_SOCKET_Terminate() cannot terminate IP address"));
		rid = C_ERROR_SOCK_OPEN;
	}


	return rid;
}


/*  @ROLE    : is used to send data to the UDP socket
    @RETURN  : Error code */
T_ERROR UDP_SOCKET_SendBytes(
										 /* INOUT */ T_UDP_SOCKET * ptr_this,
										 /* IN    */ T_BUFFER buffer,
										 /* IN    */ T_INT32 size)
{
	T_ERROR rid = C_ERROR_OK;
	T_INT32 bytes;

	switch (ptr_this->_ipAddr._family)
	{
	case AF_INET:
		/* !CB
		   bytes = sendto(ptr_this->_socket, buffer, size, 0, 
		   (SOCKET_ADDR_CAST_INET)&(ptr_this->_ipAddr._addrInet), 
		   sizeof(ptr_this->_ipAddr._addrInet));
		 */
		bytes = sendto(ptr_this->_socket, buffer, size, 0,
							(struct sockaddr *) &(ptr_this->_ipAddr._addrInet),
							sizeof(ptr_this->_ipAddr._addrInet));

		if(bytes < 0)
		{
			TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
								"sendto");
			JUMP_TRACE(FIN, rid, C_ERROR_SOCK_WRITE,
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"UDP_SOCKET_SendBytes() cannot send buffer size=%u (Addr=%s port=%u family is %d)",
							size, ptr_this->_ipAddr._addr, ptr_this->_ipAddr._port,
							ptr_this->_ipAddr._family));
		}
		if(bytes != size)
		{
			TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
								"sendto");
			JUMP_TRACE(FIN, rid, C_ERROR_SOCK_WRITE,
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"UDP_SOCKET_SendBytes() try to send buffer size=%u (with Addr=%s port=%u family is %d)but bytes=%u sent",
							size, ptr_this->_ipAddr._addr, ptr_this->_ipAddr._port,
							ptr_this->_ipAddr._family));
		}
		break;
	case AF_UNIX:
		/*  !CB
		   bytes = sendto(ptr_this->_socket, buffer, size, 0, 
		   (SOCKET_ADDR_CAST_UNIX)&(ptr_this->_ipAddr._addrUnix), 
		   sizeof(ptr_this->_ipAddr._addrUnix)); */
		bytes = sendto(ptr_this->_socket, buffer, size, 0,
							(struct sockaddr *) &(ptr_this->_ipAddr._addrUnix),
							sizeof(ptr_this->_ipAddr._addrUnix));
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_DEBUG,
					  "UDP SOCKET sendto port=%u size=%d",
					  ptr_this->_ipAddr._port, size));
		break;
	default:
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "sendto");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_WRITE,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_SendBytes() Unknown family=%d, cannot send buffer",
						ptr_this->_ipAddr._family));
	}


	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
				  "UDP_SOCKET_SendBytes() send %d bytes on socket id %d",
				  bytes, ptr_this->_socket));

 FIN:
	return rid;
}


/*  @ROLE    : is used to receive data from the UDP socket
    @RETURN  : Error code */
T_ERROR UDP_SOCKET_RecvBytes(
										 /* INOUT */ T_UDP_SOCKET * ptr_this,
										 /*   OUT */ T_BUFFER buffer,
										 /* IN    */ T_INT32 size,
										 /*   OUT */ T_IP_ADDR * ptr_ipAddr,
										 /*   OUT */ T_INT32 * ptr_recvSize)
{
	T_ERROR rid = C_ERROR_OK;
	/* !CB instead of int */
	socklen_t addrLen = 0;
	extern int errno;

	switch (ptr_this->_ipAddr._family)
	{
	case AF_INET:
		if(ptr_ipAddr != NULL)
		{
			addrLen = sizeof(ptr_ipAddr->_addrInet);
			/* !CB
			 *ptr_recvSize = recvfrom(ptr_this->_socket, buffer, size, 0, 
			 (SOCKET_ADDR_CAST_INET)&(ptr_ipAddr->_addrInet), &addrLen); 
			 */
			*ptr_recvSize = recvfrom(ptr_this->_socket, buffer, size, 0,
											 (struct sockaddr *) &(ptr_ipAddr->_addrInet),
											 &addrLen);

			/* Store the addr values */
		  /*-----------------------*/
			sprintf((char *) &(ptr_ipAddr->_addr[0]), "%d.%d.%d.%d",
					  *(((unsigned char *) &(ptr_ipAddr->_addrInet.sin_addr.s_addr))
						 + 0),
					  *(((unsigned char *) &(ptr_ipAddr->_addrInet.sin_addr.s_addr))
						 + 1),
					  *(((unsigned char *) &(ptr_ipAddr->_addrInet.sin_addr.s_addr))
						 + 2),
					  *(((unsigned char *) &(ptr_ipAddr->_addrInet.sin_addr.s_addr))
						 + 3));

			/* Store the port values */
		  /*-----------------------*/
			(ptr_ipAddr->_port) = ntohs(ptr_ipAddr->_addrInet.sin_port);

		}
		else
		{
			*ptr_recvSize =
				recvfrom(ptr_this->_socket, buffer, size, 0, NULL, &addrLen);
		}
		break;
	case AF_UNIX:
		if(ptr_ipAddr != NULL)
		{
			int nbRetry = 0;
			*ptr_recvSize = -1;
			addrLen = sizeof(ptr_ipAddr->_addrUnix);
			while((nbRetry < 3) && (*ptr_recvSize < 0))
			{
				/* !CB          *ptr_recvSize = recvfrom(ptr_this->_socket, buffer, size, 0, 
				   (SOCKET_ADDR_CAST_UNIX)&(ptr_ipAddr->_addrUnix), &addrLen);
				 */
				*ptr_recvSize = recvfrom(ptr_this->_socket, buffer, size, 0,
												 (struct sockaddr *) &(ptr_ipAddr->
																			  _addrUnix),
												 &addrLen);
				nbRetry++;
				if(*ptr_recvSize < 0)
				{
					if(errno == EINTR)
					{
						rid = C_ERROR_END_SIMU;
						TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
									  C_TRACE_VALID,
									  "UDP_SOCKET_RecvBytes() interrupted function call"));
						goto FIN;
					}
				}
			}
		}
		else
		{
			int nbRetry = 0;
			*ptr_recvSize = -1;
			while((nbRetry < 3) && (*ptr_recvSize < 0))
			{
				*ptr_recvSize =
					recvfrom(ptr_this->_socket, buffer, size, 0, NULL, &addrLen);
				nbRetry++;
				if(*ptr_recvSize < 0)
				{
					if(errno == EINTR)
					{
						rid = C_ERROR_END_SIMU;
						TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
									  C_TRACE_VALID,
									  "UDP_SOCKET_RecvBytes() interrupted function call"));
						goto FIN;
					}
				}
			}
		}
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_DEBUG,
					  "UDP SOCKET recfrom port=%u size=%d",
					  ptr_this->_ipAddr._port, *ptr_recvSize));
		break;
	default:
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							"recvfrom");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_READ,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_RecvBytes() unknown family %d cannot receive buffer",
						ptr_this->_ipAddr._family));
	}

	if(*ptr_recvSize < 0)
	{
		if(errno == EINTR)
		{
			rid = C_ERROR_END_SIMU;
			TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
						  C_TRACE_VALID,
						  "UDP_SOCKET_RecvBytes() interrupted function call"));
			goto FIN;
		}
		else
		{
			TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
								"recvfrom");
			JUMP_TRACE(FIN, rid, C_ERROR_SOCK_READ,
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"UDP_SOCKET_RecvBytes() cannot receive buffer"));
		}
	}

	if(ptr_ipAddr != NULL)
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
					  "UDP_SOCKET_RecvBytes() receive %d bytes on socket id %d from IP %s on port %d",
					  *ptr_recvSize, ptr_this->_socket, ptr_ipAddr->_addr,
					  ptr_ipAddr->_port));
	}
	else
	{
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
					  "UDP_SOCKET_RecvBytes() receive %d bytes on socket id %d",
					  *ptr_recvSize, ptr_this->_socket));
	}

 FIN:
	return rid;
}


/*  @ROLE    : get the number of data bytes in UDP socket
    @RETURN  : Error code */
T_ERROR UDP_SOCKET_CheckRecvData(
											  /* INOUT */ T_UDP_SOCKET * ptr_this,
											  /*   OUT */ T_UINT32 * ptr_nbBytes)
{
	T_ERROR rid = C_ERROR_OK;

	if(ioctl(ptr_this->_socket, FIONREAD, ptr_nbBytes) < 0)
	{
		*ptr_nbBytes = 0;
		TRACE_SYSERROR(C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, "ioctl");
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_READ,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"UDP_SOCKET_CheckRecvData() cannot get the number of data bytes in socket"));
	}

 FIN:
	return rid;
}

/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file GenericPort.c
 * @author TAS
 * @brief The GenericPort class implements the reveiver and sender functions
 *        for the generic packet
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "IPAddr_e.h"
#include "UDPSocket_e.h"
#include "GenericPacket_e.h"
#include "GenericPort_e.h"


/*  @ROLE    : This function initialises a generic packet sender
    @RETURN  : Error code */
T_ERROR GENERIC_PORT_InitSender(
											 /* INOUT */ T_GENERIC_PORT * ptr_this,
											 /* IN    */ T_IP_ADDR * ptr_ipAddr,
											 /* IN    */ T_UINT32 maxGenElt)
{

	memset(ptr_this, 0, sizeof(T_GENERIC_PORT));

	/* Calculate the max sender size */
  /*-------------------------------*/
	ptr_this->_maxSendSize = HD_GEN_PKT_SIZE + (ELT_GEN_PKT_SIZE * maxGenElt);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
				  "GENERIC_PORT_InitSender() init sender for %d max gen packet",
				  maxGenElt));

	/* Init UDP socket */
  /*-----------------*/
	return (UDP_SOCKET_InitSender(&(ptr_this->_udpSocket), ptr_ipAddr,
											(ptr_this->_maxSendSize *
											 C_GEN_PORT_SND_MAX_PKG) +
											(C_SOCKET_HEADER_SIZE *
											 C_GEN_PORT_SND_MAX_PKG)));
}


/*  @ROLE    : This function initialises a generic packet receiver
    @RETURN  : Error code */
T_ERROR GENERIC_PORT_InitReceiver(
												/* INOUT */ T_GENERIC_PORT * ptr_this,
												/* IN    */ T_IP_ADDR * ptr_ipAddr,
												/* IN    */ T_UINT32 maxGenElt)
{
	/* Calculate the max receive size */
  /*--------------------------------*/
	ptr_this->_maxRecvSize = HD_GEN_PKT_SIZE + (ELT_GEN_PKT_SIZE * maxGenElt);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
				  "GENERIC_PORT_InitSender() init receiver for %d max gen packet",
				  maxGenElt));

	/* Init UDP socket */
  /*-----------------*/
	return (UDP_SOCKET_InitReceiver(&(ptr_this->_udpSocket), ptr_ipAddr,
											  (ptr_this->_maxRecvSize *
												C_GEN_PORT_RCV_MAX_PKG) +
											  (C_SOCKET_HEADER_SIZE *
												C_GEN_PORT_RCV_MAX_PKG), TRUE));
}


/*  @ROLE    : This function terminates a generic packet sender/receiver
    @RETURN  : Error code */
T_ERROR GENERIC_PORT_Terminate(
											/* INOUT */ T_GENERIC_PORT * ptr_this)
{
	return (UDP_SOCKET_Terminate(&(ptr_this->_udpSocket)));
}


/*  @ROLE    : is used to send a generic packet
    @RETURN  : Error code */
T_ERROR GENERIC_PORT_SendGenPacket(
												 /* INOUT */ T_GENERIC_PORT * ptr_this,
												 /* IN    */ T_GENERIC_PKT * ptr_genPacket)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 packetSize;

	JUMP_ERROR_TRACE(FIN, rid, GENERIC_PACKET_SizeOf(ptr_genPacket, &packetSize),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"GENERIC_PORT_SendGenPacket() cannot get the generic packet size"));

	/* Check the receive size */
  /*------------------------*/
	if(packetSize > ptr_this->_maxSendSize)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_WRITE,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"GENERIC_PORT_SendGenPacket() buffer size %d is too big (maximun allowed %d)",
						packetSize, ptr_this->_maxSendSize));
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
				  "GENERIC_PORT_SendGenPacket() send %d bytes", packetSize));

	/* Send the generic packet */
  /*-------------------------*/
	rid =
		UDP_SOCKET_SendBytes(&(ptr_this->_udpSocket), (T_BUFFER) ptr_genPacket,
									(T_INT32) packetSize);

 FIN:
	return rid;
}


/*  @ROLE    : is used to receive a generic packet
    @RETURN  : Error code */
T_ERROR GENERIC_PORT_RecvGenPacket(
												 /* INOUT */ T_GENERIC_PORT * ptr_this,
												 /*   OUT */ T_GENERIC_PKT * ptr_genPacket)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 size, packetSize;

	/* Receive the generic packet */
  /*----------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, UDP_SOCKET_RecvBytes(&(ptr_this->_udpSocket),
																	(T_BUFFER) ptr_genPacket,
																	(T_INT32) ptr_this->
																	_maxRecvSize, NULL,
																	(T_INT32 *) & size),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"GENERIC_PORT_RecvGenPacket() bad receive packet"));

	/* Get the generic packet size */
  /*-----------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, GENERIC_PACKET_SizeOf(ptr_genPacket, &packetSize),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
							C_TRACE_ERROR,
							"GENERIC_PORT_RecvGenPacket() cannot get the generic packet size"));

	/* Check the receive size */
  /*------------------------*/
	if(size != packetSize)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_READ,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"GENERIC_PORT_RecvGenPacket() bad receive size %d (expected %d)",
						size, packetSize));
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
				  "GENERIC_PORT_RecvGenPacket() receive %d bytes", size));


 FIN:
	return rid;
}

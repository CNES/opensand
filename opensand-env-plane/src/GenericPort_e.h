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
 * @file GenericPort_e.h
 * @author TAS
 * @brief The GenericPort class implements the reveiver and sender
 *        functions for the generic packet
 */

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

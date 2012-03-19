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
 * @file IPAddr_e.h
 * @author TAS
 * @brief The IpAddr class generates "system-formatted" addresses
 */

#ifndef IPAddr_e
#   define IPAddr_e

/* SYSTEM RESOURCES */
#   include <netinet/in.h>
#   include <sys/un.h>
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"

#   define C_IP_MAX_LEN              20/* IP address or IP MASK maximum length */

typedef struct
{
	T_CHAR _addr[C_IP_MAX_LEN];
	T_UINT16 _padding;
	T_UINT16 _port;
	T_UINT8 _family;
	struct sockaddr_in _addrInet;
	struct sockaddr_un _addrUnix;
} T_IP_ADDR;


/*  @ROLE    : This function creates "system-formatted" addresses
    @RETURN  : Error code */
extern T_ERROR IP_ADDR_Init(
										/* INOUT */ T_IP_ADDR * ptr_this,
										/* IN    */ T_STRING addr,
										/* IN    */ T_UINT16 port,
										/* IN    */ T_UINT8 family);

/*  @ROLE    : This function terminates a IpAddr class
    @RETURN  : Error code */
extern T_ERROR IP_ADDR_Terminate(
											  /* INOUT */ T_IP_ADDR * ptr_this);

/*  @ROLE    : This function gives the IP "system-formatted" address
    @RETURN  : Error code */
extern T_ERROR IP_ADDR_GetIpAddr(
											  /* INOUT */ T_IP_ADDR * ptr_this,
											  /*   OUT */ T_UINT32 * ptr_addr);

#endif /* IPAddr_e */

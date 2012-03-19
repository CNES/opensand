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
 * @file IPAddr.c
 * @author TAS
 * @brief The IpAddr class generates "system-formatted" addresses
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "IPAddr_e.h"

/*  @ROLE    : This function creates "system-formatted" addresses
    @RETURN  : Error code */
T_ERROR IP_ADDR_Init(
							  /* INOUT */ T_IP_ADDR * ptr_this,
							  /* IN    */ T_STRING addr,
							  /* IN    */ T_UINT16 port,
							  /* IN    */ T_UINT8 family)
{
	T_ERROR rid = C_ERROR_OK;
	int ret, herr;
	struct hostent localAddress_r, *result;
	struct in_addr iaHost;
	size_t length = 0;
	T_CHAR fixed_buffer[200], localAddr[C_IP_MAX_LEN] =
	{
	'\0'};

	/* Reset the socket address */
  /*--------------------------*/
	memset(ptr_this, 0, sizeof(T_IP_ADDR));
	ptr_this->_addr[0] = '\0';

	/* Store the port values */
  /*-----------------------*/
	ptr_this->_port = port;

	switch (family)
	{
	case AF_INET:
		break;
	case AF_UNIX:
		break;
	default:
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
					  "IP_ADDR_Init() unknown family=%d (addr=%s port=%u) set by default to AF_INET (AF_INET=%d AF_UNIX=%d)",
					  ptr_this->_family, addr, port, AF_INET, AF_UNIX));
		family = AF_INET;
		break;
	}

	/* Store the family value */
	ptr_this->_family = family;

	/* Implementation socket */
  /*----------------------*/
	switch (ptr_this->_family)
	{
	case AF_INET:
		ptr_this->_addrInet.sin_family = AF_INET;
		ptr_this->_addrInet.sin_port = htons(ptr_this->_port);
		if(addr != NULL)
			length = strlen(addr);
		if(length == 0)
		{
			/* Store the addr values */
		  /*-----------------------*/
			strcpy((char *) &(ptr_this->_addr[0]), "INADDR_ANY");

			/* Implementation socket */
		  /*----------------------*/
			ptr_this->_addrInet.sin_addr.s_addr = htonl(INADDR_ANY);
		}
		else
		{
			if(strcasecmp(addr, "localhost") == 0)
				strcpy(localAddr, "127.0.0.1");
			else
				strcpy(localAddr, addr);

			/* Implementation socket */
		  /*----------------------*/
			ret = gethostbyname_r(localAddr,
										 &localAddress_r,
										 fixed_buffer,
										 sizeof(fixed_buffer), &result, (int *) &herr);
			if(ret != 0)
			{
				iaHost.s_addr = inet_addr(localAddr);
				ret =
					gethostbyaddr_r((char *) &(iaHost), sizeof(struct in_addr),
										 AF_INET, &localAddress_r, fixed_buffer,
										 sizeof(fixed_buffer), &result, &herr);
			}
			if(ret == 0)
			{
				memcpy(&(ptr_this->_addrInet.sin_addr.s_addr), result->h_addr,
						 result->h_length);

				/* Store the addr values */
			 /*-----------------------*/
				sprintf((char *) &(ptr_this->_addr[0]), "%d.%d.%d.%d",
						  *(((unsigned char *) &(ptr_this->_addrInet.sin_addr.s_addr))
							 + 0),
						  *(((unsigned char *) &(ptr_this->_addrInet.sin_addr.s_addr))
							 + 1),
						  *(((unsigned char *) &(ptr_this->_addrInet.sin_addr.s_addr))
							 + 2),
						  *(((unsigned char *) &(ptr_this->_addrInet.sin_addr.s_addr))
							 + 3));
			}
			else
			{
				JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
							  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT,
								C_TRACE_ERROR, "IP_ADDR_Init() bad address %s port %d",
								localAddr, port));
			}
		}
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_VALID,
					  "IP_ADDR_Init() port %d addr %s family %d (AF_INET=%d AF_UNIX=%d)",
					  ptr_this->_port,
					  ptr_this->_addr, ptr_this->_family, AF_INET, AF_UNIX));
		break;
	case AF_UNIX:
		ptr_this->_addrUnix.sun_family = AF_UNIX;
		if(addr != NULL)
			length = strlen(addr);
		sprintf(ptr_this->_addr, "%s", addr);
		if(length == 0)
		{
			/* Store the addr values */
		  /*-----------------------*/
			strcpy((char *) &(ptr_this->_addr[0]), "INADDR_ANY");
		}
		memset(ptr_this->_addrUnix.sun_path, 0,
				 sizeof(ptr_this->_addrUnix.sun_path));
		sprintf(ptr_this->_addrUnix.sun_path, "/tmp/tmp_socket_%u",
				  ptr_this->_port);
		break;
	default:
		JUMP_TRACE(FIN, rid, C_ERROR_SOCK_OPEN,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_TRANSPORT, C_TRACE_ERROR,
						"IP_ADDR_Init() unknown family=%d", ptr_this->_family));
	}

 FIN:
	return rid;
}


/*  @ROLE    : This function terminates a IpAddr class
    @RETURN  : Error code */
T_ERROR IP_ADDR_Terminate(
									 /* INOUT */ T_IP_ADDR * ptr_this)
{

	ptr_this->_addr[0] = '\0';
	ptr_this->_port = 0;
	ptr_this->_family = 0;
	memset(&(ptr_this->_addrInet), 0, sizeof(ptr_this->_addrInet));
	memset(&(ptr_this->_addrUnix), 0, sizeof(ptr_this->_addrUnix));

	return C_ERROR_OK;
}


/*  @ROLE    : This function gives the IP "system-formatted" address
    @RETURN  : Error code */
T_ERROR IP_ADDR_GetIpAddr(
									 /* INOUT */ T_IP_ADDR * ptr_this,
									 /*   OUT */ T_UINT32 * ptr_addr)
{
	*ptr_addr = ptr_this->_addrInet.sin_addr.s_addr;
	return C_ERROR_OK;
}

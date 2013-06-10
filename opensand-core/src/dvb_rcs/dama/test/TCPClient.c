/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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

/*************************** IDENTIFICATION *****************************
**
** Project          : GAC
**
** Module           : Simulators/RLSS_agent
**
** File name        : rlss_agent.c
**
** Description      : MIB group implementation
**
** Reference(s)     : 
**
** Contact          : Poucet B.
**
** Creation date    : 03/06/2002
**
************************************************************************/ 
	
/*#######################################################################
 #                                                                      #
 #  INCLUDES                                                            #
 #                                                                      #
 ######################################################################*/ 
/* include important headers */ 
	
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
	
/* global variables */ 
	
#define damaServPort 5555		  // OpenSAND interface
#define servIp "127.0.0.1"		  // OpenSAND interface
int main(int argc, char *argv[]) 
{
	struct sockaddr_in damaServAddr;	/* DAMA server address */
	int damaSocket;				  // OpenSAND interface
	char send_buff[32];
	int stringLen;
	
		/* open a socket to the DAMA */ 
		if((damaSocket = socket(AF_INET, SOCK_STREAM, 0)))
		
	{
		
			/* Construct the server address structure */ 
			memset(&damaServAddr, 0, sizeof(damaServAddr));	/* Zero out structure */
		damaServAddr.sin_family = AF_INET;	/* Internet address family */
		damaServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");	/* Server IP address */
		damaServAddr.sin_port = htons(damaServPort);	/* Server port */
		
			/* Establish the connection to the echo server */ 
			if(connect
				(damaSocket, (struct sockaddr *) &damaServAddr,
				 sizeof(damaServAddr)) < 0)
			
		{
			damaSocket = -1;
		}
	}
	
	else
		
	{
		damaSocket = -1;
	}
	sprintf(send_buff, "%d:%d", 32, 16);
	stringLen = strlen(send_buff);	/* Determine input length */
	
		/* Send the string to the server */ 
		if(damaSocket != -1)
	{
		if(send(damaSocket, send_buff, stringLen, 0) != stringLen)
		{
			fprintf(stderr,
						("send() sent a different number of bytes than expected"));
		}
	}
}



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

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define MAXPENDING 5				  /* Maximum outstanding connection requests */

#define RCVBUFSIZE 32			  /* Size of receive buffer */

void DieWithError(char *errorMessage)
{
	perror(errorMessage);
	exit(1);
}

void HandleTCPClient(int clntSocket)
{
	char echoBuffer[RCVBUFSIZE]; /* Buffer for echo string */
	int recvMsgSize;				  /* Size of received message */

	/* Receive message from client */
	if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE - 1, 0)) < 0)
		DieWithError("recv() failed");

	echoBuffer[recvMsgSize] = '\0';	/* Terminate the string! */
	printf(echoBuffer);			  /* Print the echo buffer */
	printf("\n");					  /* Print the echo buffer */

	close(clntSocket);			  /* Close client socket */
}

int main(int argc, char *argv[])
{
	extern int errno;
	int servSock;					  /* Socket descriptor for server */
	int clntSock;					  /* Socket descriptor for client */
	struct sockaddr_in echoServAddr;	/* Local address */
	struct sockaddr_in echoClntAddr;	/* Client address */
	unsigned short echoServPort; /* Server port */
	unsigned int clntLen;		  /* Length of client address data structure */
	int connect_flag = 0;
	long option = 0;

	if(argc != 2)					  /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
		exit(1);
	}

	echoServPort = atoi(argv[1]);	/* First arg:  local port */

	/* Create socket for incoming connections */
	if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");

	/* set the socket non blocking */
	//    if (setsockopt(servSock, SOL_SOCKET, NULL, 0) < 0)
	option = option | O_NONBLOCK;
	if(fcntl(servSock, F_SETFL, option) < 0)
		DieWithError("set socket option  failed");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));	/* Zero out structure */
	echoServAddr.sin_family = AF_INET;	/* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* Any incoming interface */
	echoServAddr.sin_port = htons(echoServPort);	/* Local port */

	/* Bind to the local address */
	if(bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) <
		0)
		DieWithError("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if(listen(servSock, MAXPENDING) < 0)
		DieWithError("listen() failed");

	/* Set the size of the in-out parameter */
	clntLen = sizeof(echoClntAddr);

	for(;;)							  /* Run forever */
	{
		if(connect_flag == 0)
		{
			/* Wait for a client to connect */
			if((clntSock =
				 accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
			{
				if(errno != EAGAIN)
				{
					DieWithError("accept() failed");
				}
			}
			else
			{
				/* clntSock is connected to a client! */
				printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

				connect_flag = 1;
			}

			if(connect_flag == 1)
			{
				HandleTCPClient(clntSock);
			}
			else
			{
				printf("not connected\n");
			}
			sleep(1);
		}
	}
	/* NOT REACHED */
}

/************************************************************************
**                                                                     **
**                              ALCATEL                                **
**                                                                     **
************************************************************************/  
	
/************************ COPYRIGHT INFORMATION *************************
**                                                                     **
** This program contains proprietary information which is a trade      **
** secret of ALCATEL and also is protected as an unpublished           **
** work under applicable Copyright laws. Recipient is to retain this   **
** program in confidence and is not permitted to use or make copies    **
** thereof other than as permitted in a written agreement with ALCATEL.**
**                                                                     **
************************************************************************/ 
	
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
	
#define damaServPort 5555		  // Platine interface
#define servIp "127.0.0.1"		  // Platine interface
int main(int argc, char *argv[]) 
{
	struct sockaddr_in damaServAddr;	/* DAMA server address */
	int damaSocket;				  // Platine interface
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



/**********************************************************************
**
** Margouilla Runtime Library
**
**
** Copyright (C) 2002-2003 CQ-Software.  All rights reserved.
**
**
** This file is distributed under the terms of the GNU Library 
** General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.
**
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.margouilla.com
**********************************************************************/
/* $Id: mgl_socket.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


#ifndef MGL_SOCKET_H
#define MGL_SOCKET_H


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
//#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#define DWORD	long
#endif

#include <stdio.h>
//#include "mgl_type.h"


// Provide platform independent IPv4, IPv6 socket functions
class mgl_socket {
	//
	// Basic socket functions
	//
public:
	virtual ~mgl_socket();
	static void initSocket();
	static void cleanupSocket();
	static int socket(int i_af, int i_type, int i_protocol);
	static struct hostent *gethostbyname(char *ip_servername);

#ifdef USE_IPV6
	static struct hostent *getaddrinfo(char *ip_servername);
#endif

	static int connect(int i_fd, struct sockaddr *ip_addr, int i_addrlen);
	static int select(int i_fdmax, fd_set *iop_readfdset, fd_set *iop_writefdset, fd_set *iop_exeptionfdset, struct timeval *ip_tv);
	static int send(int i_fd, char *ip_buf, int i_len, int i_flags);
	static int recv(int i_fd, char *op_buf, int i_buflen, int i_flags);
	static int bind (int i_s, struct sockaddr *ip_addr,  int i_addrlen);
	static void close(int i_fd);
	virtual int getFd() { return 0; }
	void setHugeBuffer() { if (getFd()) { socketSetHugeBuffer(getFd()); } };

	//
	// Advanced socket functions
	//
	static int getAvailableDataSize(int i_fd, int i_timer_s, int i_timer_us);
	static void socketSetNoDelay(int i_fd);
	static void socketSetNonBlocking(int i_fd);
	static int socketSetHugeBuffer(int i_fd);
};

///////////////////////////////////////////////////////////////////
///
///
///
///
///
///

// Provide platform independent TCP over IPv4, IPv6 socket functions
class mgl_socket_tcp_server: public mgl_socket {
public:
	int  _nbMaxClient;
	int	 _FD;			// Listen socket
	int	 _fd[128];		// Client connexion sockets
	int  _port;

	mgl_socket_tcp_server();
	virtual ~mgl_socket_tcp_server();

	int openSocket(int &io_port);
	int isOpened();
	int getFreeSockNum();
	int getClientCount();
	void acceptNewConnection(int i_timer_s, int i_timer_us);
	int rcv_buf(char *op_buf, int i_buflen, int i_flags, int i_timer_s, int i_timer_us);
	int  snd_buf(int i_client_num, char *ip_buf, int i_buflen);
	int init(int i_port);
	void close();
	virtual int  getFd() { return 0; }
};



///////////////////////////////////////////////////////////////////
///
///
///
///
///
///

// Provide platform independent TCP over IPv4, IPv6 socket functions
class mgl_socket_tcp_client: public mgl_socket 
{
public:
	int _fd;
	char _server_name[256];
	unsigned short _server_port;

	mgl_socket_tcp_client();
	~mgl_socket_tcp_client();

	int connectToServer();
	int init(char *ip_server_name, int i_port);
	int snd_buf(char *ip_buf, int i_buflen);
	int rcv_buf(char *op_buf, int i_buflen, int i_flags, int i_timer_s, int i_timer_us);
	void close();
	virtual int  getFd() { return _fd; };
};



// Provide platform independent TCP link 
class mgl_link_tcp_client: public mgl_socket_tcp_client {
public:
	// Manage partial sends of buffers
	int snd_pkt(char *ip_pkt, int i_pktlen);
	// Manage partial receive of buffers
	int rcv_pkt(char *op_pkt, int *op_pktlen, int i_timer_s, int i_timer_us);
	int trace(const char *ip_format,...);

};


class mgl_link_tcp_server: public mgl_socket_tcp_server 
{
public:
	// Manage partial sends of buffers
	int snd_pkt(int i_client_num, char *ip_pkt, int i_pktlen);
	// Manage partial receive of buffers
	int rcv_pkt(char *op_pkt, int *iop_pktlen, int i_timer_s, int i_timer_us);
	static int rcv_pkt_fd(int i_fd, char *op_pkt, int *iop_pktlen, int i_timer_s, int i_timer_us);
	int trace(int i_client_num, const char *ip_format,...);
};

class mgl_multicast_channel: public mgl_socket 
{
public:
	int _fd;
	unsigned short _port;
	struct sockaddr_in stTo;


	mgl_multicast_channel();
	~mgl_multicast_channel();
	int openSocket(const char *ip_addr, int i_port, int i_ttl);
	int snd_buf(char *ip_buf, int i_buflen);
	int rcv_buf(char *op_buf, int i_buflen, int i_timer_s, int i_timer_us);
	static int rcv_buf_fd(int i_fd, char *op_buf, int i_buflen, int i_timer_s, int i_timer_us);
	void close();
	virtual int  getFd() { return _fd; }
};

int mgl_socket_select_fd(long i_delay, int i_fd1=0, int i_fd2=0, int i_fd3=0, int i_fd4=0);
int mgl_socket_select(long i_delay, mgl_socket *ip_s1=NULL, mgl_socket *ip_s2=NULL, mgl_socket *ip_s3=NULL, mgl_socket *ip_s4=NULL);


#endif


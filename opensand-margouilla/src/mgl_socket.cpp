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
/* $Id: mgl_socket.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


#include <stdarg.h>
#ifndef WIN32
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define DWORD   long
#endif

#ifdef __linux__
#include <asm/ioctls.h>     // Linux                                             
#endif


#include "mgl_socket.h"
#include "mgl_debug.h"


mgl_socket::~mgl_socket()
{
}

// Provide platform independent IPv4, IPv6 socket functions
void mgl_socket::initSocket()
{
#ifdef WIN32
	/* Init des winsock*/
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 0 ); 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we couldn't find a usable */
		/* WinSock DLL.                                  */
		return;
	}
#endif
}

void mgl_socket::cleanupSocket()
{
#ifdef WIN32
  WSACleanup();
#endif
}



// socket (IPv4, IPv6)
int mgl_socket::socket(int i_af, int i_type, int i_protocol)

{
	return ::socket(i_af, i_type, i_protocol);
}

// gethostbyname (IPv4)
struct hostent *mgl_socket::gethostbyname(char *ip_servername) 
{
	return ::gethostbyname(ip_servername);
}

#ifdef USE_IPV6
// getaddrinfo(IPv4, IPv6)
struct hostent *mgl_socket::getaddrinfo(char *ip_servername) 
{
	return ::gethostbyname(ip_servername);
}
#endif

// connect
int mgl_socket::connect(int i_fd, struct sockaddr *ip_addr, int i_addrlen)
{
	return ::connect(i_fd, ip_addr, i_addrlen);
}

// select
int mgl_socket::select(int i_fdmax, fd_set *iop_readfdset, fd_set *iop_writefdset, fd_set *iop_exeptionfdset, struct timeval *ip_tv)
{
	return ::select(i_fdmax+1, iop_readfdset, iop_writefdset, iop_exeptionfdset, ip_tv);
}

// send
int mgl_socket::send(int i_fd, char *ip_buf, int i_len, int i_flags)
{
	return ::send(i_fd, ip_buf, i_len, i_flags);
}

// receive
int mgl_socket::recv(int i_fd, char *op_buf, int i_buflen, int i_flags)
{ 
	return ::recv(i_fd, op_buf, i_buflen, i_flags);
}

// bind
int mgl_socket::bind (int i_s, struct sockaddr *ip_addr,  int i_addrlen)
{
	return ::bind (i_s, ip_addr, i_addrlen);
}

// close
void mgl_socket::close(int i_fd)
{
#ifdef WIN32
	::closesocket(i_fd);
#else
	::close(i_fd);
#endif
}

//
// Advanced socket functions
//
int mgl_socket::getAvailableDataSize(int i_fd, int i_timer_s, int i_timer_us)
{
	fd_set fdset;
	struct timeval l_tv;
	int l_ret;
	int l_available;

	FD_ZERO(&fdset);
	FD_SET((unsigned int)i_fd, &fdset);
	l_tv.tv_sec = i_timer_s;
	l_tv.tv_usec = i_timer_us;
	l_ret = select(i_fd+1, &fdset, NULL, NULL, &l_tv);
	if (  l_ret< 0 )  { return -1; }
	if (  l_ret== 0 ) { return 0; }

	if ( FD_ISSET(i_fd, &fdset) ) {
		/* Get available data size */
#ifdef WIN32
		ioctlsocket( i_fd, FIONREAD, (unsigned long *)&l_available );
#else
		ioctl( i_fd, FIONREAD, &l_available );
#endif
			return l_available;
	}
	return 0;

}
/*****************************
/* P_socketSetNoDelay
/* 
/* Disable the Nagle algorithme.
/* TCP buffer are directly sent, instead of
/* beeing bufferized
/*****************************/
void mgl_socket::socketSetNoDelay(int i_fd)
{
#ifdef WIN32
	int	l_val;
	int l_ret;
	struct linger l_linger;

	return;
	l_val = 1;
	l_linger.l_onoff = 1;
	l_linger.l_linger= 0;

	/* Disable Nagle */
	if (setsockopt ( i_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&l_val, sizeof(int) )) {
		MGL_WARNING(MGL_CTX, "Socket buffering not desactivated\n");
	}
	 
	/* Allow fast close */
	if (setsockopt ( i_fd, SOL_SOCKET , SO_LINGER, (const char *)&l_linger, sizeof(struct linger) )) {
		MGL_WARNING(MGL_CTX, "SO_LINGER not activated\n");
		l_ret=WSAGetLastError ();
		switch (l_ret){

 		case WSANOTINITIALISED:
			MGL_WARNING(MGL_CTX, "A successful WSAStartup must occur before using this function. \n");
			break;
 		case WSAENETDOWN:
			MGL_WARNING(MGL_CTX, "The network subsystem has failed. \n");
			break;
 		case WSAEFAULT:
			MGL_WARNING(MGL_CTX, "optval is not in a valid part of the process address space or optlen parameter is too small. \n");
			break;
 		case WSAEINPROGRESS:
			MGL_WARNING(MGL_CTX, "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n");
			break;
		case WSAEINVAL:
			MGL_WARNING(MGL_CTX, "level is not valid, or the information in optval is not valid.\n");
			break;
		case WSAENETRESET:
			MGL_WARNING(MGL_CTX, "Connection has timed out when SO_KEEPALIVE is set.\n");
			break;
		case WSAENOPROTOOPT:

			MGL_WARNING(MGL_CTX, "The option is unknown or unsupported for the specified provider.\n");
			break;
		case WSAENOTCONN:
			MGL_WARNING(MGL_CTX, "Connection has been reset when SO_KEEPALIVE is set.\n");
			break;
		case WSAENOTSOCK:
			MGL_WARNING(MGL_CTX, "The descriptor is not a socket.\n");
			break;
		default: 
			break;
		}
	}

#else
	return;
	fcntl((int) i_fd, F_SETFL, O_NDELAY);	 
#endif
}

/*****************************
/* P_socketSetNonBlocking
/* 
/* Socket is put in non-blocking mode
/* Tips: Use select instead of this...
/*****************************/
void mgl_socket::socketSetNonBlocking(int i_fd)
{
#ifdef WIN32
	/* Passage en mode non bloquant */
	unsigned long l_param;

	l_param = 1;
	ioctlsocket (i_fd, FIONBIO ,&l_param ); 

#else
	fcntl((int) i_fd,F_SETFL,O_NONBLOCK);	
#endif
}


/*****************************
/* P_socketSetHugeBuffer
/* 
/* Set socket buffer size
/*****************************/
int mgl_socket::socketSetHugeBuffer(int i_fd)
{
	// Info var
	int l_optval;
#ifdef WIN32
	int l_optlen;
#else
	socklen_t l_optlen;
#endif
	int l_ret;

//#ifdef WIN32
	// set
	l_optval = 1024000;
#ifndef WIN32
	l_optval = 256000;
#endif
	l_optlen = sizeof(l_optval);
	l_ret = setsockopt ( i_fd,  SOL_SOCKET, SO_SNDBUF , (const char *)&l_optval, l_optlen ); 
	if (!l_ret) {
		//MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Send buffer size : %d\n", l_optval);
	} 

	// set
	l_optval = 1024000;
#ifndef WIN32
	l_optval = 256000;
#endif
	l_optlen = sizeof(l_optval);
	l_ret = setsockopt ( i_fd,  SOL_SOCKET, SO_RCVBUF , (const char *)&l_optval, l_optlen ); 
	if (!l_ret) {
		//MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Receive buffer size : %d\n", l_optval);
	}
	// informations
	l_optlen = sizeof(l_optval);
	l_ret = getsockopt ( i_fd,  SOL_SOCKET, SO_SNDBUF , (char *)&l_optval, &l_optlen ); 
	if (!l_ret) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Send buffer size : %d\n", l_optval);
	}

	// informations
	l_optlen = sizeof(l_optval);
	l_ret = getsockopt ( i_fd,  SOL_SOCKET, SO_RCVBUF , (char *)&l_optval, &l_optlen ); 
	if (!l_ret) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Receive buffer size : %d\n", l_optval);
	}
//#else
	
//#endif
	return 0;
}

///////////////////////////////////////////////////////////////////
///
///
///
///
///
///

// Provide platform independent TCP over IPv4, IPv6 socket functions

mgl_socket_tcp_server::mgl_socket_tcp_server() {
	int l_nb;
	int l_cpt;

	_FD=0;
	_port=0;
	l_nb = 128;
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		_fd[l_cpt]=0;
	}
	_nbMaxClient=15;
}

mgl_socket_tcp_server::~mgl_socket_tcp_server() {
	close();
}

int mgl_socket_tcp_server::openSocket(int &io_port)
{
#ifndef USE_IPV6
	// IPv4 only code
	struct sockaddr_in 	lv_addr_in;	/* Adresse local AF_INET */
	DWORD   dwValue=5;
#ifdef WIN32
	int l_len;
#else
	socklen_t l_len;
#endif

	/* socket creation*/
	if ((_FD = socket(AF_INET, SOCK_STREAM,0)) < 0) {
	  return -1;
	}

	/* local adresse building */
	lv_addr_in.sin_family = AF_INET;
	lv_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
	lv_addr_in.sin_port	= htons((unsigned short)io_port);

	/* System link */
	if (bind((int)_FD,
		 (struct sockaddr *)&lv_addr_in,
		 (int)sizeof(struct sockaddr_in)) < 0) {
	  return -1;
	} /* bind */


	/* Listen to the socket & set client queue size */
	if (listen((int)_FD, _nbMaxClient) < 0) {
		MGL_WARNING(MGL_CTX, "Listen Pb _FD(%d)\n", _FD);
		return -1;
	} /* listen */

	if (io_port==0) {
		l_len = sizeof(struct sockaddr_in);
		if (getsockname(_FD, (struct sockaddr *)&lv_addr_in, &l_len) ==0) {
			io_port= ntohs(lv_addr_in.sin_port);
		}
	}
	return _FD;
#else

#endif
}

int mgl_socket_tcp_server::isOpened()
{
	if (_FD) {
		return mgl_true;
	} else {
		return mgl_false;
	}
}

int mgl_socket_tcp_server::getFreeSockNum()
{
	int l_cpt;

	/* The 0 stay unused */
	for (l_cpt=1; l_cpt<_nbMaxClient; l_cpt++) {
		if (_fd[l_cpt]==0) { 
			return l_cpt;
		}
	}
	return -1;
}

int mgl_socket_tcp_server::getClientCount()
{
	int l_nb;
	int l_cpt;

	/* The 0 stay unused */
	l_nb=0;
	for (l_cpt=1; l_cpt<_nbMaxClient; l_cpt++) {
		if (_fd[l_cpt]!=0) { 
			l_nb++;
		}
	}
	return l_nb;
}

void mgl_socket_tcp_server::acceptNewConnection(int i_timer_s, int i_timer_us)
{
	/* Accept new connections */
#ifdef WIN32
	int		lv_addrsize;	/* Dummy */
#else
	socklen_t lv_addrsize;
#endif
	struct		sockaddr_in	lv_addr;	/* Dummy (unused en AF_UNIX) */
	int			lv_fd;
	int			l_numSock;
	fd_set fdset;
	struct timeval l_tv;
	int l_ret;

	lv_addrsize = sizeof(struct sockaddr_in);
	/* Some free room ? */
	l_numSock = getFreeSockNum();
	if (l_numSock==-1) {
		return;
	}

	FD_ZERO(&fdset);
	FD_SET((unsigned int)_FD, &fdset);
	l_tv.tv_sec = i_timer_s;
	l_tv.tv_usec = i_timer_us;
	l_ret = select(_FD+1, &fdset, NULL, NULL, &l_tv);
	//MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "LL_AcceptNewConnection select=%d\n", l_ret);
	if (  l_ret< 0 ) { return; }
	if (  l_ret== 0 ) { return; }

	if ( FD_ISSET(_FD, &fdset) ) {
		lv_fd = accept((int)_FD, (struct sockaddr *)&lv_addr, &lv_addrsize);
		if (lv_fd > 0) {
			/* On ajoute la connexion dans la liste */
			//P_socketSetNoDelay(lv_fd);
			if (_fd[l_numSock]) { mgl_socket::close(_fd[l_numSock]); } 
			_fd[l_numSock] = lv_fd;
			MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "New connexion accepted\n");
		}
	}
}

int mgl_socket_tcp_server::rcv_buf(char *op_buf, int i_buflen, int i_flags, int i_timer_s, int i_timer_us)
{
	int l_len;
	int l_sock;
	fd_set fdset;
	struct timeval l_tv;
	int l_maxfd;
	int l_ret;
	
	acceptNewConnection(0, 0);

	// Test fd
	FD_ZERO(&fdset);
	l_maxfd =0;
	for (l_sock=0; l_sock<_nbMaxClient; l_sock++) {
		if (_fd[l_sock]) {
			FD_SET((unsigned int)_fd[l_sock], &fdset);
			if (_fd[l_sock]>l_maxfd) {
				l_maxfd = _fd[l_sock];
			}
		}
	}
	l_tv.tv_sec = i_timer_s;
	l_tv.tv_usec = i_timer_us;
	l_ret = select(l_maxfd+1, &fdset, NULL, NULL, &l_tv);
	if (  l_ret< 0 ) {
#ifdef DEBUGWIN32
		l_ret = WSAGetLastError();
		MGL_WARNING(MGL_CTX, "WSAGetLastError()==%d\n", l_ret);
		if (l_ret==WSAEWOULDBLOCK) {
			MGL_WARNING(MGL_CTX, "WSAEWOULDBLOCK.\n");
		}
		if (l_ret==WSANOTINITIALISED) {
			MGL_WARNING(MGL_CTX, "WSANOTINITIALISED.\n");
		}
		if (l_ret==WSAEFAULT) {
			MGL_WARNING(MGL_CTX, "WSAEFAULT.\n");
		}
		if (l_ret==WSAENETDOWN) {
			MGL_WARNING(MGL_CTX, "WSAENETDOWN.\n");
		}
		if (l_ret==WSAEINVAL) {
			MGL_WARNING(MGL_CTX, "WSAEINVAL.\n");
		}
		if (l_ret==WSAEINTR) {
			MGL_WARNING(MGL_CTX, "WSAEINTR.\n");
		}
		if (l_ret==WSAENOTSOCK) {
			MGL_WARNING(MGL_CTX, "WSAENOTSOCK.\n");
		}
		if (l_ret==WSAEINPROGRESS) {
			MGL_WARNING(MGL_CTX, "WSAEINPROGRESS.\n");
		}
#endif
		return 0;
	}
	if (  l_ret== 0 )
		return 0;
		
	for (l_sock=0; l_sock<_nbMaxClient; l_sock++) {
		if (_fd[l_sock]) {
			if ( FD_ISSET(_fd[l_sock], &fdset) ) {	
				l_len = recv(_fd[l_sock], (char *)op_buf, i_buflen, i_flags);  

				if (l_len==0) {   /* deconnection */
					mgl_socket::close(_fd[l_sock]);
					_fd[l_sock] = 0;
					MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "distant deconnected cx(%d)\n", l_sock);
				}
				if (l_len<0)  {   /* nothing to read */
				}
				if (l_len>0) {    /* received something */
					MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received Msg [%d octets] by socket[%d] : ",l_len, l_sock);
					return 1;
				}
			}
		}
	}
	return 0;
}

int  mgl_socket_tcp_server::snd_buf(int i_client_num, char *ip_buf, int i_buflen)
{
	int l_sock;
	int l_ret;

	l_ret=0;
	l_sock = _fd[i_client_num];
	if (l_sock<=0) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Dest (%d) not connected\n", i_client_num);
	} else {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "sending buffer [%d bytes] to client num %d\n", i_buflen, i_client_num);
		l_ret = send(l_sock, ip_buf, i_buflen, 0);
		if (l_ret!=i_buflen) { 
			MGL_WARNING(MGL_CTX, "Pb, buffer partial sent\n"); }
	} 

	return l_ret;
}

int mgl_socket_tcp_server::init(int i_port)
{
	int l_cpt;
	int l_ret;

	/* init */
	initSocket();
	_FD=0;
	for (l_cpt=0; l_cpt<_nbMaxClient; l_cpt++) {
		_fd[l_cpt]=0;
	}

	l_ret = openSocket(i_port);
	if (l_ret) {
		_port = i_port;
		socketSetHugeBuffer(l_ret);
	} else {
		_port = 0;
	}
	return l_ret;
}


void mgl_socket_tcp_server::close()
{
	int l_cpt;
	if (_FD) { mgl_socket::close(_FD); _FD=0; }
	for (l_cpt=0; l_cpt<_nbMaxClient; l_cpt++) {
		if (_fd[l_cpt]) { 
			mgl_socket::close(_fd[l_cpt]); 
			_fd[l_cpt]=0;
		}
	}

}




///////////////////////////////////////////////////////////////////
///
///
///
///
///
///

// Provide platform independent TCP over IPv4, IPv6 socket functions

mgl_socket_tcp_client::mgl_socket_tcp_client()
{ 
	_fd=0;
}

mgl_socket_tcp_client::~mgl_socket_tcp_client()
{
	if (_fd) { mgl_socket::close(_fd); }
}

int mgl_socket_tcp_client::connectToServer()
{
	struct	sockaddr_in 	lv_addr_in;	/* Adresse local AF_INET */
	DWORD   dwValue=5;
	struct	sockaddr *	lv_addr;	/* Adresse local */
	int		l_len;	/* pour gethostbyname */
	struct hostent *phe;

	initSocket();
	/* creation de la socket */
	if ((_fd = socket(AF_INET, SOCK_STREAM,0)) < 0) {
		_fd = 0;
		return 0;
	}

	/* Connection au serveur */
	phe = gethostbyname(_server_name);
	if (phe == NULL) {
	 return 0;
	}
	memcpy((char *)&(lv_addr_in.sin_addr), phe->h_addr, phe->h_length);

	lv_addr_in.sin_family = AF_INET;
	lv_addr_in.sin_port = htons(_server_port);

	lv_addr = (struct sockaddr *) (&lv_addr_in);
	l_len = sizeof(struct sockaddr_in);
	if (connect((int)_fd, (struct sockaddr *)lv_addr, l_len) < 0) {
		_fd = 0;
		return 0;
	}
	/* Disable buffering */
	//socketSetNoDelay(LL_fd_client); 


	return _fd;
}

int mgl_socket_tcp_client::init(char *ip_server_name, int i_port)
{
	int l_ret;

	strcpy(_server_name, ip_server_name);
	_server_port = i_port;
	l_ret = connectToServer();
	if (l_ret) {
		mgl_socket::socketSetHugeBuffer(l_ret);
	}
	return l_ret;
}

int mgl_socket_tcp_client::snd_buf(char *ip_buf, int i_buflen)
{
	int l_ret;

	l_ret=0;
	if (_fd<=0) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Not connected to server\n");
	} else {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "sending buffer [%d bytes] to server\n", i_buflen);
		l_ret = ::send(_fd, ip_buf, i_buflen, 0);
		if (l_ret!=i_buflen) { 
			MGL_WARNING(MGL_CTX, "Pb, buffer partial sent\n"); 
		}
	} 

	return l_ret;
}


int mgl_socket_tcp_client::rcv_buf(char *op_buf, int i_buflen, int i_flags, int i_timer_s, int i_timer_us)
{
	int l_len;
	fd_set fdset;
	struct timeval l_tv;
	int l_ret;

	if (_fd) {
		// Test fd
		FD_ZERO(&fdset);
		FD_SET((unsigned int)_fd, &fdset);
		l_tv.tv_sec = i_timer_s;
		l_tv.tv_usec = i_timer_us;
		l_ret = select(_fd+1, &fdset, NULL, NULL, &l_tv);
		if (  l_ret< 0 ) {
			return -1;
		}
		if (  l_ret==0 ) {
			return 0;
		}
		l_len = recv(_fd, op_buf, i_buflen, i_flags);

		if (l_len==0) {   /* deconnection */
			mgl_socket::close(_fd);
			_fd = 0;
			MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Server deconnected\n");
		}
		if (l_len<0)  {   /* socket non valide */
			/*closesocket(fd);
			fd = 0; */
		}
		if (l_len>0) {    /* normal */
			MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received datas [%d Byte] from server \n",l_len);
			return l_len;
		}
	}
	return 0;
}



void mgl_socket_tcp_client::close()
{
	if (_fd) { mgl_socket::close(_fd); }
}




// Provide platform independent TCP link 
// Manage partial sends of buffers
int mgl_link_tcp_client::snd_pkt(char *ip_pkt, int i_pktlen)
{
	long l_len;
	long l_nb;

	// Send length
	l_len = htonl(i_pktlen);
	l_nb = snd_buf((char *)&l_len, 4);
	if (l_nb!=4) {
		MGL_WARNING(MGL_CTX, "Pb, partial send (l_nb=%d/4)!!\n", l_nb);
		return 0;
	}

	// Send datas
	l_nb = snd_buf(ip_pkt, i_pktlen);
	if (l_nb!=i_pktlen) {
		MGL_WARNING(MGL_CTX, "Pb, partial send (l_nb=%d/%d)!!\n", l_nb, i_pktlen);
		return 0;
	}
	return 1;
}

int mgl_link_tcp_client::trace(const char *ip_format,...)
{
	char l_buf[1024]="";
	va_list l_va;
	long l_len;
	long l_ret;

	if (!_fd) { return 0; }
	va_start( l_va, ip_format );     
	l_len = vsprintf( l_buf, ip_format, l_va );
	va_end( l_va );              
	l_ret = snd_pkt(l_buf, l_len+1);
	return l_ret;
}


// Manage partial receive of buffers
int mgl_link_tcp_client::rcv_pkt(char *op_pkt, int *op_pktlen, int i_timer_s, int i_timer_us)
{
	int l_nb;
	long l_len;

	// Check data availablility for length (4 bytes)
	if (getAvailableDataSize(_fd, i_timer_s, i_timer_us)<4) {
		return 0;
	}

	// Get Length
	l_nb = rcv_buf((char *)&l_len, 4, MSG_PEEK, 0, 0); // The 4 bytes stay in queue
	if (l_nb!=4) {
		MGL_WARNING(MGL_CTX, "Pb, partial read !!\n");
		return 0;
	}
	l_len = ntohl(l_len);

	// The whole packet is available ?
	if (getAvailableDataSize(_fd, 0, 0)<l_len) {
		return 0;
	}

	// Get lenght
	l_nb = rcv_buf((char *)&l_len, 4, 0, 0, 0); 
	if (l_nb!=4) {
		MGL_WARNING(MGL_CTX, "Pb, partial read !! (Desynchronised !!!)\n");
		return 0;
	}

	// Get packet
	l_nb = rcv_buf(op_pkt, l_len, 0, 0, 0);
	if (l_nb!=l_len) {
		MGL_WARNING(MGL_CTX, "Pb, partial read !!\n");
	}
	(*op_pktlen) = l_len;
	return 1;
}



// Manage partial sends of buffers
int mgl_link_tcp_server::snd_pkt(int i_client_num, char *ip_pkt, int i_pktlen)
{
	long l_len;
	long l_nb;

	// Send length
	l_len = htonl(i_pktlen);
	l_nb = snd_buf(i_client_num, (char *)&l_len, 4);
	if (l_nb!=4) {
		MGL_WARNING(MGL_CTX, "Pb, partial send (l_nb=%d)!!\n", l_nb);
		return 0;
	}

	// Send datas
	l_nb = snd_buf(i_client_num, ip_pkt, i_pktlen);
	if (l_nb!=i_pktlen) {
		MGL_WARNING(MGL_CTX, "Pb, partial send (l_nb=%d)!!\n", l_nb);
		return 0;
	}
	return 1;
}

int mgl_link_tcp_server::rcv_pkt_fd(int i_fd, char *op_pkt, int *iop_pktlen, int i_timer_s, int i_timer_us)
{
	int l_nb;
	long l_len;


	// Check data availablility for length (4 bytes)
	if (getAvailableDataSize(i_fd, 0, 0)<4) {
		return 0;
	}

	// Get Length
	l_nb = recv(i_fd, (char *)&l_len, 4, MSG_PEEK);  // The 4 bytes stay in queue
	if (l_nb==0) {   /* deconnection */
		mgl_socket::close(i_fd);
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "distant deconnected cx.\n");
	}
	if (l_nb<0)  {   /* nothing to read */
	}
	if (l_nb>0) {    /* received something */
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received Length [%d octets] by socket[%d] (still in socket buffer)\n",l_nb, i_fd);
	}
	if (l_nb!=4) {
		MGL_WARNING(MGL_CTX, "Pb, partial read !!\n");
		return 0;
	}
	l_len = ntohl(l_len);

	// the buffer is enougth ?
	if ((*iop_pktlen)<l_len) {
		MGL_WARNING(MGL_CTX, "Buffer too small\n");
		return 0;
	}

	// The whole packet is available ?
	l_nb = getAvailableDataSize(i_fd, 0, 0);
	if (l_nb<l_len) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Not enough data in socket buffer (%d<%d)\n", l_nb, l_len);
		return 0;
	}

	// Get length
	l_nb = recv(i_fd, (char *)&l_len, 4, 0);  
	MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received Length [%d octets] by socket[%d]\n",l_nb, i_fd);
	if (l_nb!=4) {
		MGL_WARNING(MGL_CTX, "Pb, partial read !! (Desynchronised !!!)\n");
		return 0;
	}
	l_len = ntohl(l_len);

	// Get packet
	l_nb = recv(i_fd, op_pkt, l_len, 0);  
	MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received Data [%d octets] by socket[%d]\n",l_nb, i_fd);
	if (l_nb!=l_len) {
		MGL_WARNING(MGL_CTX, "Pb, partial read !!\n");
	}
	(*iop_pktlen) = l_len;
	return 1;

}


// Manage partial receive of buffers
int mgl_link_tcp_server::rcv_pkt(char *op_pkt, int *iop_pktlen, int i_timer_s, int i_timer_us)
{
	int l_nb;
	long l_len;
	int l_sock;
	fd_set fdset;
	struct timeval l_tv;
	int l_maxfd;
	int l_ret;


	// Test fd
	FD_ZERO(&fdset);
	l_maxfd =0;
	for (l_sock=0; l_sock<_nbMaxClient; l_sock++) {
		if (_fd[l_sock]) {
			FD_SET((unsigned int)_fd[l_sock], &fdset);
			if (_fd[l_sock]>l_maxfd) {
				l_maxfd = _fd[l_sock];
			}
		}
	}
	l_tv.tv_sec = i_timer_s;
	l_tv.tv_usec = i_timer_us;
	l_ret = select(l_maxfd+1, &fdset, NULL, NULL, &l_tv);
	if (  l_ret< 0 ) {
#ifdef DEBUGWIN32
		l_ret = WSAGetLastError();
		MGL_WARNING(MGL_CTX, "WSAGetLastError()==%d\n", l_ret);
		if (l_ret==WSAEWOULDBLOCK) {
			MGL_WARNING(MGL_CTX, "WSAEWOULDBLOCK.\n");
		}
		if (l_ret==WSANOTINITIALISED) {
			MGL_WARNING(MGL_CTX, "WSANOTINITIALISED.\n");
		}
		if (l_ret==WSAEFAULT) {
			MGL_WARNING(MGL_CTX, "WSAEFAULT.\n");
		}
		if (l_ret==WSAENETDOWN) {
			MGL_WARNING(MGL_CTX, "WSAENETDOWN.\n");
		}
		if (l_ret==WSAEINVAL) {
			MGL_WARNING(MGL_CTX, "WSAEINVAL.\n");
		}
		if (l_ret==WSAEINTR) {
			MGL_WARNING(MGL_CTX, "WSAEINTR.\n");
		}

		if (l_ret==WSAENOTSOCK) {
			MGL_WARNING(MGL_CTX, "WSAENOTSOCK.\n");
		}
		if (l_ret==WSAEINPROGRESS) {
			MGL_WARNING(MGL_CTX, "WSAEINPROGRESS.\n");
		}
#endif
		return 0;
	}
	if (  l_ret== 0 )
		return 0;
		
	for (l_sock=0; l_sock<_nbMaxClient; l_sock++) {
		if (_fd[l_sock]) {
			if ( FD_ISSET(_fd[l_sock], &fdset) ) {	

				// Check data availablility for length (4 bytes)
				l_nb = getAvailableDataSize(_fd[l_sock], 0, 0);
				if (l_nb==0) {
					// Client deconnection
					mgl_socket::close(_fd[l_sock]);
					_fd[l_sock] = 0;
					MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "distant deconnected cx(%d)\n", l_sock);
				}
				if (l_nb<4) {
					return 0;
				}

				// Get Length
				l_nb = recv(_fd[l_sock], (char *)&l_len, 4, MSG_PEEK);  // The 4 bytes stay in queue
				if (l_nb==0) {   /* deconnection */
					mgl_socket::close(_fd[l_sock]);
					_fd[l_sock] = 0;
					MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "distant deconnected cx(%d)\n", l_sock);
				}
				if (l_nb<0)  {   /* nothing to read */
				}
				if (l_nb>0) {    /* received something */
					MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received Length [%d octets] by socket[%d] (still in socket buffer)\n",l_nb, l_sock);
				}
				if (l_nb!=4) {
					MGL_WARNING(MGL_CTX, "Pb, partial read !!\n");
					return 0;
				}
				l_len = ntohl(l_len);

				// the buffer is enougth ?
				if ((*iop_pktlen)<l_len) {
					MGL_WARNING(MGL_CTX, "Buffer too small\n");
					return 0;
				}

				// The whole packet is available ?
				l_nb = getAvailableDataSize(_fd[l_sock], 0, 0);
				if (l_nb<l_len) {
					MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Not enough data in socket buffer (%d<%d)\n", l_nb, l_len);
					return 0;
				}

				// Get length
				l_nb = recv(_fd[l_sock], (char *)&l_len, 4, 0);  
				MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received Length [%d octets] by socket[%d]\n",l_nb, l_sock);
				if (l_nb!=4) {
					MGL_WARNING(MGL_CTX, "Pb, partial read !! (Desynchronised !!!)\n");
					return 0;
				}
				l_len = ntohl(l_len);

				// Get packet
				l_nb = recv(_fd[l_sock], op_pkt, l_len, 0);  
				MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Received Data [%d octets] by socket[%d]\n",l_nb, l_sock);
				if (l_nb!=l_len) {
					MGL_WARNING(MGL_CTX, "Pb, partial read !!\n");
				}
				(*iop_pktlen) = l_len;
				return 1;


			}
		}
	}
	return 0;
}

int mgl_link_tcp_server::trace(int i_client_num, const char *ip_format,...)
{
	char l_buf[1024]="";
	va_list l_va;
	long l_len;
	long l_ret;
	
	if (!_fd[i_client_num]) { return 0; }
	va_start( l_va, ip_format );     
	l_len = vsprintf( l_buf, ip_format, l_va );
	va_end( l_va );              
	l_ret = snd_pkt(i_client_num, l_buf, l_len+1);
	return l_ret;
}

mgl_multicast_channel::mgl_multicast_channel()
{
	_fd=0;;
	_port=0;
}

mgl_multicast_channel::~mgl_multicast_channel()
{
	close();
}


int getErrorNum()
{
#ifdef WIN32  
return WSAGetLastError();
#else
return errno;
#endif


}

int mgl_multicast_channel::openSocket(const char *ip_addr, int i_port, int i_ttl)
{
	struct sockaddr_in stLocal;
	struct ip_mreq stMreq;
	int iTmp, iRet;
	int addr_size = sizeof(struct sockaddr_in);

	#ifdef WIN32  
	/* Init WinSock */
	WSADATA stWSAData;
	iTmp = WSAStartup(0x0202, &stWSAData);
	if (iTmp) {
	MGL_WARNING(MGL_CTX, "WSAStartup failed: Err: %d\r\n", iTmp);
	exit (1);
	}
	#endif

	/* get a datagram socket */
	_fd = socket(AF_INET, 
	 SOCK_DGRAM, 
	 0);
	if (_fd <0) {
		MGL_WARNING(MGL_CTX, "socket() failed, Err: %d\n", getErrorNum());
		return 0;
	}

	/* avoid EADDRINUSE error on bind() */ 
	iTmp = 1;
	iRet = setsockopt(_fd, 
	 SOL_SOCKET, 
	 SO_REUSEADDR, 
	 (char *)&iTmp, 
	 sizeof(iTmp));
	if (iRet <0) {
		MGL_WARNING(MGL_CTX, "setsockopt() SO_REUSEADDR failed, Err: %d\n", getErrorNum());
	}

	/* name the socket */
	stLocal.sin_family = 	 AF_INET;
	stLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	stLocal.sin_port = 	 htons(i_port);
	iRet = bind(_fd, (struct sockaddr*) &stLocal, sizeof(stLocal));
	if (iRet <0) {

		MGL_WARNING(MGL_CTX, "bind() failed, Err: %d\n",	  getErrorNum());
	}

	/* join the multicast group. */
	stMreq.imr_multiaddr.s_addr = inet_addr(ip_addr);
	stMreq.imr_interface.s_addr = INADDR_ANY; //inet_addr("172.16.43.148"); //INADDR_ANY;
	iRet = setsockopt(_fd, 
	 IPPROTO_IP, 
	 IP_ADD_MEMBERSHIP, 
	 (char *)&stMreq, 
	 sizeof(stMreq));
	if (iRet <0) {
		MGL_WARNING(MGL_CTX, "setsockopt() IP_ADD_MEMBERSHIP failed, Err: %d\n",	  getErrorNum());
    #ifdef WIN32
    #else
    printf("%s\n", strerror(getErrorNum()));
    #endif
	} 

	/* set TTL to traverse up to multiple routers */
	iTmp = i_ttl;
	iRet = setsockopt(_fd, 
	 IPPROTO_IP, 
	 IP_MULTICAST_TTL, 
	 (char *)&iTmp, 
	 sizeof(iTmp));
	if (iRet <0) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "setsockopt() IP_MULTICAST_TTL failed, Err: %d\n",	  getErrorNum());
	}

	/* disable loopback */
	/*
	iTmp = 0;
	iRet = setsockopt(_fd, 
	 IPPROTO_IP, 
	 IP_MULTICAST_LOOP, 
	 (char *)&iTmp, 
	 sizeof(iTmp));
	if (iRet<0) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "setsockopt() IP_MULTICAST_LOOP failed, Err: %d\n",	  getErrorNum());
	}
	*/

	/* assign our destination address */
	stTo.sin_family =      AF_INET;
	stTo.sin_addr.s_addr = inet_addr(ip_addr);
	stTo.sin_port =        htons(i_port);

	MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "Now sending to (and receiving from) multicast group: %s\n", ip_addr);
	return _fd;
}

int mgl_multicast_channel::snd_buf(char *ip_buf, int i_buflen)
{
	int l_ret;
	int addr_size = sizeof(struct sockaddr_in);

	/* send to the multicast address */
	l_ret = sendto(_fd, 
	  ip_buf, 
	  i_buflen, 
	  0,
	  (struct sockaddr*)&stTo, 
	  addr_size);
	if (l_ret < 0) {
		MGL_WARNING(MGL_CTX, "sendto() failed, Error: %d\n", getErrorNum());
	}
	return l_ret;
}

int mgl_multicast_channel::rcv_buf(char *op_buf, int i_buflen, int i_timer_s, int i_timer_us)
{
	return rcv_buf_fd(_fd, op_buf, i_buflen, i_timer_s, i_timer_us);
}

int mgl_multicast_channel::rcv_buf_fd(int i_fd, char *op_buf, int i_buflen, int i_timer_s, int i_timer_us)
{
	int l_ret;
	struct sockaddr_in stFrom;
	int addr_size = sizeof(struct sockaddr_in);

	// Datas ?
	l_ret = getAvailableDataSize(i_fd, i_timer_s, i_timer_us);
	if (l_ret <=0) {
		return l_ret;
	}

	// Get data
    l_ret = recvfrom(i_fd, 
      op_buf, 
      (i_buflen), 
      0,
      (struct sockaddr*)&stFrom, 
#ifdef WIN32
      &addr_size);
#else
      (socklen_t *)&addr_size);
#endif
    if (l_ret < 0) {
		MGL_WARNING(MGL_CTX, "recvfrom() failed, Error: %d\n", getErrorNum());
    }
    MGL_TRACE(MGL_CTX, MGL_TRACE_SOCKET, "From host:%s port:%d, %s\n",
      inet_ntoa(stFrom.sin_addr), 
      ntohs(stFrom.sin_port), op_buf);
	return l_ret;
}



void mgl_multicast_channel::close()
{
	if (_fd) mgl_socket::close(_fd);
}



int mgl_socket_select(long i_delay, mgl_socket *ip_s1, mgl_socket *ip_s2, mgl_socket *ip_s3, mgl_socket *ip_s4)
{
	fd_set fdset;
	struct timeval l_tv;
	int l_maxfd;
	int l_ret;

	// Test fd
	FD_ZERO(&fdset);
	l_maxfd =0;

	// socket 1
	if (ip_s1) {
		FD_SET((unsigned int)ip_s1->getFd(), &fdset);
		if (ip_s1->getFd()>l_maxfd) {
			l_maxfd = ip_s1->getFd();
		}
	}
	// socket 2
	if (ip_s2) {
		FD_SET((unsigned int)ip_s2->getFd(), &fdset);
		if (ip_s2->getFd()>l_maxfd) {
			l_maxfd = ip_s2->getFd();
		}
	}
	// socket 3
	if (ip_s3) {
		FD_SET((unsigned int)ip_s3->getFd(), &fdset);
		if (ip_s3->getFd()>l_maxfd) {
			l_maxfd = ip_s3->getFd();
		}
	}
	// socket 4
	if (ip_s4) {
		FD_SET((unsigned int)ip_s4->getFd(), &fdset);
		if (ip_s4->getFd()>l_maxfd) {
			l_maxfd = ip_s4->getFd();
		}
	}

	l_tv.tv_sec = i_delay/1000;
	l_tv.tv_usec = (i_delay-((i_delay/1000)*1000))*1000;
	l_ret = select(l_maxfd+1, &fdset, NULL, NULL, &l_tv);
	if (  l_ret< 0 ) {
#ifdef DEBUGWIN32
		l_ret = WSAGetLastError();
		MGL_WARNING(MGL_CTX, "WSAGetLastError()==%d\n", l_ret);
		if (l_ret==WSAEWOULDBLOCK) {
			MGL_WARNING(MGL_CTX, "WSAEWOULDBLOCK.\n");
		}
		if (l_ret==WSANOTINITIALISED) {
			MGL_WARNING(MGL_CTX, "WSANOTINITIALISED.\n");
		}
		if (l_ret==WSAEFAULT) {
			MGL_WARNING(MGL_CTX, "WSAEFAULT.\n");
		}
		if (l_ret==WSAENETDOWN) {
			MGL_WARNING(MGL_CTX, "WSAENETDOWN.\n");
		}
		if (l_ret==WSAEINVAL) {
			MGL_WARNING(MGL_CTX, "WSAEINVAL.\n");
		}
		if (l_ret==WSAEINTR) {
			MGL_WARNING(MGL_CTX, "WSAEINTR.\n");
		}
		if (l_ret==WSAENOTSOCK) {
			MGL_WARNING(MGL_CTX, "WSAENOTSOCK.\n");
		}
		if (l_ret==WSAEINPROGRESS) {
			MGL_WARNING(MGL_CTX, "WSAEINPROGRESS.\n");
		}
#endif
		return l_ret;
	}
	if (  l_ret== 0 ) {
		return 0;
	}
		
	// socket 1
	if (ip_s1) {
		if ( FD_ISSET(ip_s1->getFd(), &fdset) ) {	
			return 1;
		}
	}

	// socket 2
	if (ip_s2) {
		if ( FD_ISSET(ip_s2->getFd(), &fdset) ) {	
			return 2;
		}
	}
	// socket 3
	if (ip_s3) {
		if ( FD_ISSET(ip_s3->getFd(), &fdset) ) {	
			return 3;
		}
	}
	// socket 4
	if (ip_s4) {
		if ( FD_ISSET(ip_s4->getFd(), &fdset) ) {	
			return 4;
		}
	}

	return -1;
}


int mgl_socket_select_fd(long i_delay, int i_fd1, int i_fd2, int i_fd3, int i_fd4)
{
	fd_set fdset;
	struct timeval l_tv;
	int l_maxfd;
	int l_ret;


	// Test fd
	FD_ZERO(&fdset);
	l_maxfd =0;

	// socket 1
	if (i_fd1) {
		FD_SET((unsigned int)i_fd1, &fdset);
		if (i_fd1>l_maxfd) {
			l_maxfd = i_fd1;
		}
	}
	// socket 2
	if (i_fd2) {
		FD_SET((unsigned int)i_fd2, &fdset);
		if (i_fd2>l_maxfd) {
			l_maxfd = i_fd2;
		}
	}
	// socket 3
	if (i_fd3) {
		FD_SET((unsigned int)i_fd3, &fdset);
		if (i_fd3>l_maxfd) {
			l_maxfd = i_fd3;
		}
	}
	// socket 4
	if (i_fd4) {
		FD_SET((unsigned int)i_fd4, &fdset);
		if (i_fd4>l_maxfd) {

			l_maxfd = i_fd4;
		}
	}

	l_tv.tv_sec = i_delay/1000;
	l_tv.tv_usec = (i_delay-((i_delay/1000)*1000))*1000;
	l_ret = select(l_maxfd+1, &fdset, NULL, NULL, &l_tv);
	if (  l_ret< 0 ) {
#ifdef DEBUGWIN32
		l_ret = WSAGetLastError();
		MGL_WARNING(MGL_CTX, "WSAGetLastError()==%d\n", l_ret);
		if (l_ret==WSAEWOULDBLOCK) {
			MGL_WARNING(MGL_CTX, "WSAEWOULDBLOCK.\n");
		}
		if (l_ret==WSANOTINITIALISED) {
			MGL_WARNING(MGL_CTX, "WSANOTINITIALISED.\n");
		}
		if (l_ret==WSAEFAULT) {
			MGL_WARNING(MGL_CTX, "WSAEFAULT.\n");
		}
		if (l_ret==WSAENETDOWN) {
			MGL_WARNING(MGL_CTX, "WSAENETDOWN.\n");
		}
		if (l_ret==WSAEINVAL) {
			MGL_WARNING(MGL_CTX, "WSAEINVAL.\n");
		}
		if (l_ret==WSAEINTR) {
			MGL_WARNING(MGL_CTX, "WSAEINTR.\n");
		}
		if (l_ret==WSAENOTSOCK) {
			MGL_WARNING(MGL_CTX, "WSAENOTSOCK.\n");
		}
		if (l_ret==WSAEINPROGRESS) {
			MGL_WARNING(MGL_CTX, "WSAEINPROGRESS.\n");
		}
#endif
		return l_ret;
	}
	if (  l_ret== 0 ) {
		return 0;
	}
		
	// socket 1
	if (i_fd1) {
		if ( FD_ISSET(i_fd1, &fdset) ) {	
			return 1;
		}
	}

	// socket 2
	if (i_fd2) {
		if ( FD_ISSET(i_fd2, &fdset) ) {	
			return 2;
		}
	}
	// socket 3
	if (i_fd3) {
		if ( FD_ISSET(i_fd3, &fdset) ) {	
			return 3;
		}
	}
	// socket 4
	if (i_fd4) {
		if ( FD_ISSET(i_fd4, &fdset) ) {	
			return 4;
		}
	}

	return -1;
}










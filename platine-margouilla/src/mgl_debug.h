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
/* $Id: mgl_debug.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef MGL_DEBUG_H
#define MGL_DEBUG_H

#include "mgl_type.h"

#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif


/**************************************************
*
* Debug levels
*
**************************************************/
#define MGL_TRACE_NONE		0x0000
#define MGL_TRACE_SOCKET	0x0001		/* Socket read/write*/
#define MGL_TRACE_MSG		0x0002		/* Msg event functions */
#define MGL_TRACE_TIMER		0x0004		/* Timer event functions */
#define MGL_TRACE_FD		0x0008		/* Msg event functions */
#define MGL_TRACE_CHANNEL	0x0010		/* Channel functions */
#define MGL_TRACE_CMD		0x0020		/* Remote command */
#define MGL_TRACE_MAIN_STEP	0x0040		/* Main steps */
#define MGL_TRACE_MGR_CX	0x0080		/* Messages exchanged between managers */
#define MGL_TRACE_FIO		0x0100		/* Function in/out */
#define MGL_TRACE_INFO 		0x0200		/* Enable Info */
#define MGL_TRACE_WARNING	0x0400		/* Enable Warnings */
#define MGL_TRACE_CRITICAL	0x0800		/* Enable Critical messages */
#define MGL_TRACE_HEADER	0x1000		/* Margouilla hearders */
#define MGL_TRACE_ROUTING	0x2000		/* Msg routing decisions */
#define MGL_TRACE_ALL		0xFFFF

#define MGL_TRACE_DEFAULT	MGL_TRACE_HEADER|MGL_TRACE_MAIN_STEP|MGL_TRACE_WARNING|MGL_TRACE_CRITICAL

/**************************************************
*
* Trace & Debug functions
*
**************************************************/
#define MGL_CTX	__FILE__, __LINE__
void MGL_TRACE_SET_LEVEL(long i_flag);
void MGL_TRACE_SET_FLAG(long i_flag);
int  MGL_NEED_TRACE(long i_level);
void MGL_TRACE(const char *ip_file, long i_line, long i_level, const char *,...);
void MGL_TRACE_BUF(const char *ip_file, long i_line, long i_level, char *ip_header, char *ip_buf, long i_buflen);
void MGL_WARNING(const char *ip_file, long i_line, const char *ip_format,...);
void MGL_CRITICAL(const char *ip_file, long i_line, const char *ip_format,...);

/**************************************************
*
* ASSERT
*
**************************************************/
// Stop and core dump or launch debugger
#ifndef MGL_NO_DEBUGGER
#define MGL_DEBUGGER() abort()
#else
#define MGL_DEBUGGER()
#endif

// Assert
#ifndef MGL_NO_ASSERT
#define MGL_ASSERT(x) \
	{ \
		if ( !(x) )  { \
			printf("Assertion failed: \"%s\" in %s (%d)\n", #x, __FILE__, __LINE__); \
			MGL_DEBUGGER(); \
		} \
	}
#else
#define MGL_ASSERT(x) { }
#endif



class mgl_sleep
{
public:
	static void sleep(long i_ms)
	{
	#ifdef WIN32
		//Sleep((DWORD)i_ms);
		SleepEx((DWORD)i_ms, 0);
		/*
		struct timeval l_tv;
		int l_ret;
		l_tv.tv_sec = i_ms/1000;
		l_tv.tv_usec = (i_ms-(l_tv.tv_sec*1000))*1000;
		l_ret = select(0, NULL, NULL, NULL, &l_tv);
		*/
	#else
		struct timeval l_tv;
		int l_ret;
		l_tv.tv_sec = i_ms/1000;
		l_tv.tv_usec = (i_ms-(l_tv.tv_sec*1000))*1000;
		l_ret = select(0, NULL, NULL, NULL, &l_tv);
	#endif
	}

};





/**************************************************
*
* mgl_trace classe
*
**************************************************/

class mgl_trace 
{
protected:
	virtual void print(char *i_buf);
public:
	virtual ~mgl_trace();
	virtual void trace(const char *,...);
	virtual void trace_buf(int i_sourceLine,
		      const char *ip_sourceFilename,
		      int  i_seuil_trace,
		      char *ip_buf,
		      char *ip_label,
		      int  i_taillebuf);
	virtual void close();
};



class mgl_trace_screen: public mgl_trace
{
public:
	virtual void print(char *i_buf);
};



class mgl_trace_file: public mgl_trace
{
public:
	FILE *_fd;
	mgl_trace_file();
	mgl_status open(char *ip_file);
	virtual void print(char *i_buf);
	void close();
};


class mgl_trace_file_nam: public mgl_trace_file
{
public:
	void init_node(mgl_id i_id);
	void init_link(mgl_id i_src_id, mgl_id i_dst_id);
	void init_queue(mgl_id i_src_id, mgl_id i_dst_id);
	void send(long i_time_ms, mgl_id i_src, mgl_id i_dst, const char *ip_desc, long i_length, long i_pkt_id);
	void receive(long i_time_ms, mgl_id i_src, mgl_id i_dst, const char *ip_desc, long i_length, long i_pkt_id);
	void drop(long i_time_ms, mgl_id i_src, mgl_id i_dst, char *ip_desc, long i_length, long i_pkt_id);
	void enqueue(long i_time_ms, mgl_id i_src, mgl_id i_dst, char *ip_desc, long i_length, long i_pkt_id);
	void dequeue(long i_time_ms, mgl_id i_src, mgl_id i_dst, char *ip_desc, long i_length, long i_pkt_id);
};

#endif

 

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
**
**********************************************************************/
/* $Id: mgl_time.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#include <sched.h>

#include <unistd.h>
#endif

#include "mgl_time.h"

/* using clock_nanosleep of librt */
extern int clock_nanosleep(clockid_t __clock_id, int __flags,
        __const struct timespec *__req,
        struct timespec *__rem);


long mgl_get_tickcount()
{
	#ifdef WIN32
		return GetTickCount();
	#else
		struct timeval l_now;
		long l_count;
		gettimeofday(&l_now, NULL);
		l_count=(l_now.tv_sec)*1000+(l_now.tv_usec)/1000;
		return l_count;
	#endif
}



void mgl_sleep(long i_ms)
{
#ifdef WIN32
	Sleep((DWORD)i_ms);
#else
/*	struct timeval l_tv;
	int l_ret;
	l_tv.tv_sec = i_ms/1000;
	l_tv.tv_usec = (i_ms-(l_tv.tv_sec*1000))*1000;
	l_ret = select(0, NULL, NULL, NULL, &l_tv);
*/
    struct timespec t;
    t.tv_sec = i_ms/1000;
    t.tv_nsec = (i_ms-(t.tv_sec*1000))*1000000;

    clock_nanosleep(0, TIMER_ABSTIME, &t, NULL);


#endif
}




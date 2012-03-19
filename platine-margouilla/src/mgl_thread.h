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
/* $Id: mgl_thread.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


#ifndef MGL_THREAD_H
#define MGL_THREAD_H


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>
//#include "mgl_type.h"

// thread
#ifdef WIN32
typedef HANDLE mgl_thread;	
#else
typedef pthread_t mgl_thread;
#endif

// Thread function type
#ifdef WIN32
#define MGL_THREAD_FCT_RETURN_TYPE DWORD WINAPI
typedef DWORD WINAPI mgl_thread_fct (LPVOID );
#define MGL_THREAD_FCT_RETURN(x) return x;
#else
#define MGL_THREAD_FCT_RETURN_TYPE void *
typedef void *mgl_thread_fct (void *);
#define MGL_THREAD_FCT_RETURN(x)
#endif

// Create thread
int mgl_thread_create(mgl_thread *op_thread, mgl_thread_fct *ip_fct);

// Terminate thread
void mgl_thread_terminate(mgl_thread *ip_thread);

// thread id
int ud_get_current_thread_id();


// Mutex
#ifdef WIN32
typedef HANDLE	mgl_mutex;
#else
typedef pthread_mutex_t	mgl_mutex;
#endif

// Create Mutex
void mgl_mutex_init(mgl_mutex *iop_mutex);

// Mutex lock
void mgl_mutex_lock(mgl_mutex *iop_mutex);

// Mutex unlock
void mgl_mutex_unlock(mgl_mutex *iop_mutex);




#endif


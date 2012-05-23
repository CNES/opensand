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
/* $Id: mgl_thread.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Application Includes ******/
#include "mgl_thread.h"



int mgl_thread_create(mgl_thread *op_thread, mgl_thread_fct *ip_fct)
{
#ifdef WIN32
  DWORD l_id=0;
#else
  pthread_attr_t	lv_thread_attr;
#endif
  
#ifdef WIN32
	(*op_thread) = CreateThread(
		NULL,	// default security attribut
		0,		// default stack size
		ip_fct, // Adresse de la fonction du thread
		0,		// pas de parametres
		0,		// lancement des que possible
		&l_id
	);
	return l_id;
#else
    pthread_attr_init(&lv_thread_attr);
    return pthread_create(op_thread, &lv_thread_attr, ip_fct, NULL);
#endif
}



void mgl_thread_terminate(mgl_thread *ip_thread)
{
#ifdef WIN32
  LPVOID lpMsgBuf;
  DWORD dwExitCode=0;
#endif

#ifdef WIN32
  if (!( TerminateThread( (*ip_thread), dwExitCode ))) {
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
  }
#else
  if (pthread_cancel(*ip_thread) != 0) {
    // Error
  } 
#endif
}


// Warning, fix it under linux
int ud_get_current_thread_id()
{
 #ifndef WIN32
   pthread_t  lv_tid;
 
   lv_tid=pthread_self();
   //return pthread_getunique_np(&lv_tid);
 #else
   return GetCurrentThreadId();
 #endif
}


// -1: Pb

void mgl_mutex_init(mgl_mutex *iop_mutex)
{
#ifdef WIN32
  (*iop_mutex) = CreateSemaphore(NULL, 1, 1, NULL); // Init sans prise du mutex
#else

  pthread_mutex_init(iop_mutex, NULL); //pthread_mutexattr_default for  HP Ux

#endif
}



void mgl_mutex_lock(mgl_mutex *iop_mutex)
{
#ifdef WIN32
  if (WaitForSingleObject( (*iop_mutex), INFINITE )== WAIT_FAILED) {
	  /*
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
		*/
  }
#else
  if (pthread_mutex_lock(iop_mutex) == -1) {
    // Error
  } 
#endif
}

void mgl_mutex_internal_synchro()
{ /* This fonction fixes an HPUX mutex bug */
#ifdef HPUX
//   HPUX bug
 struct timespec interval;
  interval.tv_sec=0; /* secondes */
  interval.tv_nsec=0; /* nano secondes */
  pthread_delay_np(&interval); /* passe au thread suivant */
#endif
}


void mgl_mutex_unlock(mgl_mutex *iop_mutex)
{
#ifdef WIN32
  LPVOID lpMsgBuf;
  if (!(ReleaseSemaphore( (*iop_mutex), 1, NULL))) {
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);  
  }
#else
  if (pthread_mutex_unlock(iop_mutex) == -1) {
  } 
#endif
  mgl_mutex_internal_synchro();
}




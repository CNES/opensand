/**********************************************************************
**
** Margouilla Runtime Test
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
/* $Id: test_thread.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Application Includes ******/
#include "mgl_thread.h"
#include "mgl_debug.h"

//////////////////////// Test 1
// Create threads 1 and 2, wait 5s then terminate them.
// Thread 1: print "Thread 1 (id) every 500 ms
// Thread 2: print "Thread 1 (id) every 1 s
MGL_THREAD_FCT_RETURN_TYPE thread1(void *ip_null)
{
	for (;;) {
		printf("Thread 1 (%d)\n", ud_get_current_thread_id());
		mgl_sleep::sleep(500);
	}
	MGL_THREAD_FCT_RETURN(0);
}

MGL_THREAD_FCT_RETURN_TYPE thread2(void *ip_null)
{
	for (;;) {
		printf("Thread 2 (%d)\n", ud_get_current_thread_id());
		mgl_sleep::sleep(1000);
	}
	MGL_THREAD_FCT_RETURN(0);
}

void test1()
{
	mgl_thread t1;
	mgl_thread t2;

	printf("Create threads 1 & 2\n");
	mgl_thread_create(&t1, &thread1);
	mgl_thread_create(&t2, &thread2);

	printf("Sleep 5s\n");
	mgl_sleep::sleep(5000);

	printf("Terminate thread 1 & 2\n");
	mgl_thread_terminate(&t1);
	mgl_thread_terminate(&t2);

	printf("Sleep 2s\n");
	mgl_sleep::sleep(2000);
}

//////////////////////// Test 2
// Create a mutex, then threads 1 and 2, wait 5s then terminate them.
// Thread 1: get the mutex, keep it 500 ms, then release it
// Thread 2: get the mutex, keep it 2 s, then release it

mgl_mutex g_mutex;

MGL_THREAD_FCT_RETURN_TYPE thread1_2(void *ip_null)
{
	for (;;) {
		printf("Thread 1 (%d) wait to lock mutex\n", ud_get_current_thread_id());
		mgl_mutex_lock(&g_mutex);
		printf("Thread 1 (%d) mutex lock, sleep 500\n", ud_get_current_thread_id());
		mgl_sleep::sleep(500);
		printf("Thread 1 (%d) unlock mutex\n", ud_get_current_thread_id());
		mgl_mutex_unlock(&g_mutex);
	}
	MGL_THREAD_FCT_RETURN(0);
}

MGL_THREAD_FCT_RETURN_TYPE thread2_2(void *ip_null)
{
	for (;;) {
		printf("Thread 2 (%d) wait to lock mutex\n", ud_get_current_thread_id());
		mgl_mutex_lock(&g_mutex);
		printf("Thread 2 (%d) mutex lock, sleep 2000\n", ud_get_current_thread_id());
		mgl_sleep::sleep(2000);
		printf("Thread 2 (%d) unlock mutex\n", ud_get_current_thread_id());
		mgl_mutex_unlock(&g_mutex);
	}
	MGL_THREAD_FCT_RETURN(0);
}

void test2()
{
	mgl_thread t1;
	mgl_thread t2;

	printf("Create threads 1 & 2\n");
	mgl_mutex_init(&g_mutex);
	mgl_thread_create(&t1, &thread1_2);
	mgl_thread_create(&t2, &thread2_2);

	printf("Sleep 5s\n");
	mgl_sleep::sleep(5000);

	printf("Terminate thread 1 & 2\n");
	mgl_thread_terminate(&t1);
	mgl_thread_terminate(&t2);

	printf("Sleep 2s\n");
	mgl_sleep::sleep(2000);
}



int main()
{
	test1();
	test2();
	return 0;
}


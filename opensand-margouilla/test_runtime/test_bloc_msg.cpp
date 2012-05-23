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
/* $Id: test_bloc_msg.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Application Includes ******/
#include "mgl_eventmgr.h"
#include "mgl_bloc.h"
#include "mgl_blocmgr.h"

//////////////////////// Test 1

mgl_id g_id_1;
mgl_id g_id_2;

#define MSG_TEST_TYPE_1 1
#define MSG_TEST_TYPE_2 2
#define MSG_TEST_TYPE_3 3

// Blocs heritate from mgl_bloc classe
// mgl_bloc classe defines some default handlers such as 'onEvent'
// Here, BlockSend classe set a timer, then send two messages
class BlockSend: public mgl_bloc {
public:
	// Class constructor
	// Use mgl_bloc default constructor
	BlockSend(mgl_blocmgr *ip_blocmgr, mgl_id i_fatherid, const char *ip_name):
	  mgl_bloc(ip_blocmgr, i_fatherid, ip_name)
	{
	}

	//Event handlers
	mgl_status onEvent(mgl_event *ip_event) { 
		static mgl_id l_timer; // Store the timer id
		mgl_msg *lp_msg;
		char l_buf[]="Hello world";
		int l_len;
		char *lp_ptr;

		// The first event received by each bloc is : mgl_event_type_init
		// It simply 
		// Init event, set timer
		if (MGL_EVENT_IS_INIT(ip_event)) {
			printf("BlockSend Ready\n");
			setTimer(l_timer, 1000); 
			printf("Timer set\n");
			return mgl_ok;
		}

		// Receive a Timer event
		if (MGL_EVENT_IS_TIMER(ip_event)) { 
			if (MGL_EVENT_TIMER_IS_TIMER(ip_event, l_timer)) {
				printf("Timer received\n");

				// Send msg1: Msg type=MSG_TEST_TYPE_1, no body
				// Quite easy, not ?
				lp_msg = newMsg(MSG_TEST_TYPE_1); 
				sendMsgTo(g_id_2, lp_msg); // Send to bloc2
				printf("Msg 1 sent\n");

				// Send msg2: Msg type=MSG_TEST_TYPE, body contains a char buffer
				// The buffer is allocated, then memcopied from bloc buffer into message
				// This is the safer way to manage messages but it is not optimised 
				// for fast data exchange (lot of memcopy and malloc).
				l_len = strlen(l_buf)+1; // Include last \0
				lp_msg = newMsg(MSG_TEST_TYPE_2, l_buf, l_len); 
				sendMsgTo(g_id_2, lp_msg); // Send to bloc2
				printf("Msg 2 sent\n");

				// Send msg3: Msg type=MSG_TEST_TYPE, body constains a pointer to a buffer
				// No memcopy is done, memory is allocated by the sender, and only a
				// pointer is sent to the receiver.
				// This is faster, but be very carrefull with pointer management and memory
				// allocation, especially with distributed process and threads.
				lp_ptr = (char *)malloc(150);
				sprintf(lp_ptr, "Msg body contains only a pointer.");
				lp_msg = newMsgWithBodyPtr(MSG_TEST_TYPE_3, lp_ptr); 
				sendMsgTo(g_id_2, lp_msg); // Send to bloc2
				printf("Msg 2 sent\n");

				// Set the timer again
				setTimer(l_timer, 1000, mgl_true); 
				return mgl_ok;
			}
		}
		return mgl_ok; 
	};
};


// Blocs heritate from mgl_bloc classe
// mgl_bloc classe defines some default handlers such as 'onEvent'
// Here, BlockReceive classe simply wait for messages
class BlockReceive: public mgl_bloc {
public:
	// Class constructor
	// Use mgl_bloc default constructor
	BlockReceive(mgl_blocmgr *ip_blocmgr, mgl_id i_fatherid, const char *ip_name):
	  mgl_bloc(ip_blocmgr, i_fatherid, ip_name)
	{
	}

	//Event handlers
	mgl_status onEvent(mgl_event *ip_event) { 
		char *lp_ptr;

		// Init, nothing to do
		if (MGL_EVENT_IS_INIT(ip_event)) {
			printf("BlockReceive Ready\n");
			return mgl_ok;
		}

		// Receive a message
		if (MGL_EVENT_IS_MSG(ip_event)) { 

			// Msg type 1
			if (MGL_EVENT_MSG_IS_TYPE(ip_event, MSG_TEST_TYPE_1)) {
				printf("Received Msg 1\n");
				return mgl_ok;
			}

			// Msg type 2
			if (MGL_EVENT_MSG_IS_TYPE(ip_event, MSG_TEST_TYPE_2)) {
				printf("Received Msg 2 [%s]\n", (char *)ip_event->event.msg.ptr->pBuf);
				return mgl_ok;
			}

			// Msg type 3
			if (MGL_EVENT_MSG_IS_TYPE(ip_event, MSG_TEST_TYPE_3)) {
				lp_ptr = (char *)(ip_event->event.msg.ptr->pBuf);
				printf("Received Msg 3 [%s]\n", lp_ptr);
				free(lp_ptr);
				return mgl_ok;
			}

		}
		return mgl_ok; 
	};
};



int main()
{
	// First of all instanciate an event manager.
	// Event manager manages time (real, compressed, event based), message files, sockets...
	mgl_eventmgr	l_eventmgr(realTime);

	// Then instanciate a bloc manager.
	// A bloc manager manages bloc lists and solve message routing between blocs.
	mgl_blocmgr		l_blocmgr;

	// Set Runtime Trace level
	MGL_TRACE_SET_LEVEL(0);

	// Set event Mgr to Bloc manager
	l_blocmgr.setEventMgr(&l_eventmgr);


	// Instancitate blocs
	// Each bloc register itself and could instanciate some subblocs
	// A unique Id is set for each bloc when registering
	// This id is used here to send messages to blocs.
	mgl_bloc *lp_bloc1 = new BlockSend(&l_blocmgr, 0/* 0 means no father*/, "BlockSend");
	g_id_1 = lp_bloc1->getId();

	mgl_bloc *lp_bloc2 = new BlockReceive(&l_blocmgr, 0/* 0 means no father*/, "BlockReceive");
	g_id_2 = lp_bloc2->getId();

	// Now, each system bloc is registered, got an Id, 
	// has registered its links, and know wether it must run localy or remotly
	// Set the eventMgr for localy managed blocs
	mgl_status l_ret = l_blocmgr.setEventMgrToLocallyManagedBlocs();

	// Let's run...
	for (;;) {
		l_ret = l_blocmgr.process_step();
	}
	return 0;
}



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
/* $Id: test_bloc_delayed_msg.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


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
				printf("\nTimer received at %ld\n", getCurrentTime());

				// Send msg1: Msg type=MSG_TEST_TYPE_1
				// 500 ms delay
				lp_msg = newMsg(MSG_TEST_TYPE_1); 
				sendDelayedMsgTo(g_id_2, lp_msg, 500); // Send to bloc2
				printf("Msg 1 sent with 500ms delay.\n");

				// Send msg2: Msg type=MSG_TEST_TYPE_1
				// 200 ms delay
				lp_msg = newMsg(MSG_TEST_TYPE_2); 
				sendDelayedMsgTo(g_id_2, lp_msg, 200); // Send to bloc2
				printf("Msg 2 sent with 200 ms delay\n");

				// Send msg2: Msg type=MSG_TEST_TYPE_1
				// 200 ms delay
				lp_msg = newMsg(MSG_TEST_TYPE_3); 
				sendMsgTo(g_id_2, lp_msg); // Send to bloc2
				printf("Msg 3 sent without delay\n");

				// Bloc 2 should receives : 3(0ms), 2(200 ms), 1(300)ms
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
		static long s_last=0;

		// Init, nothing to do
		if (MGL_EVENT_IS_INIT(ip_event)) {
			printf("BlockReceive Ready\n");
			return mgl_ok;
		}

		// Receive a message
		if (MGL_EVENT_IS_MSG(ip_event)) { 

			// Msg type 1
			if (MGL_EVENT_MSG_IS_TYPE(ip_event, MSG_TEST_TYPE_1)) {
				printf("Received Msg 1 (%ld ms delay)\n", getCurrentTime()-s_last);
				return mgl_ok;
			}

			// Msg type 2
			if (MGL_EVENT_MSG_IS_TYPE(ip_event, MSG_TEST_TYPE_2)) {
				printf("Received Msg 2 (%ld ms delay)\n", getCurrentTime()-s_last);
				return mgl_ok;
			}

			// Msg type 3
			if (MGL_EVENT_MSG_IS_TYPE(ip_event, MSG_TEST_TYPE_3)) {
				printf("Received Msg 3 (at %ld)\n", getCurrentTime());
				s_last = getCurrentTime();
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

	// Enable delayed message handling.
	// Thanks to this functionnality one can simulate a link delay.
	// Off course it results in a increased use of CPU to sort message lists.
	// By default this flag is not set
	l_eventmgr.setDelayedEventsFlag(1);

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



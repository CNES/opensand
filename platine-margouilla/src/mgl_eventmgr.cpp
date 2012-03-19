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
/* $Id: mgl_eventmgr.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "mgl_eventmgr.h"
#include "mgl_debug.h"
#include "mgl_socket.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <time.h>
#else
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>  
#ifdef __linux__                                                    
#include <asm/ioctls.h>     // Linux                                                   
#endif
#endif


#define NB_EVENT_MAX		15000
#define NB_EVENT_MSG_MAX	15000


// Time mgr: Real Time, or Compressed Time
mgl_eventmgr::mgl_eventmgr(mgl_timeType i_timeType)
{ 
	// Set time type
	_timeType=i_timeType; 
	
	// Manage delayed event ?
	_delayedEventsFlag=0;

	// Allocate memory pools
	_msg_list.init(NB_EVENT_MSG_MAX);
	l_memory_pool_event.allocate(sizeof(mgl_event), NB_EVENT_MAX);
	l_memory_pool_event.setName("EventMgr: Event Memory Pool");
	l_memory_pool_msg.allocate(sizeof(mgl_event), NB_EVENT_MSG_MAX);
	l_memory_pool_msg.setName("EventMgr: Msg Memory Pool");
}

mgl_eventmgr::~mgl_eventmgr()
{
	mgl_eventmgr_fd *fd;
	mgl_event *event;
	long i;

	/* clear the list of fd */
	for(i = 0; i < this->_fd_list.getCount(); i++)
	{
		fd = (mgl_eventmgr_fd *) this->_fd_list.get(i);
		
		if(fd != NULL)
			free(fd);
	}

	/* clear the message events */
	for(i = 0; i < this->_msg_list.getCount(); i++)
	{
		event = (mgl_event *) this->_msg_list.get(i);
		this->freeEvent(event);
	}

	/* clear the fd events */
	for(i = 0; i < this->_fd_event_list.getCount(); i++)
	{
		event = (mgl_event *) this->_fd_event_list.get(i);
		this->freeEvent(event);
	}

	/* clear the timer events */
	for(i = 0; i < this->_timer_list.getCount(); i++)
	{
		event = (mgl_event *) this->_timer_list.get(i);
		this->freeEvent(event);
	}
}

void mgl_eventmgr::setDelayedEventsFlag(long i_val)
{
	_delayedEventsFlag = i_val;
}

long mgl_eventmgr::getDelayedEventsFlag()
{
	return _delayedEventsFlag;
}


long mgl_eventmgr::getNextEventDelay()
{
	mgl_event *lp_event;
	long l_current;
	long l_delay;
	long l_delaymsg;
	long l_delaytimer;

	// Current time
	l_current = getCurrentTime();

	// Check delayed msg
	l_delaymsg=1000;
	if (_msg_list.getCount()) {
		//lp_event = (mgl_event *)_msg_list.get(0);
		lp_event = (mgl_event *)_msg_list.get();
		if (lp_event) {
			l_delaymsg = lp_event->event.msg.time_out -  l_current;
			if (!_delayedEventsFlag) { l_delaymsg=0; }
			if (l_delaymsg<0) { l_delaymsg=0; }
		}
	}

	// Check timer
	l_delaytimer=1000;
	if (_timer_list.getCount()) {
		lp_event = (mgl_event *)_timer_list.get(0);
		if (lp_event) {
			//l_delaytimer = lp_event->event.msg.time_out -  l_current;
			l_delaytimer = lp_event->event.timer.time -  l_current;
			if (l_delaytimer<0) { l_delaytimer=0; }
		}
	}

	// Get min
	if (l_delaytimer<l_delaymsg) {
		l_delay=l_delaytimer; 
	} else {
		l_delay=l_delaymsg;
	}
	return l_delay; 
}


// Poll & manage Events
mgl_event *mgl_eventmgr::getNextEvent(long i_ms)
{
	mgl_event *lp_event;
	long l_nexteventdelay;
	mgl_status l_ret;

	// Get next event time (delayed msg, timer)
	l_nexteventdelay = getNextEventDelay();

	// If real time clock, then wait for a socket event during this period
	// If Compressed time clock, jump in time
	if (_timeType==realTime) {
		if (i_ms<l_nexteventdelay) {
			l_nexteventdelay = i_ms;
		}
	} else {
		incrementCurrentTime(l_nexteventdelay);
	}


	// Check timer
	if (_timer_list.getCount()) {
		lp_event = (mgl_event *)_timer_list.get(0);
		if (!lp_event) {
			// Critical: Runtime integrity error
			return NULL; 
		}
		if (lp_event->event.timer.time <=getCurrentTime()) {
			_timer_list.remove(0);
			return lp_event;
		}
	}

	// Check msg
	if (_msg_list.getCount()) {
		//lp_event = (mgl_event *)_msg_list.get(0);
		lp_event = (mgl_event *)_msg_list.get();
		if (!lp_event) {
			// Critical: Runtime integrity error
			return NULL; 
		}
		MGL_TRACE(MGL_CTX, MGL_TRACE_MSG, "Msg out at %d, (current %d)\n", lp_event->event.msg.time_out, getCurrentTime());
		if (lp_event->event.msg.time_out <=getCurrentTime()) {
			//_msg_list.remove(0);
			_msg_list.remove();
			return lp_event;
		}
	}


	// Check fd, if stored event dispatch, else 
	// blocking select during l_nexteventdelay ms
	if (_fd_event_list.getCount()>0) {
		lp_event = (mgl_event *)_fd_event_list.remove(0);
		if (!lp_event) {
			// Warning
			return NULL; 
		}
		return lp_event;
	}
	if (_fd_list.getCount()>0) {
		// select fd
		l_ret = selectFd(l_nexteventdelay);
		if (l_ret==mgl_ok) {
			// Dispatch first fd event
			if (_fd_event_list.getCount()>0) {
				lp_event = (mgl_event *)_fd_event_list.remove(0);
				if (!lp_event) {
					// Warning
					return NULL; 
				}
				return lp_event;
			}
		}

	} else {
		if (l_nexteventdelay) {
			mgl_sleep::sleep(l_nexteventdelay);
			//printf("Sleep(%d)", l_nexteventdelay); 
		}
	}

	return NULL; 
}


// Poll & manage internal Events: remote cmd & msg between mgr
mgl_event *mgl_eventmgr::getNextInternalEvent(long i_ms)
{
	mgl_event *lp_event;
	long l_nexteventdelay;
	mgl_status l_ret;
	long l_nb;
	long l_cpt;

	// Get next event time (delayed msg, timer)
	l_nexteventdelay = getNextEventDelay();

	// If real time clock, then wait for a socket event during this period
	// If Compressed time clock, jump in time
	if (_timeType==realTime) {
		if (i_ms<l_nexteventdelay) {
			l_nexteventdelay = i_ms;
		}
	} else {
		incrementCurrentTime(l_nexteventdelay);
	}


	// Check fd, if stored event dispatch, else 
	// blocking select during l_nexteventdelay ms
	if (_fd_event_list.getCount()>0) {
		l_nb = _fd_event_list.getCount();
		for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
			lp_event = (mgl_event *)_fd_event_list.get(l_cpt);
			if (lp_event->event.fd.blocid<0) {
				lp_event = (mgl_event *)_fd_event_list.remove(l_cpt);
				return lp_event;
			}
		}
	}

	if (l_nexteventdelay<10) { l_nexteventdelay=10; }
	if (_fd_list.getCount()>0) {
		// select fd
		l_ret = selectFd(l_nexteventdelay, true);
		if (l_ret==mgl_ok) {
			// Dispatch first fd event
			if (_fd_event_list.getCount()>0) {
				lp_event = (mgl_event *)_fd_event_list.remove(0);
				if (!lp_event) {
					// Warning
					return NULL; 
				}
				return lp_event;
			}
		}

	} else {
		mgl_sleep::sleep(l_nexteventdelay);
	}

	return NULL; 
}
mgl_status mgl_eventmgr::addFd(long i_fd, mgl_id i_blocid)
{
	mgl_eventmgr_fd *lp_mgl_eventmgr_fd;

	if (getFd(i_fd)) {
		MGL_WARNING(MGL_CTX, "fd already registered\n");
		return mgl_ko;
	}

	lp_mgl_eventmgr_fd =  (mgl_eventmgr_fd *)malloc(sizeof(mgl_eventmgr_fd));
	if(lp_mgl_eventmgr_fd == NULL)
	{
		MGL_WARNING(MGL_CTX, "cannot allocate memory for fd\n");
		return mgl_ko;
	}

	lp_mgl_eventmgr_fd->fd = i_fd;
	lp_mgl_eventmgr_fd->blocid = i_blocid;
	_fd_list.append(lp_mgl_eventmgr_fd);

	return mgl_ok;
}

mgl_eventmgr_fd *mgl_eventmgr::getFd(long i_fd)
{
	mgl_eventmgr_fd *lp_mgl_eventmgr_fd;
	long l_nb;
	long l_cpt;

	l_nb = _fd_list.getCount();
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		lp_mgl_eventmgr_fd =  (mgl_eventmgr_fd *)_fd_list.get(l_cpt);
		if (lp_mgl_eventmgr_fd) {
			if (lp_mgl_eventmgr_fd->fd == i_fd) {
				return lp_mgl_eventmgr_fd;
			}
		}
	}
	return NULL;
}

mgl_status mgl_eventmgr::removeFd(long i_fd)
{
	mgl_eventmgr_fd *lp_mgl_eventmgr_fd;

	lp_mgl_eventmgr_fd = getFd(i_fd);
	if (lp_mgl_eventmgr_fd) {
		_fd_list.removeByPtr(lp_mgl_eventmgr_fd);
		return mgl_ok;
	}
	return mgl_ko;
}




mgl_status mgl_eventmgr::selectFd(long i_delay, bool i_internalOnly)
{
	fd_set fdset;
	struct timeval l_tv;
	int l_maxfd;
	int l_ret;
	long l_nb;
	long l_cpt;
	mgl_eventmgr_fd *lp_mgl_eventmgr_fd;
	mgl_event *lp_event;

	// Test fd
	FD_ZERO(&fdset);
	l_maxfd =0;
	l_nb = _fd_list.getCount();
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		lp_mgl_eventmgr_fd =  (mgl_eventmgr_fd *)_fd_list.get(l_cpt);
		if (lp_mgl_eventmgr_fd) {
			if (lp_mgl_eventmgr_fd->fd) {
				if ((!i_internalOnly)||(lp_mgl_eventmgr_fd->blocid<0)) {
					FD_SET((unsigned int)lp_mgl_eventmgr_fd->fd, &fdset);
					if (lp_mgl_eventmgr_fd->fd>l_maxfd) {
						l_maxfd = lp_mgl_eventmgr_fd->fd;
					}
				}
			}
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
		return mgl_ko;
	}
	if (  l_ret== 0 ) {
		return mgl_ok;
	}
		
	l_nb = _fd_list.getCount();
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		lp_mgl_eventmgr_fd =  (mgl_eventmgr_fd *)_fd_list.get(l_cpt);
		if (lp_mgl_eventmgr_fd) {
			if (lp_mgl_eventmgr_fd->fd) {
				if ( FD_ISSET(lp_mgl_eventmgr_fd->fd, &fdset) ) {	
#if MEMORY_POOL
					lp_event = allocateNewEvent();
#else
					lp_event = (mgl_event *)malloc(sizeof(mgl_event));
#endif
					lp_event->type = mgl_event_type_fd;
					lp_event->event.fd.fd = lp_mgl_eventmgr_fd->fd;
					lp_event->event.fd.blocid = lp_mgl_eventmgr_fd->blocid;
					_fd_event_list.append(lp_event);
				}
			}
		}
	}
	return mgl_ok;
}



mgl_event *mgl_eventmgr::waitNextEvent(long i_ms) 
{ 
	mgl_event *lp_event;
	long l_endtime;

	l_endtime = getCurrentTime()+i_ms;
	do {
		lp_event=getNextEvent(i_ms);
	} while ((lp_event==NULL)&&(getCurrentTime()<l_endtime));

	return lp_event; 
}


mgl_msg *mgl_eventmgr::allocateNewMessage()
{
	mgl_msg *lp_msg;

	lp_msg=(mgl_msg *)l_memory_pool_msg.get();
	if (!lp_msg) {
		// Critical, run out of memory
		MGL_TRACE(MGL_CTX, MGL_TRACE_CRITICAL, "mgl_eventmgr::allocateNewMessage Out of memory\n");
	}
	return lp_msg;
}

mgl_event *mgl_eventmgr::allocateNewEvent()
{
	mgl_event *lp_event;
	lp_event=(mgl_event *)l_memory_pool_event.get();
	if (!lp_event) {
		// Critical, run out of memory
		MGL_TRACE(MGL_CTX, MGL_TRACE_CRITICAL, "mgl_eventmgr::allocateNewEvent Out of memory\n");
	}
	return lp_event;
}


mgl_status mgl_eventmgr::freeEvent(mgl_event *ip_event)
{ 
	if (!ip_event) { return mgl_ko; }
	switch (ip_event->type) {
		case mgl_event_type_msg: 
			if (ip_event->event.msg.ptr->freeBody) { free(ip_event->event.msg.ptr->pBuf); }
			l_memory_pool_msg.release((char *)ip_event->event.msg.ptr); //free(ip_event->event.msg.ptr);
			l_memory_pool_event.release((char *)ip_event); //free(ip_event);
		break;
		case mgl_event_type_timer: 
			l_memory_pool_event.release((char *)ip_event); //free(ip_event);
		break;
		case mgl_event_type_fd: 
			l_memory_pool_event.release((char *)ip_event); //free(ip_event);
		break;
		default: //trace("Warning mgl_eventmgr::freeEvent try to free unknown event\n");
		break;
	}
	return mgl_ko; 
};

mgl_bool mgl_event_msg_sort_fct(void *ip_em1, void *ip_em2)
{
	mgl_event *lp_msg_event1;
	mgl_event *lp_msg_event2;

	if (ip_em1&&ip_em2) {
		lp_msg_event1 = (mgl_event *)ip_em1;
		lp_msg_event2 = (mgl_event *)ip_em2;
		if ((lp_msg_event1->event.msg.time_out)>(lp_msg_event2->event.msg.time_out)) {
			return mgl_true;
		} else {
			return mgl_false;
		}
	}
	return mgl_false;
}


// Messages
//mgl_list _msg_list;
mgl_msginfo *mgl_eventmgr::sendMsg(mgl_msg *ip_msg, mgl_id i_fromBloc, mgl_id i_fromPort, long i_delay)
{
	mgl_event *lp_event;
	static long sl_counter=0;

	// Allocate memory for a new event
#if MEMORY_POOL
	lp_event    = allocateNewEvent();
#else
	lp_event = (mgl_event *)malloc(sizeof(mgl_event));
#endif
	if (!lp_event) {
		// Critical, run out of memory
		MGL_TRACE(MGL_CTX, MGL_TRACE_CRITICAL, "mgl_eventmgr::sendMsg Can't allocate new event.\n");
		exit(1);
	}

	// Set fields
	lp_event->type = mgl_event_type_msg;
	lp_event->event.msg.ptr = ip_msg;
	lp_event->event.msg.srcBloc = i_fromBloc;
	lp_event->event.msg.srcPort = i_fromPort;
	lp_event->event.msg.dstBloc = -1;
	lp_event->event.msg.dstPort = -1;
	lp_event->event.msg.time_in = getCurrentTime();
	lp_event->event.msg.time_out = getCurrentTime()+i_delay;
	lp_event->event.msg.id = sl_counter++;
	MGL_TRACE(MGL_CTX, MGL_TRACE_MSG, "mgl_eventmgr: Msg added at %d\n", getCurrentTime());

	// Append it at the end of the queue
	// If the runtime manages relayed messages, then the new message
	// is inserted and fifo is sorted.
	if (_delayedEventsFlag) {
		_msg_list.appendSorted(lp_event, mgl_event_msg_sort_fct);
	} else {
		_msg_list.append(lp_event);
	}

	return &(lp_event->event.msg);
}




mgl_status mgl_eventmgr::sortMsgList()
{
	mgl_status l_ret=mgl_ok;
	if (0/*_supportDelayedMessages&&_sortMessages*/) {
		l_ret = _msg_list.sort(mgl_event_msg_sort_fct);
	}
	return l_ret;
}


mgl_bool mgl_event_timer_sort_fct(void *ip_em1, void *ip_em2)
{
	mgl_event *lp_msg_event1;
	mgl_event *lp_msg_event2;

	if (ip_em1&&ip_em2) {
		lp_msg_event1 = (mgl_event *)ip_em1;
		lp_msg_event2 = (mgl_event *)ip_em2;
		if ((lp_msg_event1->event.timer.time)>(lp_msg_event2->event.timer.time)) {
			return mgl_true;
		} else {
			return mgl_false;
		}
	}
	return mgl_false;
}

mgl_status mgl_eventmgr::sortTimerList()
{
	mgl_status l_ret=mgl_ok;
	l_ret = _timer_list.sort(mgl_event_timer_sort_fct);
	return l_ret;
}


// number of ms (4bytes) since the system has started
// Loop every 40 days
// Take it into account pleaase..
long g_time=0;
long mgl_eventmgr::getCurrentTime()
{
	static long sl_starttick=0;
	long l_count;
	long l_ret;

	if (_timeType==realTime) {
		#ifdef WIN32
			l_count = GetTickCount();
			if (sl_starttick==0) { sl_starttick=l_count; }
			l_ret = (l_count- sl_starttick);
			return l_ret;
		#else
			struct timeval now;

			gettimeofday(&now, NULL);
			l_count=(now.tv_sec)*1000+(now.tv_usec)/1000;
			if (sl_starttick==0) { sl_starttick=l_count; }
			l_ret = (l_count- sl_starttick);
			return l_ret;
		#endif
	} else {
		return g_time;
	}
}

long mgl_eventmgr::incrementCurrentTime(long i_ms)
{
	if (_timeType!=realTime) {
		g_time+=i_ms;
	}
	return getCurrentTime();
}

// Timer
//mgl_list _timer_list;
mgl_status mgl_eventmgr::setTimer(mgl_id i_blocid, mgl_id &i_timerid, long i_mstimer, mgl_bool i_loop)
{
	mgl_event *lp_event;
	static mgl_id s_id=0;

	// Allocate memory for a new event
#if MEMORY_POOL
	lp_event    = allocateNewEvent();
#else
	lp_event = (mgl_event *)malloc(sizeof(mgl_event));
#endif

	// Set fields
	i_timerid = s_id++;
	if (i_timerid==65000) { i_timerid=1; }
	lp_event->type = mgl_event_type_timer;
	lp_event->event.timer.id = i_timerid;
	lp_event->event.timer.duration = i_mstimer;
	lp_event->event.timer.time = getCurrentTime()+i_mstimer;
	lp_event->event.timer.loop = i_loop;
	lp_event->event.timer.bloc = i_blocid;

	// Insert it in a sorted way
	_timer_list.appendSorted(lp_event, mgl_event_timer_sort_fct);
	//sortTimerList();

	return mgl_ok;
}

// Fd: File descriptor (socket, pipe...)



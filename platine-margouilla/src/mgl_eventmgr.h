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
/* $Id: mgl_eventmgr.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */





#ifndef MGL_EVENTMGR_H
#define MGL_EVENTMGR_H

#include "mgl_event.h"
#include "mgl_list.h"
#include "mgl_fifo.h"
#include "mgl_memorypool.h"

typedef	enum {
	realTime,
	compressedTime
} mgl_timeType;

typedef struct {
	mgl_id blocid;
	long fd;
} mgl_eventmgr_fd;


class mgl_eventmgr {
public:
	// Time mgr: Real Time, or Compressed Time
	mgl_eventmgr(mgl_timeType i_timeType=realTime);
	~mgl_eventmgr();

	// Poll & manage Events
	long _delayedEventsFlag;
	void setDelayedEventsFlag(long i_val);
	long getDelayedEventsFlag();

	mgl_memory_pool l_memory_pool_event;
	mgl_event *allocateNewEvent();
	long getNextEventDelay();
	mgl_event *getNextEvent(long i_ms=0);
	mgl_event *getNextInternalEvent(long i_ms=0);
	mgl_event *waitNextEvent(long i_ms);
	mgl_status freeEvent(mgl_event *ip_event);

	// Messages
	mgl_memory_pool l_memory_pool_msg;
	mgl_msg *allocateNewMessage(); 
	//mgl_list _msg_list;
	mgl_fifo _msg_list;
	mgl_msginfo *sendMsg(mgl_msg *ip_msg, mgl_id i_fromBloc, mgl_id i_fromPort=-1, long i_delay=0);
	mgl_status sortMsgList();
	mgl_status sortTimerList();

	// Time type
	mgl_timeType _timeType;

	// Timer
	mgl_list _timer_list;
	long getCurrentTime();
	long incrementCurrentTime(long i_ms);
	mgl_status setTimer(mgl_id i_blocid, mgl_id &i_timerid, long i_mstimer, mgl_bool i_loop=mgl_false);

	// Fd: File descriptor (socket, pipe...)
	mgl_list _fd_list; // list of fd to manage
	mgl_list _fd_event_list; // list of fd whith waiting datas from previous select
	mgl_status addFd(long i_fd, mgl_id i_blocid);
	mgl_eventmgr_fd *getFd(long i_fd);
	mgl_status removeFd(long i_fd);
	mgl_status selectFd(long i_delay, bool i_internalOnly=false);

};

#endif



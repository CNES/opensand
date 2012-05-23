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
/* $Id: mgl_event.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef MGL_EVENT_H
#define MGL_EVENT_H


#include "mgl_msg.h" 

// Type of events
typedef enum {
	mgl_event_type_null,
	mgl_event_type_init,
	mgl_event_type_msg,
	mgl_event_type_timer,
	mgl_event_type_fd,
	mgl_event_type_last
} mgl_event_type;

// Event msg
typedef mgl_msginfo mgl_event_msg;

// Event timer
// Timer resolution in ms
// A long egal 16M ms >= 40days
typedef struct {
	mgl_id	id;
	long duration;
	mgl_bool loop;
	long time;
	mgl_id bloc;
} mgl_event_timer;

typedef mgl_id timer;
typedef mgl_id mgl_timer;

// Event file descriptor or socket
// File descriptor + Flag interested by: data, errors, free buffer availabilility...
typedef struct {
	mgl_id blocid;
	long fd;
} mgl_event_fd;

// Overoall event structure
typedef struct {
	long type;
	union {
		mgl_event_msg	msg;
		mgl_event_timer timer;
		mgl_event_fd	fd;
	} event;
} mgl_event;


// Marshaller function prototype
mgl_event *mgl_event_msginfo_buf_to_struct(char *ip_buf, long ip_len, mgl_marshaller_buf_to_struct *ip_fct_marshaller);

// Usefull macros to test type of event
#define MGL_EVENT_IS_NULL(ip_event) (ip_event->type == mgl_event_type_null)
#define MGL_EVENT_IS_INIT(ip_event) (ip_event->type == mgl_event_type_init)
#define MGL_EVENT_IS_MSG(ip_event) (ip_event->type == mgl_event_type_msg)
#define MGL_EVENT_IS_TIMER(ip_event) (ip_event->type == mgl_event_type_timer)
#define MGL_EVENT_IS_FD(ip_event) (ip_event->type == mgl_event_type_fd)


// Usefull macros get fields of events
#define MGL_EVENT_TIMER_IS_TIMER(ip_event, i_timer) (ip_event->event.timer.id == i_timer)

#define MGL_EVENT_MSG_IS_TYPE(ip_event, i_type) (ip_event->event.msg.ptr->type== i_type)
#define MGL_EVENT_MSG_GET_BODY(ip_event) (ip_event->event.msg.ptr->pBuf)
#define MGL_EVENT_MSG_GET_BODYLEN(ip_event) (ip_event->event.msg.ptr->len)

#define MGL_EVENT_MSG_GET_SRCBLOC(ip_event) (ip_event->event.msg.srcBloc)
#define MGL_EVENT_MSG_GET_SRCPORT(ip_event) (ip_event->event.msg.srcPort)
#define MGL_EVENT_MSG_GET_DSTBLOC(ip_event) (ip_event->event.msg.dstBloc)
#define MGL_EVENT_MSG_GET_DSTPORT(ip_event) (ip_event->event.msg.dstPort)
#define MGL_EVENT_MSG_GET_TIMEIN(ip_event) (ip_event->event.msg.time_in)
#define MGL_EVENT_MSG_GET_TIMEOUT(ip_event) (ip_event->event.msg.time_out)
#define MGL_EVENT_MSG_GET_ID(ip_event) (ip_event->event.msg.id)

#define MGL_EVENT_FD_GET_FD(ip_event) (ip_event->event.fd.fd)

#endif



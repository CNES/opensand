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
/* $Id: mgl_bloc.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef MGL_BLOC_H
#define MGL_BLOC_H

#include <stdlib.h>


#include "mgl_type.h"
#include "mgl_obj.h"
#include "mgl_eventmgr.h"
#include "mgl_link.h"
#include "mgl_debug.h"
#include "mgl_msg.h"
#include "mgl_string.h"

typedef long mgl_blocid;

class mgl_blocmgr;

/**
 *   Classe that defines a bloc.
 *   A bloc is an object with default event handlers to receive timers, messages...
 *   A bloc can define several ports, to receive or send typed messages.
 */ 
class mgl_bloc: public mgl_obj {
public:
	/**
	*   Constructor.
	*   @param ip_blocmgr : Pointer of bloc manager.
	*   @param i_fatherid : Id of bloc that instanciates this bloc. 0 if no father.
	*   @param ip_name    : Name of the bloc.
	*   @param ip_type    : Type of the bloc.
	*   Constructor.
	*/ 
	mgl_bloc(mgl_blocmgr *ip_blocmgr=NULL, mgl_id i_fatherid=-1, const char *ip_name="Default", const char *ip_type="Default");

	/**
	*   Destructor
	*/
	virtual ~mgl_bloc();

	/**
	*   Hierarchical name of the bloc, including father hierarchical name.
	*/ 
	mgl_string _fullname;
	long setFullname(const char *ip_name) { return _fullname.set(ip_name); }
	const char *getFullname() { return _fullname.get(); }

	/**
	*   Type of the bloc.
	*/ 
	mgl_string _type;
	long setType(const char *ip_type) { return _type.set(ip_type); }
	const char *getType() { return _type.get(); }

	/**
	*   Unique Id of the bloc father.
	*/ 
	mgl_id _fatherid;


	/**
	*   Pointer to the event manager, set by the bloc manager before bloc initialisation.
	*/ 
	//mgl_eventmgr *_pEventMgr;
	//mgl_status setEventMgr(mgl_eventmgr *ip_pEventMgr);

	mgl_blocmgr *_pBlocMgr;
	mgl_status setBlocMgr(mgl_blocmgr *ip_pBlocMgr);

	long _blocmgrIndex;

	mgl_bool _local;
	mgl_bool isLocallyManaged() { return _local; }

	virtual mgl_status onEvent(mgl_event *ip_event) { return mgl_ko; };
	mgl_status onTerminate() { return mgl_ko; };


	long _nextId;
	void setNext(long i_id);
	long getNext();
	mgl_status executeNext();
	virtual mgl_status execute(long i_id, mgl_event *ip_event);

#define MGL_newMsg(msgId) newMsg(msgid_ ## msgId ##)

	// Allocate memory for a message and its body.
	// Set body to null, or copy from body pointer
	// If i_bodyLength==-1, search for body length in msg definition table
	mgl_msg *newMsg(long i_msgType, char *ip_msgBody=NULL, long i_bodyLength=-1);

	// Allocate memory for a message , not for its body.
	// Set body pointer from argument
	mgl_msg *newMsgWithBodyPtr(long i_msgType, void *ip_msgBody=NULL, long i_size=0);

	
#define MGL_sendMsgType(msgId, msgBody, fromPort) sendMsgType(msgid_ ## msgId ##, msgBody, fromPort)
	mgl_status MGL_copyMsgBody(char *ip_msgBodyDest, char *ip_msgBodySrc, long i_msg_id);

	mgl_status sendMsgType(long i_msgType, char *ip_msgBody=NULL, mgl_id i_fromPort=-1);

	mgl_status sendMsgTo(mgl_id i_toBloc, mgl_msg *ip_msg, mgl_id i_fromPort=-1);
	mgl_status sendMsg(mgl_msg *ip_msg, mgl_id i_fromPort=-1);
	mgl_status sendDelayedMsgTo(mgl_id i_toBloc, mgl_msg *ip_msg, long i_delay, mgl_id i_fromPort=-1);

	// Usefull feature for upper/lower layer management
	mgl_id _upperLayerBlocId;
	mgl_id _lowerLayerBlocId;
	mgl_status setLowerLayer(mgl_id i_id);
	mgl_status setUpperLayer(mgl_id i_id);
	mgl_id getUpperLayer();
	mgl_id getLowerLayer();


	mgl_status setTimer(mgl_id &i_timerid, long i_ms, mgl_bool i_loop=mgl_false);
	long getCurrentTime();

	/* fd handler */
	mgl_status addFd(long i_fd);
	mgl_status removeFd(long i_fd);

	mgl_link *registerLink(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_tobloc, mgl_id i_toport, const mgl_msgset &i_msgset, long i_delay=0, long i_bandwidth=-1);
	void registerChannelSnd(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay=0, long i_bandwidth=-1);
	void registerChannelRcv(mgl_id i_tobloc, mgl_id i_toport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay=0, long i_bandwidth=-1);

};


//#define MGL_ONEVENT_GOTO(next) onEvent_ ## next(ip_event);
#define MGL_ONEVENT_GOTO(next) setNext( ## next ##);
#define MGL_ONEVENT_MSG(msgtype, varptr, next) \
			if (ip_event->type == mgl_event_type_msg) { \
				if (ip_event->event.msg.ptr->type== msgid_ ## msgtype) {\
					varptr = ( void *)(ip_event->event.msg.ptr->pBuf);\
					onEvent_ ## next (ip_event);\
					return mgl_ok;\
				}\
			}

#define MGL_ONEVENT_SETSTATE(state) _state = state_ ## state; setNext(0);



#endif


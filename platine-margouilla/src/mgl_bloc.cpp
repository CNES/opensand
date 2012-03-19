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
/* $Id: mgl_bloc.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */





#include "mgl_bloc.h"
#include "mgl_blocmgr.h"

mgl_bloc::mgl_bloc(mgl_blocmgr *ip_blocmgr, mgl_id i_fatherid, const char *ip_name, const char *ip_type)
{
	// Store name & manager
	setBlocMgr(ip_blocmgr);
	_fatherid = i_fatherid;         
	setName(ip_name);             
	setType(ip_type);             

	// Upper and lower layers
	_upperLayerBlocId=-1;
	_lowerLayerBlocId=-1;

	// Get Id from BlocMgr          
	_blocmgrIndex=0;
	_id = ip_blocmgr->registerBloc(i_fatherid, ip_name, this); 

	// No next instruction
	_nextId=0;

	// Nam trace
	if (_pBlocMgr) if (_pBlocMgr->_pTraceNam) _pBlocMgr->_pTraceNam->init_node(_id);

}

mgl_bloc::~mgl_bloc()
{
}

/*
mgl_status mgl_bloc::setEventMgr(mgl_eventmgr *pMsgMgr) {
	_pEventMgr=pMsgMgr;
	return mgl_ok;
}
*/


mgl_status mgl_bloc::setBlocMgr(mgl_blocmgr *ip_blocMgr) {
	_pBlocMgr=ip_blocMgr;
	return mgl_ok;
}


long mgl_bloc::getNext()
{
	return _nextId;
}

void mgl_bloc::setNext(long i_id)
{
	_nextId = i_id;
}


mgl_status mgl_bloc::executeNext()
{
	return execute(_nextId, NULL);
}

mgl_status mgl_bloc::execute(long i_id, mgl_event *ip_event)
{
	return mgl_ok;
}


// Allocate memory for a message and its body.
// Set body to null, or copy from body pointer
mgl_msg *mgl_bloc::newMsg(long i_msgType, char *ip_msgBody, long i_bodyLength) 
{
	mgl_msg *lp_msg;

	// Allocated message header
	lp_msg = _pBlocMgr->allocateNewMessage(); //(mgl_msg *)malloc(sizeof(mgl_msg));
	lp_msg->type = i_msgType;

	// If a struct is associated to the msg, allocate it
	// If i_bodyLength==-1, ask Mgr for msg length
	if (i_bodyLength==-1) {
		if (!_pBlocMgr) { lp_msg->len =0; } // Problem here, please trace
		lp_msg->len = _pBlocMgr->getMsgBodySize(i_msgType);
	} else {
		lp_msg->len = i_bodyLength;
	}
	if (lp_msg->len>0) {
		lp_msg->pBuf = (char *)malloc(lp_msg->len);
		lp_msg->freeBody=1;
		if (ip_msgBody) {
			memcpy(lp_msg->pBuf, ip_msgBody, lp_msg->len);
		}
	} else {
		lp_msg->pBuf = NULL;
	}
	return lp_msg;
}

mgl_msg *mgl_bloc::newMsgWithBodyPtr(long i_msgType, void *ip_msgBody, long i_size)
{
	mgl_msg *lp_msg;

	// Allocated message header
	lp_msg = _pBlocMgr->allocateNewMessage(); //(mgl_msg *)malloc(sizeof(mgl_msg));
	lp_msg->type = i_msgType;
	lp_msg->pBuf = ip_msgBody;
	// Body memory pointer by pBuf has been allocated by the bloc
	// The event manager musn't try to free it
	lp_msg->freeBody=0; 
	lp_msg->len=i_size;
	return lp_msg;
}

mgl_status mgl_bloc::MGL_copyMsgBody(char *ip_msgBodyDest, char *ip_msgBodySrc, long i_msg_id) 
{
	long l_size;
	l_size = _pBlocMgr->getMsgBodySize(i_msg_id);
	if (l_size>0) {
		memcpy(ip_msgBodyDest, ip_msgBodySrc, l_size);
	}
	return mgl_ok;
}


mgl_status mgl_bloc::sendMsgType(long i_msgType, char *ip_msgBody, mgl_id i_fromPort)
{ 
	mgl_msg *lp_msg;

	if (!_pBlocMgr) {
		return mgl_ko; 
	} 
	lp_msg = newMsg(i_msgType, ip_msgBody);
	return _pBlocMgr->sendMsg(lp_msg, _id, i_fromPort); 
};

mgl_status mgl_bloc::sendMsgTo(mgl_id i_toBloc, mgl_msg *ip_msg, mgl_id i_fromPort)
{ 
	if (!_pBlocMgr) {
		return mgl_ko; 
	} 
	return _pBlocMgr->sendMsgTo(i_toBloc, ip_msg, _id, i_fromPort); 
};

mgl_status mgl_bloc::sendDelayedMsgTo(mgl_id i_toBloc, mgl_msg *ip_msg, long i_delay, mgl_id i_fromPort)
{ 
	if (!_pBlocMgr) {
		return mgl_ko; 
	} 
	return _pBlocMgr->sendMsgTo(i_toBloc, ip_msg, _id, i_fromPort, i_delay); 
};


mgl_status mgl_bloc::sendMsg(mgl_msg *ip_msg, mgl_id i_fromPort)
{ 
	if (!_pBlocMgr) {
		return mgl_ko; 
	} 
	return _pBlocMgr->sendMsg(ip_msg, _id, i_fromPort); 
};

mgl_status mgl_bloc::setTimer(mgl_id &i_timerid, long i_ms, mgl_bool i_loop) 
{ 
	if (!_pBlocMgr) { return mgl_ko; } 
	return _pBlocMgr->setTimer(_id, i_timerid, i_ms, i_loop); 
};


mgl_status mgl_bloc::addFd(long i_fd)
{
	if (!_pBlocMgr) { return mgl_ko; } 
	return _pBlocMgr->addFd(i_fd, _id); 
}

mgl_status mgl_bloc::removeFd(long i_fd)
{
	if (!_pBlocMgr) { return mgl_ko; } 
	return _pBlocMgr->removeFd(i_fd); 
}

mgl_link *mgl_bloc::registerLink(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_tobloc, mgl_id i_toport, const mgl_msgset &i_msgset, long i_delay, long i_bandwidth)
{
	if (!_pBlocMgr) { return NULL; } 
	return _pBlocMgr->registerLink(i_frombloc, i_fromport, i_tobloc, i_toport, i_msgset, i_delay, i_bandwidth); 
}

void mgl_bloc::registerChannelSnd(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay, long i_bandwidth)
{
	if (!_pBlocMgr) { return; } 
	_pBlocMgr->registerChannelSnd(i_frombloc, i_fromport, i_channel, i_msgset, i_delay, i_bandwidth); 
}

void mgl_bloc::registerChannelRcv(mgl_id i_tobloc, mgl_id i_toport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay, long i_bandwidth)
{
	if (!_pBlocMgr) { return; } 
	_pBlocMgr->registerChannelRcv(i_tobloc, i_toport,  i_channel, i_msgset, i_delay, i_bandwidth); 
}

long mgl_bloc::getCurrentTime() {
	if (_pBlocMgr) {
		return _pBlocMgr->getCurrentTime();
	} else {
		return 0;
	}
}

mgl_status mgl_bloc::setLowerLayer(mgl_id i_id) { _lowerLayerBlocId=i_id; if (_pBlocMgr) _pBlocMgr->registerHierachicalLink(_id, i_id); return mgl_ok; }
mgl_status mgl_bloc::setUpperLayer(mgl_id i_id) { _upperLayerBlocId=i_id; if (_pBlocMgr) _pBlocMgr->registerHierachicalLink(_id, i_id); return mgl_ok; }
mgl_id mgl_bloc::getUpperLayer() { return _upperLayerBlocId; }
mgl_id mgl_bloc::getLowerLayer() { return _lowerLayerBlocId; }

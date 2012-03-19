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
/* $Id: mgl_blocmgr.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#ifndef MGL_BLOCMGR_H
#define MGL_BLOCMGR_H

#include <stdio.h>

#include "mgl_type.h"
#include "mgl_msgset.h"
#include "mgl_eventmgr.h"
#include "mgl_list.h"
#include "mgl_bloc.h"
#include "mgl_socket.h"
#include "mgl_string.h"
#include "mgl_channel.h"


// Bloc managers network configuration: Name, Host, port
typedef struct {
		char *name;
		char *host;
		long port;
		void *pData;
} mgl_blocmgr_conf;

// List of Blocs and their bloc managers
typedef struct {
		char *name;
		char *mgr;
		long thread;
} mgl_blocmgr_blocs_conf;


class mgl_blocmgr {
public:
	long _blocIndex;

	mgl_blocmgr(char *ip_name=NULL, mgl_blocmgr_conf *ip_blocmgr_conf=NULL, mgl_blocmgr_blocs_conf *ip_blocs_conf=NULL, mgl_channeldesc *ip_channel_conf=NULL);
	~mgl_blocmgr();

	/* Bloc mgr name */
	mgl_string _name;
	long setName(const char *ip_name) { return _name.set(ip_name); }
	const char *getName() { return _name.get(); }

	/* Msg definition */
	mgl_msgdesc *_msgdesc_list;
	long getMsgBodySize(long i_msgType) {
		long l_size;
		if (!_msgdesc_list) { return 0; }
		l_size = _msgdesc_list[i_msgType].buf_len; 
		return l_size;
	}
	void setMsgDescList(mgl_msgdesc *ip_msgdesclist) {
		_msgdesc_list = ip_msgdesclist;
	}

	mgl_msg *allocateNewMessage(); 
	mgl_status sendMsgTo(mgl_id i_toBloc, mgl_msg *ip_msg, mgl_id i_fromBloc, mgl_id i_fromPort=-1, long i_delay=0);
	mgl_status sendMsg(mgl_msg *ip_msg, mgl_id i_fromBloc, mgl_id i_fromPort=-1);
	mgl_status sendMsgViaBlocMgr(mgl_event *ip_event);
	mgl_status setTimer(mgl_id i_blocid, mgl_id &i_timerid, long i_mstimer, mgl_bool i_loop=mgl_false);
	long getCurrentTime();

	/* Blocs ***/
	mgl_list	_bloc_list;
	mgl_id registerBloc(mgl_id i_fatherid, const char *ip_name, mgl_bloc *ip_bloc);
	const char *getBlocHierarchicalName(mgl_id i_id);
	mgl_id getBlocId(mgl_id i_fatherid, char *ip_name);
	mgl_bool isLocallyManaged(mgl_id i_blocid);
	mgl_bool blocIsSubBlocOf(mgl_id i_subbloc, mgl_id i_bloc);

	/* Links ***/
	mgl_list	_link_list;
	mgl_link *registerLink(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_tobloc, mgl_id i_toport, const mgl_msgset &i_msgset, long i_delay=0, long i_bandwidth=-1);
	void registerHierachicalLink(mgl_id i_upperbloc, mgl_id i_lowerBloc);

	/* Channels ***/
	mgl_channeldesc *_channel_conf;
	void registerChannelSnd(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay=0, long i_bandwidth=-1);
	void registerChannelRcv(mgl_id i_tobloc, mgl_id i_toport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay=0, long i_bandwidth=-1);
	mgl_status msgSendToChannel(mgl_id i_channel, mgl_event_msg *ip_msg, mgl_bool i_send_network);
	mgl_status setChannelsConfiguration(mgl_channeldesc *ip_conf);
	mgl_status blocmgrOpenChannelConnection();


	/* Routing fct ***/
	mgl_status msgFindDestination(mgl_event_msg *ip_msg, bool i_subBlocOnly=false);
	mgl_status msgFindFirstPort(mgl_event_msg *ip_msg);
	mgl_status msgFindFirstLink(mgl_event_msg *ip_msg, bool i_subBlocOnly=false);
	
	/* event mgr ***/
	mgl_eventmgr	*_pEventmgr;
	mgl_status setEventMgr(mgl_eventmgr	*ip_eventmgr) { _pEventmgr=ip_eventmgr; return mgl_ok; };
	mgl_eventmgr *getEventMgr() { return _pEventmgr; };
	mgl_status setEventMgrToLocallyManagedBlocs();

	/* fd ***/
	mgl_status addFd(long i_fd, mgl_id i_blocid);
	mgl_status removeFd(long i_fd);

	/* Process events ***/
	mgl_status process_init();
	mgl_status process_step();
	mgl_status process_loop();
	mgl_status process_duration(long i_ms);
	mgl_status process_terminate();


	/* states ***/
	enum { 
		state_null,
		state_initializing1,
		state_initializing2,
		state_running,
		state_terminating,
		state_terminated
	} _state;
    bool isRunning() {return (this->_state == state_running);};

	/* Managers configuration ***/
	mgl_blocmgr_conf *_blocmgr_conf;
	mgl_status setManagersConfiguration(mgl_blocmgr_conf *ip_conf);
	long mgrconfGetCount();
	char *mgrconfGetName(long i_index);
	char *mgrconfGetHost(long i_index);
	long mgrconfGetPort(long i_index);
	long mgrconfGetMgrIndex(const char *ip_name);

	/* Managers communication */
	mgl_link_tcp_server _blocmgrInput;
	long _blocmgrOutputNb;
	mgl_bool _blocmgrOutput_activated_flag;
	mgl_status blocmgrOpenInputConnection();
	mgl_status blocmgrConnectOutputConnections();
	mgl_status blocmgrWaitInputConnections();
	mgl_status blocmgrTerminateConnections();
	mgl_status blocmgrFdHandler(mgl_event_fd *ip_event_fd);

	/* List of Blocs and their managers ***/
	mgl_blocmgr_blocs_conf *_blocmgr_blocs_conf;
	mgl_status setBlocsConfiguration(mgl_blocmgr_blocs_conf *ip_conf);
	long blocGetBlocIndex(const char *ip_fullname);
	long blocGetBlocmgrIndex(const char *ip_fullname);

	/* Remote ctrl ***/
	enum {
		cmd_go,
		cmd_pause,
		cmd_step,
		cmd_terminate
	} _cmd;
	mgl_link_tcp_client _ctrl;
	mgl_status setRemoteCtrl(char *ip_host, long i_port);
	void trace(const char *ip_format,...);
	void processRemoteCmd(char *ip_cmd);

	/* Trace */
	mgl_trace *_pTrace;
	mgl_trace_file_nam *_pTraceNam;
	void setTrace(mgl_trace *ip_trace);
	void traceEventsToFile(char *ip_filename);
	void traceEventsToScreen();
	void traceNamEventsToFile(char *ip_filename);

	/* Command line arguments ***/
	void usageCommandLineArguments(char *ip_name);
	void parseCommandLineArguments(int *iop_argc, char **iop_argv);

};





#endif



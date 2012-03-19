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
/* $Id: mgl_blocmgr.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#include "mgl_blocmgr.h"
#include "mgl_debug.h"
#include "mgl_msgset.h"
#include "mgl_remote_cmd.h"

#include <stdlib.h>
#include <stdarg.h>



mgl_blocmgr::mgl_blocmgr(char *ip_name, mgl_blocmgr_conf *ip_blocmgr_conf, mgl_blocmgr_blocs_conf *ip_blocs_conf, mgl_channeldesc *ip_channel_conf) 
{
	// Internal state
	_state = state_null;
	_blocIndex=0;
	_state=state_initializing1; 
	_blocmgr_conf=ip_blocmgr_conf;
	_msgdesc_list=NULL;
	_pEventmgr=NULL;
	_blocmgrOutputNb=0;
	_blocmgr_blocs_conf=ip_blocs_conf;
	_cmd = cmd_go;
	_channel_conf=ip_channel_conf;
	_blocmgrOutput_activated_flag=mgl_true;
	_pTrace=NULL;
	_pTraceNam=NULL;
	if (ip_name) setName(ip_name);
}


mgl_blocmgr::~mgl_blocmgr() 
{
	long l_cpt;
	mgl_bloc *lp_bloc;

	for (l_cpt=0; l_cpt<_bloc_list.getCount(); l_cpt++) {
		lp_bloc = (mgl_bloc *)_bloc_list.get(l_cpt);
		if (lp_bloc) {
			delete(lp_bloc);
		} else {
			MGL_WARNING(MGL_CTX, "Blocmgr: One bloc is badly referenced (null ptr)\n");
		}
	}
	if (_pTrace) _pTrace->close();

}

mgl_status mgl_blocmgr::setRemoteCtrl(char *ip_host, long i_port)
{
	// Remote control connection
	if (i_port>0) {
		printf("Try to connect to %s:%ld\n", ip_host, i_port);
		_ctrl.init(ip_host, i_port);
		while (_ctrl.connectToServer()<=0) {
			mgl_sleep::sleep(1000);
			printf("Try to connect to %s:%ld\n", ip_host, i_port);
		}
		printf("Connected.\n");

		// MSC trace: Mgr name
		trace("Bloc manager [%s](%d) connected.", _name.get(), _blocIndex);

		// Ask event mgr to handle it
		_pEventmgr->addFd(_ctrl._fd, -4);

		// Pause at start
		_cmd = cmd_pause;

	}

	return mgl_ok;
}


// Blocs
mgl_id mgl_blocmgr::registerBloc(mgl_id i_fatherid, const char *ip_name, mgl_bloc *ip_bloc)
{ 
	mgl_bloc *lp_bloc;

	_bloc_list.append(ip_bloc);

	// Set full name
	ip_bloc->setName(ip_name);
	if (i_fatherid==0) {
		ip_bloc->setFullname(ip_bloc->getName());
	} else {
		lp_bloc = (mgl_bloc *)_bloc_list.get(i_fatherid-1); 
		if (lp_bloc) {
			ip_bloc->setFullname(lp_bloc->getName());
			ip_bloc->_fullname.append("/");
			ip_bloc->_fullname.append(ip_name);
		} else {
			MGL_TRACE(MGL_CTX, MGL_TRACE_CRITICAL, "registerBloc: invalid father Id\n");
			MGL_DEBUGGER();
		}
	}
	
	// Set manager index
	ip_bloc->_blocmgrIndex = blocGetBlocmgrIndex(ip_bloc->getFullname());
	if (mgrconfGetMgrIndex(getName())==ip_bloc->_blocmgrIndex) {
		ip_bloc->_local = mgl_true;
	} else {
		ip_bloc->_local = mgl_false;
	}

	MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Registered Bloc [%d](%s)(%s) Father(%d) Mgr(%d)\n", _bloc_list.getCount(), ip_bloc->getName(), ip_bloc->getFullname(), i_fatherid, ip_bloc->_blocmgrIndex);

	return _bloc_list.getCount(); 
}


const char *mgl_blocmgr::getBlocHierarchicalName(mgl_id i_id)
{
	return "";
}


mgl_id mgl_blocmgr::getBlocId(mgl_id i_fatherid, char *ip_name)
{ 
	return 2; 
};

mgl_bool mgl_blocmgr::isLocallyManaged(mgl_id i_blocid) 
{ 
	mgl_bloc *lp_bloc;

	lp_bloc = (mgl_bloc *)_bloc_list.get(i_blocid-1);
	if (lp_bloc) {
		return lp_bloc->isLocallyManaged();
	}
	return mgl_true; 
};

// Links
mgl_link *mgl_blocmgr::registerLink(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_tobloc, mgl_id i_toport, const mgl_msgset &i_msgset, long i_delay, long i_bandwidth) 
{ 
	mgl_link *lp_link;

	lp_link = new mgl_link();
	lp_link->srcBloc = i_frombloc;
	lp_link->srcPort = i_fromport;
	lp_link->dstBloc = i_tobloc;
	lp_link->dstPort = i_toport;
	lp_link->msgset = i_msgset;
	lp_link->delay = i_delay;
	lp_link->bandwidth = i_bandwidth;
	_link_list.append(lp_link);

	MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Registered link from %d:%d To %d:%d msgset (delay=%d ms, bandwidth=%d Bps).\n", i_frombloc, i_fromport, i_tobloc, i_toport, i_delay, i_bandwidth);

	// Nam trace
	if (_pTraceNam) _pTraceNam->init_link(i_frombloc, i_tobloc);

	return lp_link; 
};

void mgl_blocmgr::registerHierachicalLink(mgl_id i_upperBloc, mgl_id i_lowerBloc)
{
	// Nam trace
	if (_pTraceNam) _pTraceNam->init_link(i_upperBloc, i_lowerBloc);
};



mgl_status mgl_blocmgr::setEventMgrToLocallyManagedBlocs()
{
/* No more in use
	long l_cpt;
	mgl_bloc *lp_bloc;

	if (!_pEventmgr) { 
		MGL_WARNING(MGL_CTX, "Try to set bloc's eventMgr when eventMgr is not instanciated\n");
		return mgl_ko; 
	}

	for (l_cpt=0; l_cpt<_bloc_list.getCount(); l_cpt++) {
		lp_bloc = (mgl_bloc *)_bloc_list.get(l_cpt);
		if (lp_bloc) {
			lp_bloc->setEventMgr(_pEventmgr);
		} else {
			MGL_WARNING(MGL_CTX, "Blocmgr: One bloc is badly referenced (null ptr)\n");
			return mgl_ko;
		}
	}
*/
	return mgl_ok;
}


mgl_status mgl_blocmgr::msgFindFirstPort(mgl_event_msg *ip_msg)
{ 
	long l_cpt;
	mgl_link *lp_link;
	
	for (l_cpt=0; l_cpt<_link_list.getCount(); l_cpt++) {
		lp_link = (mgl_link *)_link_list.get(l_cpt);
		if (lp_link) {
			if (lp_link->srcBloc==ip_msg->srcBloc) {
				if (lp_link->msgset.msgIdIsIn(ip_msg->ptr->type)) {
					ip_msg->srcPort = lp_link->srcPort;
					return mgl_ok;
				}
			}
		} 
	}
	
	return mgl_ko;
}

mgl_status mgl_blocmgr::msgFindFirstLink(mgl_event_msg *ip_msg, bool i_subBlocOnly) 
{ 
	long l_nb;
	long l_cpt;
	mgl_link *lp_link;
	long l_time;
	long l_in_time;
	float l_size_in_bits;
	float l_throughput_in_bits_pec_second;
	float l_duration_ms;
	long l_duration;
	long l_last_bandwidth_out_time;
	long l_len;

	// For each link in the list
	l_nb = _link_list.getCount();
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		lp_link = (mgl_link *)_link_list.get(l_cpt);
		if (lp_link) {
			// Look for Src(Bloc:Port)
			if (lp_link->srcBloc==ip_msg->srcBloc) {
				// If msg specify no port: msg.port=0
				// If link can catch all msg: link.port=0
				if (  (ip_msg->srcPort==0)
					||(lp_link->srcPort==ip_msg->srcPort) 
					||(lp_link->srcPort==0)) 
				{
					// If the msgtype match, then use this link to get Dest(Bloc:Port)
					if (lp_link->msgset.msgIdIsIn(ip_msg->ptr->type)) {
						if  (  (i_subBlocOnly&&(blocIsSubBlocOf(lp_link->dstBloc, ip_msg->srcBloc)))
							 ||(!i_subBlocOnly) 
							) {
							ip_msg->dstBloc = lp_link->dstBloc;
							ip_msg->dstPort = lp_link->dstPort;
							// This link adds delay ?
							if ((lp_link->delay>0)&&(lp_link->delay!=-1)) {
								ip_msg->time_out += lp_link->delay;
							}
							// This link got bandwidth limitations ?
							// Seb: Check this algo: corrupted by delay: xxx  xxx  xxx
							l_time=_pEventmgr->getCurrentTime();
							if ((lp_link->bandwidth>0)&&(lp_link->bandwidth!=-1)) {
								l_last_bandwidth_out_time = lp_link->bandwidth_out_time;
								l_in_time = mgl_max(lp_link->bandwidth_out_time, l_time);
								l_size_in_bits = ((float)(4+ip_msg->ptr->len))*8;
								l_throughput_in_bits_pec_second = (float)(lp_link->bandwidth);
								l_duration_ms = (l_size_in_bits/l_throughput_in_bits_pec_second)*1000; 
								l_duration = (long)l_duration_ms;
								lp_link->bandwidth_out_time+= l_duration;
								ip_msg->time_out += (lp_link->bandwidth_out_time-l_time);
							} else {
								lp_link->bandwidth_out_time = l_time;
							}
							// Check buffer limitation ?

							// Save some stats ?
							if (lp_link->stats_fd) {
								if ((lp_link->bandwidth>0)&&(lp_link->bandwidth!=-1)) {
									for (l_cpt=l_last_bandwidth_out_time; l_cpt<l_in_time; l_cpt+=lp_link->stats_period) {
										fprintf(lp_link->stats_fd, "%ld, 0\n", l_cpt);
									}
									l_len = (long)(l_throughput_in_bits_pec_second*((float)lp_link->stats_period)/(float)1000);
									for (l_cpt=l_in_time; l_cpt<lp_link->bandwidth_out_time; l_cpt+=lp_link->stats_period) {
										fprintf(lp_link->stats_fd, "%ld, %ld\n", l_cpt, l_len);
									}
								} else {
									for (l_cpt=l_last_bandwidth_out_time; l_cpt<l_in_time; l_cpt+=lp_link->stats_period) {
										fprintf(lp_link->stats_fd, "%ld, 0\n", l_cpt);
									}
									fprintf(lp_link->stats_fd, "%ld, %ld\n", lp_link->bandwidth_out_time, (4+ip_msg->ptr->len));
								}
							}
							fflush(lp_link->stats_fd);
							return mgl_ok;
						}
					}
				}
			}
		} 
	}
	return mgl_ko;
}

mgl_bool mgl_blocmgr::blocIsSubBlocOf(mgl_id i_subbloc, mgl_id i_bloc)
{
	mgl_bool l_ret;
	// Get sub bloc
	mgl_bloc *lp_subbloc;
	lp_subbloc = (mgl_bloc *)_bloc_list.get(i_bloc);
	if (!lp_subbloc) return mgl_false;

	// Check is subloc
	l_ret = (mgl_bool)(lp_subbloc->_fatherid==i_bloc);
	return l_ret;
}


mgl_status mgl_blocmgr::msgFindDestination(mgl_event_msg *iop_msg, bool i_subBlocOnly)
{
	mgl_event_msg l_msg;
	mgl_status l_ret;


	// The msg is sent by a bloc
	MGL_TRACE(MGL_CTX, MGL_TRACE_ROUTING, "msgFindDestination: Src (%d:%d) Dst (%d:%d)\n", 
		iop_msg->srcBloc, iop_msg->srcPort, iop_msg->dstBloc, iop_msg->dstPort);

	// Do we know the port ?
	if (iop_msg->srcPort==-1) {
		msgFindFirstPort(iop_msg);
	}
	if (iop_msg->srcPort==-1) {
		// Warning
		MGL_WARNING(MGL_CTX, "Blocmgr: Can't find an out port for msg (%d) from bloc (%d)\n", iop_msg->ptr->type, iop_msg->srcBloc);
		return mgl_ko;
	}

	// Then find the link
	msgFindFirstLink(iop_msg, i_subBlocOnly);
	MGL_TRACE(MGL_CTX, MGL_TRACE_ROUTING, "msgFindDestination: After FindFirstLink : Src (%d:%d) Dst (%d:%d)\n", 
		iop_msg->srcBloc, iop_msg->srcPort, iop_msg->dstBloc, iop_msg->dstPort);
	if (iop_msg->dstBloc==-1) {
		// Warning, but only for the first try..
		//MGL_WARNING("Blocmgr: Can't find an destination (Bloc:Port) for msg (%d) from bloc (%d)\n", ip_msg->ptr->type, ip_msg->srcBloc);
		return mgl_ko;
	}
	if (iop_msg->dstBloc<=-2) {
		// Msg sent to a Multicast channel
		return mgl_ok;
	}

	// Now, we have found a destination Bloc.
	// If the destination is not the sender, (avoid infinite loop),
	// the message may be redirected to a sub bloc inside this bloc
	//if (blocIsSubBlocOf(iop_msg->dstBloc, iop_msg->srcBloc)) {
	if (iop_msg->dstBloc != iop_msg->srcBloc) {
		l_msg.ptr = iop_msg->ptr; // let point to the same msg
		l_msg.srcBloc = iop_msg->dstBloc;
		l_msg.srcPort = iop_msg->dstPort;
		l_msg.dstBloc = -1;
		l_msg.dstPort = -1;
		l_msg.time_out = iop_msg->time_out;
		l_ret = msgFindDestination(&l_msg, true);
		if (l_ret == mgl_ok) {  // Ok, a sub bloc has been found as destination
			iop_msg->dstBloc = l_msg.dstBloc;
			iop_msg->dstPort = l_msg.dstPort;
			iop_msg->time_out = l_msg.time_out;
		}
	}
	MGL_TRACE(MGL_CTX, MGL_TRACE_ROUTING, "msgFindDestination: Final : Src (%d:%d) Dst (%d:%d)\n", 
		iop_msg->srcBloc, iop_msg->srcPort, iop_msg->dstBloc, iop_msg->dstPort);

	return mgl_ok;
}

mgl_status mgl_blocmgr::process_init()
{
	long l_cpt;
	mgl_bloc *lp_bloc;
	mgl_event l_event;

	// Init each locally managed bloc
	l_event.type = mgl_event_type_init;
	for (l_cpt=0; l_cpt<_bloc_list.getCount(); l_cpt++) {
		lp_bloc = (mgl_bloc *)_bloc_list.get(l_cpt);
		if (lp_bloc) {
			lp_bloc->onEvent(&l_event);
			// The event has been processed, lets check for another 'next' instruction
			while (lp_bloc->getNext()>0) {
				if (!lp_bloc->executeNext()) { 
					// Warning, next bad value !!
					break; 
				}
			}
		} else {
			MGL_WARNING(MGL_CTX, "Blocmgr: One bloc is wrongly referenced (null ptr)\n");
			return mgl_ko;
		}
	}
	return mgl_ok;
}

mgl_bloc	*lp_bloc=NULL;
long g_initialized_bloc=0;
mgl_status mgl_blocmgr::process_step()
{
	mgl_event *lp_event;
	mgl_status	l_ret;
	mgl_event l_event;
	long l_nb;
	long l_cpt;
	mgl_bloc	*lp_bloc2;

	// Check wether a next instruction must be executed on the last bloc
	if ((_cmd==cmd_go)||(_cmd==cmd_step)) {
		if (lp_bloc) {
			if (lp_bloc->getNext()>0) {
				l_ret = lp_bloc->executeNext();
				if (_cmd==cmd_step) { _cmd=cmd_pause; }
				if (l_ret==mgl_ko) {
					MGL_TRACE(MGL_CTX, MGL_TRACE_CRITICAL, "Pb, Bloc (%d) next event (%d) not found. Abort\nBloc Type (%s).", 
						lp_bloc->getId(), lp_bloc->getNext(), lp_bloc->getType());
					MGL_DEBUGGER();
				}
				// MSC trace
				if (lp_bloc->getNext()>0) {
					trace("Next instruction: Bloc(%d) Instruction(%d) Bloc type:%s", lp_bloc->getId(), lp_bloc->getNext(), lp_bloc->getType());
				} else {
					lp_bloc=NULL;
				}
				return mgl_ok;
			}
		}
	}

	// Check state
	switch (_state) {
	case state_null: 
		return mgl_ko;
		break;
	case state_initializing1:
		// open channels
		l_ret = blocmgrOpenChannelConnection();

		// Connect to remote bloc mgr
		if (_blocmgrOutput_activated_flag==mgl_true) {
			MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Initializing connections between managers\n");
			l_ret = blocmgrOpenInputConnection();
			l_ret = blocmgrConnectOutputConnections();
			l_ret = blocmgrWaitInputConnections();
		} else {
			MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Connections between managers desactivated\n");
		}
		// List the Blocs
		l_nb = _bloc_list.getCount();
		for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
			lp_bloc2 = (mgl_bloc *)_bloc_list.get(l_cpt);
			if (lp_bloc2) {
				// Trace
				trace("New bloc instance [%d](%s)(%s)(%s) Mgr(%d).", l_cpt+1, lp_bloc2->getName(), lp_bloc2->getType(), lp_bloc2->getFullname(), lp_bloc2->_blocmgrIndex);

			}
		}
		_state=state_initializing2; 
		MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Initializing locally managed blocs\n");
		
	case state_initializing2:

		// Initialise local blocs
		
		//l_ret = process_init(); 
		if ((_cmd==cmd_go)||(_cmd==cmd_step)) {
			l_event.type = mgl_event_type_init;
			lp_bloc = (mgl_bloc *)_bloc_list.get(g_initialized_bloc);
			g_initialized_bloc++;
			if (lp_bloc) {
				// MSC trace
				trace("Initializing bloc(%d): %s.", g_initialized_bloc, lp_bloc->getName());
				lp_bloc->onEvent(&l_event);
				if (_cmd==cmd_step) { _cmd=cmd_pause; }
				if (lp_bloc->getNext()>0) {
					trace("Next instruction: Bloc(%d) Instruction(%d) Bloc type:%s", lp_bloc->getId(), lp_bloc->getNext(), lp_bloc->getType());
				}
			} else {
				MGL_WARNING(MGL_CTX, "Blocmgr: One bloc is wrongly referenced (null ptr)\n");
				//return mgl_ko;
			}

			// Let's rock
			if (g_initialized_bloc>=_bloc_list.getCount()) {
				_state=state_running; 
				MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Running\n");
			}
		}
		if (_cmd==cmd_pause) {
			lp_event = _pEventmgr->getNextInternalEvent(500);
			if (lp_event) {
				if (lp_event->type== mgl_event_type_fd) {
					if (lp_event->event.fd.blocid<0) {
						blocmgrFdHandler(&(lp_event->event.fd));
					} 
				}
			}
		}
		return mgl_ok; 
		break;
	case state_running:

		// Check wether an event is available
		if ((_cmd==cmd_go)||(_cmd==cmd_step)) {
			lp_event = _pEventmgr->getNextEvent(500);
		}
		if (_cmd==cmd_pause) {
			lp_event = _pEventmgr->getNextInternalEvent(500);
		}

		// Dispatch the pointer to it.
		// Remember: the pointer & event memory is managed by eventMgr
		// Don't free it
		if (lp_event) {
			switch (lp_event->type) {
			case mgl_event_type_msg: 

				// Find the destination
				//msgFindDestination(&(lp_event->event.msg));
				if (lp_event->event.msg.dstBloc==-1) {
					// Warning,
					MGL_WARNING(MGL_CTX, "Blocmgr::process_step: Can't find an destination (Bloc:Port) for msg (%d) from bloc (%d)\n", lp_event->event.msg.ptr->type, lp_event->event.msg.srcBloc);
					break;
				}

				// Is destination bloc local ?
				if (!isLocallyManaged(lp_event->event.msg.dstBloc)) {
					// Send msg via remote bloc manager
					l_ret = sendMsgViaBlocMgr(lp_event);
					break;
				}

				// Is destination a channel ?
				if ((lp_event->event.msg.dstBloc)<=-2) {
					// Should never append !!
					// Send msg via channel socket
					//l_ret = msgSendToChannel(-lp_event->event.msg.dstBloc-1, &(lp_event->event.msg));
					break;
				}

				// MSC trace
				trace("Event Msg Received: msg(%d) from %d to %d at %d", lp_event->event.msg.id, lp_event->event.msg.srcBloc, lp_event->event.msg.dstBloc, lp_event->event.msg.time_out);

				// Nam trace
				if (_pTraceNam) _pTraceNam->receive(lp_event->event.msg.time_out, lp_event->event.msg.srcBloc, lp_event->event.msg.dstBloc, "msg", lp_event->event.msg.ptr->len, lp_event->event.msg.id);

				// Send msg to local destination
				lp_bloc = (mgl_bloc *)_bloc_list.get(lp_event->event.msg.dstBloc-1);
				if (!lp_bloc) { 
					MGL_WARNING (MGL_CTX, "mgl_blocmgr::process_step can't get bloc %d pointer\n", lp_event->event.msg.dstBloc);
				}
				if (!lp_bloc->onEvent(lp_event)) {
					MGL_WARNING (MGL_CTX, "mgl_blocmgr::process_step : Bloc %d can't handle event. Bloc type:%s\n", lp_event->event.msg.dstBloc, lp_bloc->getType());
				}
				if (_cmd==cmd_step) { _cmd=cmd_pause; }

				// MSC trace
				if (lp_bloc->getNext()>0) {
					trace("Next instruction: Bloc(%d) Instruction(%d) Bloc type:%s", lp_bloc->getId(), lp_bloc->getNext(), lp_bloc->getType());
				}

				break;
			case mgl_event_type_timer: 
				// MSC trace
				trace("Event Timer Expired: timer(%d) at Bloc %d at %d.", lp_event->event.timer.id, lp_event->event.timer.bloc, lp_event->event.timer.time);

				// Send timer to destination
				lp_bloc = (mgl_bloc *)_bloc_list.get(lp_event->event.timer.bloc-1);
				if (!lp_bloc) { 
					MGL_WARNING (MGL_CTX, "mgl_blocmgr::process_step can't get bloc %d pointer\n", lp_event->event.timer.bloc);
				}
				lp_bloc->onEvent(lp_event);
				if (_cmd==cmd_step) { _cmd=cmd_pause; }

				// MSC trace
				if (lp_bloc->getNext()>0) {
					trace("Next instruction: Bloc(%d) Instruction(%d) Bloc type:%s", lp_bloc->getId(), lp_bloc->getNext(), lp_bloc->getType());
				}
				break;
			case mgl_event_type_fd: 
				// blocmgr internal fd ?
				if (lp_event->event.fd.blocid<0) {
					blocmgrFdHandler(&(lp_event->event.fd));
				} else {
					// Send event to destination
					lp_bloc = (mgl_bloc *)_bloc_list.get(lp_event->event.fd.blocid-1);
					if (!lp_bloc) { 
						MGL_WARNING (MGL_CTX, "mgl_blocmgr::process_step can't get bloc %d pointer\n", lp_event->event.timer.bloc);
					}
					lp_bloc->onEvent(lp_event);
					if (_cmd==cmd_step) { _cmd=cmd_pause; }
					// MSC trace
					trace("Event Fd at bloc(%d).", lp_event->event.fd.blocid);
					// MSC trace
					if (lp_bloc->getNext()>0) {
					trace("Next instruction: Bloc(%d) Instruction(%d) Bloc type:%s", lp_bloc->getId(), lp_bloc->getNext(), lp_bloc->getType());
					}
				}
				break;
			default: 
//					trace("Warning unknown event\n");
				break;
			}
		}
		// Now, ask the eventMgr to free it
		_pEventmgr->freeEvent(lp_event);

		return mgl_ok;
	case state_terminating:
		MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Terminating\n");
		l_ret = process_terminate();
		_state=state_terminated; 
		return mgl_ok;
	case state_terminated:
		MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Terminated\n");
		return mgl_ok;
	}
	// Warning Pb internal state
	return mgl_ko;
}


mgl_status mgl_blocmgr::process_terminate()
{
	long l_cpt;
	mgl_bloc *lp_bloc;

	for (l_cpt=0; l_cpt<_bloc_list.getCount(); l_cpt++) {
		lp_bloc = (mgl_bloc *)_bloc_list.get(l_cpt);
		if (lp_bloc) {
			lp_bloc->onTerminate();
		} else {
			MGL_WARNING(MGL_CTX, "Blocmgr: One bloc is badly referenced (null ptr)\n");
			return mgl_ko;
		}
	}
	return mgl_ok;
}

mgl_status mgl_blocmgr::sendMsgTo(mgl_id i_toBloc, mgl_msg *ip_msg, mgl_id i_fromBloc, mgl_id i_fromPort, long i_delay)
{
	mgl_msginfo *lp_msginfo;
	mgl_status l_ret;

	// Enqueue it in event mgr
	if (!_pEventmgr) { return mgl_ko; } 
	lp_msginfo = _pEventmgr->sendMsg(ip_msg, i_fromBloc, i_fromPort, i_delay); 

	// Calculate the links delay, and destination bloc
	lp_msginfo->dstBloc=i_toBloc;
	//MGL_TRACE(MGL_CTX, MGL_TRACE_INFO, "sendMsg: Msg out time after routing %d\n", lp_msginfo->time);

	if (lp_msginfo->dstBloc==-1) {
		// Warning,
		MGL_WARNING(MGL_CTX, "Blocmgr::sendMsg: Can't find a destination (Bloc:Port) for msg (%d) from bloc (%d)\n", 
			lp_msginfo->ptr->type, lp_msginfo->srcBloc);
	}

	if (lp_msginfo->dstBloc<=-2) {
		// Msg sent to a multicast channel
		l_ret = msgSendToChannel(-lp_msginfo->dstBloc-1, lp_msginfo, mgl_true);
		MGL_TRACE(MGL_CTX, MGL_TRACE_INFO, "Event Msg Sent: msg(%d) type %d from %d to channel %d at %d ()", lp_msginfo->id, ip_msg->type, i_fromBloc, -1-lp_msginfo->dstBloc, lp_msginfo->time_out);
	} 
	if (lp_msginfo->dstBloc>0) {
		// MSC trace
		trace("Event Msg Sent: msg(%d) type %d from %d to %d at %d ()", lp_msginfo->id, ip_msg->type, i_fromBloc, lp_msginfo->dstBloc, lp_msginfo->time_out);
		// Nam trace
		if (_pTraceNam) _pTraceNam->send(lp_msginfo->time_out, i_fromBloc, lp_msginfo->dstBloc, "msg", lp_msginfo->ptr->len, lp_msginfo->id);
	}

	// Order messages depending on output time
	//_pEventmgr->sortMsgList();
	return mgl_ok;
}

mgl_status mgl_blocmgr::sendMsg(mgl_msg *ip_msg, mgl_id i_fromBloc, mgl_id i_fromPort)
{
	mgl_msginfo *lp_msginfo;
	mgl_status l_ret;

	// Enqueue it in event mgr
	if (!_pEventmgr) { return mgl_ko; } 
	lp_msginfo = _pEventmgr->sendMsg(ip_msg, i_fromBloc, i_fromPort); 

	// Calculate the links delay, and destination bloc
	msgFindDestination(lp_msginfo);
	//MGL_TRACE(MGL_CTX, MGL_TRACE_INFO, "sendMsg: Msg out time after routing %d\n", lp_msginfo->time);

	if (lp_msginfo->dstBloc==-1) {
		// Warning,
		MGL_WARNING(MGL_CTX, "Blocmgr::sendMsg: Can't find a destination (Bloc:Port) for msg (%d) from bloc (%d)\n", 
			lp_msginfo->ptr->type, lp_msginfo->srcBloc);
	}

	if (lp_msginfo->dstBloc<=-2) {
		// Msg sent to a multicast channel
		l_ret = msgSendToChannel(-lp_msginfo->dstBloc-1, lp_msginfo, mgl_true);
		MGL_TRACE(MGL_CTX, MGL_TRACE_INFO, "Event Msg Sent: msg(%d) type %d from %d to channel %d at %d ()", lp_msginfo->id, ip_msg->type, i_fromBloc, -1-lp_msginfo->dstBloc, lp_msginfo->time_out);
	} 
	if (lp_msginfo->dstBloc>0) {
		// MSC trace
		trace("Event Msg Sent: msg(%d) type %d from %d to %d at %d ()", lp_msginfo->id, ip_msg->type, i_fromBloc, lp_msginfo->dstBloc, lp_msginfo->time_out);
		// Nam trace
		if (_pTraceNam) _pTraceNam->send(lp_msginfo->time_out, i_fromBloc, lp_msginfo->dstBloc, "msg", lp_msginfo->ptr->len, lp_msginfo->id);
	}

	// Order messages depending on output time
	//_pEventmgr->sortMsgList();
	return mgl_ok;
}


/* Managers configuration ***/
mgl_status mgl_blocmgr::setManagersConfiguration(mgl_blocmgr_conf *ip_conf)
{
	_blocmgr_conf=ip_conf;
	return mgl_ok;
}


long mgl_blocmgr::mgrconfGetCount()
{
	long l_nb;
	if (!_blocmgr_conf) { return 0; }
	l_nb = 0;
	while (_blocmgr_conf[l_nb].name) { l_nb++; }
	return l_nb;
}

char *mgl_blocmgr::mgrconfGetName(long i_index)
{
	if (!_blocmgr_conf) { return NULL; }
	if (i_index>=mgrconfGetCount()) { return NULL; }
	return _blocmgr_conf[i_index].name;
}

char *mgl_blocmgr::mgrconfGetHost(long i_index)
{
	if (!_blocmgr_conf) { return NULL; }
	if (i_index>=mgrconfGetCount()) { return NULL; }
	return _blocmgr_conf[i_index].host;
}

long mgl_blocmgr::mgrconfGetPort(long i_index)
{
	if (!_blocmgr_conf) { return 0; }
	if (i_index>=mgrconfGetCount()) { return 0; }
	return _blocmgr_conf[i_index].port;
}

long mgl_blocmgr::mgrconfGetMgrIndex(const char *ip_name)
{
	long l_nb;
	if (!_blocmgr_conf) { return 0; }
	if (!ip_name) { return 0; }
	l_nb = 0;
	while (_blocmgr_conf[l_nb].name) { 
		if (strcmp(_blocmgr_conf[l_nb].name, ip_name)==0) {
			return l_nb;
		}
		l_nb++; 
	}
	return 0;
}

mgl_status mgl_blocmgr::blocmgrOpenInputConnection()
{
	long l_index;
	long l_port;
	long l_ret;

	// There is a need to extablish connection between bloc mgrs
	// only when there are more than one :)
	if (mgrconfGetCount()<2) { return mgl_ok; }
	
	// Open server socket
	l_index = mgrconfGetMgrIndex(getName());
	l_port = mgrconfGetPort(l_index);
	MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Opening Bloc manager input port=%d\n", l_port);
	if (l_port<=0) {
		MGL_WARNING(MGL_CTX, "Invalid network configuration.\n");
		return mgl_ko;
	}
	l_ret = _blocmgrInput.init(l_port);
	if (l_ret) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_INFO, "Listening on socket %d\n", l_ret);
	} else {
		MGL_CRITICAL(MGL_CTX, "Can't open input socket\n");
		exit(1);
	}

	// Ask event mgr to handle it
	_pEventmgr->addFd(l_ret, -1);
	return mgl_ok;
}

mgl_status mgl_blocmgr::blocmgrConnectOutputConnections()
{
	long l_nb;
	long l_cpt;
	long l_index;
	char *lp_name;
	char *lp_host;
	long l_port;
	long l_ret;
	mgl_link_tcp_client *lp_client;

	// There is a need to extablish connection between bloc mgrs
	// only when there are more than one :)
	l_nb = mgrconfGetCount();
	if (l_nb<2) { return mgl_ok; }

	// Get my index
	l_index = mgrconfGetMgrIndex(getName());

	// Connect to other servers
	_blocmgrOutputNb=0;
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		if (l_cpt!=l_index) {
			lp_name = mgrconfGetName(l_cpt);
			lp_host = mgrconfGetHost(l_cpt);
			l_port = mgrconfGetPort(l_cpt);
			MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Blocmgr [%s]: host=%s, port=%d: Connecting...", lp_name, lp_host, l_port);
			if ((!lp_host)||(!l_port)) {
				MGL_WARNING(MGL_CTX, "Invalid network configuration.\n");
				return mgl_ko;
			}
			lp_client = new mgl_link_tcp_client();
			_blocmgr_conf[l_cpt].pData = (void *)lp_client;
			do {
				MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, ".");
				_blocmgrInput.acceptNewConnection(1,0);
				l_ret = lp_client->init(lp_host, l_port);
			} while (l_ret==0);
			_blocmgrOutputNb++;
			MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, " Connected.\n");

			// Ask event mgr to handle it
			_pEventmgr->addFd(l_ret, -2);

		}
	}
	return mgl_ok;
}

mgl_status mgl_blocmgr::blocmgrWaitInputConnections()
{
	long l_nb;
	long l_cpt;

	// If network is enabled
	if (!(_blocmgrInput.isOpened())) { return mgl_ko; }

	// Waiting for a complete connected scheme between hosts
	l_nb = mgrconfGetCount()-1;
	MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Remote bloc managers. Waiting for complete connection...");
	while (_blocmgrInput.getClientCount()<l_nb) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, ".");
		_blocmgrInput.acceptNewConnection(1,0);
	}
	MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, " Connected.\n");

	// Ask event mgr to handle them
	for (l_cpt=1; l_cpt<=l_nb; l_cpt++) {
		_pEventmgr->addFd(_blocmgrInput._fd[l_cpt], -3);
	}

	return mgl_ok;
}

mgl_status mgl_blocmgr::blocmgrTerminateConnections()
{
	long l_nb;
	long l_cpt;
	mgl_link_tcp_client *lp_client;

	MGL_TRACE(MGL_CTX, MGL_TRACE_INFO, "Disconnecting from remote managers\n");
	// Disconnect from servers
	l_nb = _blocmgrOutputNb;
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		lp_client = (mgl_link_tcp_client *)_blocmgr_conf[l_cpt].pData;
		if (lp_client) { lp_client->close(); }
	}

	// Disconnect clients
	_blocmgrInput.close();

	return mgl_ok;
}


////////////////////////////////////////////////////////////////
//
/* List of Blocs and their managers ***/
//
mgl_status mgl_blocmgr::setBlocsConfiguration(mgl_blocmgr_blocs_conf *ip_conf)
{
	_blocmgr_blocs_conf = ip_conf;
	return mgl_ok;
}

long mgl_blocmgr::blocGetBlocIndex(const char *ip_fullname)
{
	long l_nb;
	if (!_blocmgr_conf) { return -1; }
	if (!_blocmgr_blocs_conf) { return -1; }
	if (!ip_fullname) { return -1; }

	// Get bloc line
	l_nb = 0;
	while (_blocmgr_blocs_conf[l_nb].name) { 
		if (strcmp(_blocmgr_blocs_conf[l_nb].name, ip_fullname)==0) {
			return l_nb;
		}
		l_nb++; 
	}

	return -1;
}


long mgl_blocmgr::blocGetBlocmgrIndex(const char *ip_fullname)
{
	long l_bloc_index;
	long l_nb;
	if (!_blocmgr_conf) { return 0; }
	if (!_blocmgr_blocs_conf) { return 0; }
	if (!ip_fullname) { return 0; }

	// Get bloc line
	l_bloc_index = blocGetBlocIndex(ip_fullname);
	// Not in conf ? Then managed by default mgr
	if (l_bloc_index<0) {
		return 0;
	}
	l_nb=0;
	while (_blocmgr_conf[l_nb].name) { 
		if (strcmp(_blocmgr_conf[l_nb].name, _blocmgr_blocs_conf[l_bloc_index].mgr)==0) {
			return l_nb;
		}
		l_nb++; 
	}

	return 0; // Default bloc mgr
}


mgl_status mgl_blocmgr::sendMsgViaBlocMgr(mgl_event *ip_event)
{
	long l_index;
	mgl_bloc *lp_bloc;
	mgl_link_tcp_client *lp_client;
	long l_ret;
	char l_buf[8002]="";
	long l_len;

	if (ip_event->type==mgl_event_type_msg) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_MGR_CX, "Sending a Msg to a remote manager\n");

		// Get index of bloc manager managing the destination bloc
		lp_bloc = (mgl_bloc *)_bloc_list.get(ip_event->event.msg.dstBloc-1);
		l_index = lp_bloc->_blocmgrIndex;
		if (l_index<0) {
			MGL_WARNING(MGL_CTX, "Mgr not identified for bloc (%d). Msg lost\n", ip_event->event.msg.dstBloc);
			return mgl_ko;
		}

		MGL_TRACE(MGL_CTX, MGL_TRACE_MGR_CX, "Sent: msg to mgr (%d): bloc (%d)\n", l_index, ip_event->event.msg.dstBloc);

		// Is connection available ?
		lp_client = (mgl_link_tcp_client *)_blocmgr_conf[l_index].pData;
		if (!lp_client) {
			MGL_WARNING(MGL_CTX, "Connection to Mgr (%d) down. Msg lost\n", l_index);
			return mgl_ko;
		}

		// Get msg marshaller 

		// Serialize msg
		l_len = 8000;
		l_len = mgl_msginfo_struct_to_buf(l_buf, &l_len, &(ip_event->event.msg), NULL);
		if (l_len<=0) {
			MGL_WARNING(MGL_CTX, "Can't serialize msg. Msg lost\n");
			return mgl_ko;
		}

		// Send msg
		l_ret = lp_client->snd_pkt(l_buf, l_len);
		if (!l_ret) {
			MGL_WARNING(MGL_CTX, "Pb when sending\n");
			return mgl_ko;
		}

		MGL_TRACE(MGL_CTX, MGL_TRACE_MGR_CX, "Msg sent\n");
		return mgl_ok;
	}
	return mgl_ko;
}


mgl_status mgl_blocmgr::blocmgrFdHandler(mgl_event_fd *ip_event_fd)
{
	char l_buf[8000];
	int l_size;
	mgl_event *lp_event;
	mgl_status l_ret;

	// Main input port
	if (ip_event_fd->blocid==-1) {
		_blocmgrInput.acceptNewConnection(1,0);
	}

	// remote hosts on input port
	// receive msg
	if (ip_event_fd->blocid==-3) {
		l_size=8000;
		if (_blocmgrInput.rcv_pkt_fd(ip_event_fd->fd, l_buf, &l_size, 1, 0)) {
			// build msg struct
			lp_event = mgl_event_msginfo_buf_to_struct(l_buf, l_size, NULL);
			if (!lp_event) {
				MGL_WARNING(MGL_CTX, "Pb, can't rebuild received msg. Msg discarded\n");
			}

			// Add it into queue : Seb : Check
			lp_event->event.msg.time_in = _pEventmgr->getCurrentTime();
			lp_event->event.msg.time_out = _pEventmgr->getCurrentTime();
			MGL_TRACE(MGL_CTX, MGL_TRACE_MGR_CX, "mgl_eventmgr: Remote msg added at %d\n", _pEventmgr->getCurrentTime());
			// Append it at the end of the queue
			_pEventmgr->_msg_list.append(lp_event);

		}
	}

	// output port
	// nothing to do :)
	if (ip_event_fd->blocid==-2) {
	}


	// Remote command port
	if (ip_event_fd->blocid==-4) {
		l_size=8000;
		if (_blocmgrInput.rcv_pkt_fd(ip_event_fd->fd, l_buf, &l_size, 1, 0)) {
			processRemoteCmd(l_buf);
		}
	}
	
	// channel msg
	if (ip_event_fd->blocid==-5) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "mgl_eventmgr: Event on channel fd\n");
		l_size=8000;
		if (mgl_multicast_channel::rcv_buf_fd(ip_event_fd->fd, l_buf, l_size, 1, 0)) {
			// build msg struct
			lp_event = mgl_event_msginfo_buf_to_struct(l_buf, l_size, NULL);
			if (!lp_event) {
				MGL_WARNING(MGL_CTX, "Pb, can't rebuild received msg. Msg discarded\n");
			}

			// Add it into queue : Seb : Check
			lp_event->event.msg.time_in = _pEventmgr->getCurrentTime();
			lp_event->event.msg.time_out = _pEventmgr->getCurrentTime();
			MGL_TRACE(MGL_CTX, MGL_TRACE_MGR_CX, "mgl_eventmgr: Remote msg added at %d\n", _pEventmgr->getCurrentTime());
			// Append it at the end of the queue
			l_ret = msgSendToChannel(-(lp_event->event.msg.dstBloc)-1, &(lp_event->event.msg), mgl_false);
		}
	}

	return mgl_ok;
}

mgl_status mgl_blocmgr::setTimer(mgl_id i_blocid, mgl_id &i_timerid, long i_mstimer, mgl_bool i_loop)
{ 
	mgl_status l_ret;

	if (!_pEventmgr) { return mgl_ko; } 
	l_ret=_pEventmgr->setTimer(i_blocid, i_timerid, i_mstimer, i_loop); 

	// MSC trace
	trace("Event Timer Set: timer(%d) at Bloc %d at %d for %d ms.", i_timerid, i_blocid, _pEventmgr->getCurrentTime(), i_mstimer);

	return l_ret; 
};

long mgl_blocmgr::getCurrentTime() 
{
	if (!_pEventmgr) { return 0; } 
	return _pEventmgr->getCurrentTime();
}


//	mgl_trace *_pTrace;
void mgl_blocmgr::setTrace(mgl_trace *ip_trace)
{
	_pTrace=ip_trace;
}


void mgl_blocmgr::traceEventsToFile(char *ip_filename)
{
	mgl_trace_file *lp_trace;

	MGL_TRACE_SET_FLAG(MGL_TRACE_CMD);
	lp_trace = new mgl_trace_file();
	lp_trace->open(ip_filename);
	setTrace(lp_trace);
}

void mgl_blocmgr::traceNamEventsToFile(char *ip_filename)
{
	_pTraceNam = new mgl_trace_file_nam();
	_pTraceNam->open(ip_filename);
}

void mgl_blocmgr::traceEventsToScreen()
{
	mgl_trace_screen *lp_trace;

	MGL_TRACE_SET_FLAG(MGL_TRACE_CMD);
	lp_trace = new mgl_trace_screen();
	setTrace(lp_trace);
}

void mgl_blocmgr::trace(const char *ip_format,...)
{
	static char l_buf[1024]="";
	va_list l_va;
	long l_len;


	if (MGL_NEED_TRACE(MGL_TRACE_CMD)) {
		// Format trace
		va_start( l_va, ip_format );     
		l_len = vsprintf( l_buf, ip_format, l_va );
		va_end( l_va );

		// Trace to a file
		if (_pTrace) { _pTrace->trace(l_buf); _pTrace->trace("\n"); }

		// Trace to remote debugger
		//MGL_TRACE(MGL_CTX, MGL_TRACE_CMD, "Send trace to remote cmd %s\n", l_buf);
		if (_ctrl._fd) _ctrl.snd_pkt(l_buf, l_len+1);

		// Printf
		//printf("%s\n", l_buf);
	}
}


void mgl_blocmgr::processRemoteCmd(char *ip_cmd)
{
	if (!ip_cmd) { return; }
	MGL_TRACE(MGL_CTX, MGL_TRACE_CMD, "Process remote cmd %s\n", ip_cmd);

	if (strncmp(ip_cmd, MGL_REMOTE_CMD_GO, strlen(MGL_REMOTE_CMD_GO))==0) {
		_cmd = cmd_go;
		process_step();
	}
	if (strncmp(ip_cmd, MGL_REMOTE_CMD_PAUSE, strlen(MGL_REMOTE_CMD_PAUSE))==0) {
		_cmd = cmd_pause;
	}
	if (strncmp(ip_cmd, MGL_REMOTE_CMD_STEP, strlen(MGL_REMOTE_CMD_STEP))==0) {
		_cmd = cmd_step;
		process_step();
	}
	if (strncmp(ip_cmd, MGL_REMOTE_CMD_TERMINATE, strlen(MGL_REMOTE_CMD_TERMINATE))==0) {
		_cmd = cmd_terminate;
		_state = state_terminating;
		//exit(1);
	}

}


void mgl_blocmgr::registerChannelSnd(mgl_id i_frombloc, mgl_id i_fromport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay, long i_bandwidth)
{
	long l_destid;

	l_destid = -1-i_channel;
	registerLink(i_frombloc, i_fromport, l_destid, 0, i_msgset, i_delay, i_bandwidth); 

}


void mgl_blocmgr::registerChannelRcv(mgl_id i_tobloc, mgl_id i_toport, mgl_id i_channel, const mgl_msgset &i_msgset, long i_delay, long i_bandwidth)
{
	mgl_channelrcv_info *lp_info;
	// Register destination bloc for this channel
	if (!_channel_conf) { return; }

	lp_info=  new mgl_channelrcv_info();
	lp_info->bloc = i_tobloc;
	lp_info->port = i_toport;
	lp_info->msgset = i_msgset;
	(_channel_conf[i_channel]).bloc_list.append(lp_info);
	
}

mgl_status mgl_blocmgr::msgSendToChannel(mgl_id i_channel, mgl_event_msg *ip_msg, mgl_bool i_send_network)
{
	mgl_channelrcv_info *lp_info;
	long l_nb;
	long l_cpt;
	mgl_msginfo *lp_msginfo;
	char l_buf[8000];
	long l_len;
	long l_ret;
	long l_distant_count;
	mgl_event_msg l_msg;

	MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "Send to channel %d\n", i_channel);
	// Check channelId

	// Copy the message for each destination bloc
	if (!_channel_conf) { return mgl_ko; }

	l_distant_count=0;
	l_nb = 	(_channel_conf[i_channel]).bloc_list.getCount();
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		lp_info = (mgl_channelrcv_info *)((_channel_conf[i_channel]).bloc_list.get(l_cpt));
		if (lp_info) {
			MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "Sending msg %d to bloc %d : ", ip_msg->ptr->type, lp_info->bloc);
			if (lp_info->msgset.msgIdIsIn(ip_msg->ptr->type)) {
				// Get real destination bloc
				l_msg.ptr = ip_msg->ptr;
				l_msg.srcBloc = lp_info->bloc;
				l_msg.srcPort = lp_info->port;
				l_msg.dstBloc = -1;
				l_msg.dstPort = -1;
				l_msg.time_out = 0;
				l_ret = msgFindDestination(&l_msg, true);
				if (l_ret == mgl_ok) {  // Ok, a sub bloc has been found as destination
					lp_info->bloc = l_msg.dstBloc;
					lp_info->port = l_msg.dstPort;
				}

				// Enqueue it in event mgr
				if (!_pEventmgr) { return mgl_ko; } 
				if (isLocallyManaged(lp_info->bloc)) {
					lp_msginfo = _pEventmgr->sendMsg(ip_msg->ptr, ip_msg->srcPort, ip_msg->srcPort); 
					lp_msginfo->dstBloc = lp_info->bloc;
					lp_msginfo->dstPort = lp_info->port;
					lp_msginfo->time_out += l_msg.time_out;

					MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "(local) Yes.\n");
				} else if (i_send_network==mgl_true) {
					if (_channel_conf[i_channel].socket._fd<=0) {
						// If the multicast channel is not opened,
						// send the messages using the bloc managers
						lp_msginfo = _pEventmgr->sendMsg(ip_msg->ptr, ip_msg->srcPort, ip_msg->srcPort); 
						lp_msginfo->dstBloc = lp_info->bloc;
						lp_msginfo->dstPort = lp_info->port;
						lp_msginfo->time_out += l_msg.time_out;

						MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "(distant via mgr) Yes.\n");
					} else {
						// If the multicast channel is opened,
						// count the number of interrested blocs
						// If this number is >0, the message is sent in multicast
						l_distant_count++;
					}
				} else {
					MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "(remote) Not allowed.\n");
				}
			} else {
				MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "No.\n");
			}
		}
	}
	// Order messages depending on output time
	//_pEventmgr->sortMsgList();

	// If some remote bloc are inetrrested by this packet and are
	// listening to the multicast channel, sent it
	if (l_distant_count>0) {
		// Get msg marshaller 

		// Serialize msg
		l_len = 8000;
		l_len = mgl_msginfo_struct_to_buf(l_buf, &l_len, ip_msg, NULL);
		if (l_len<=0) {
			MGL_WARNING(MGL_CTX, "Can't serialize msg. Msg lost\n");
			return mgl_ko;
		}

		// Send msg
		l_ret = _channel_conf[i_channel].socket.snd_buf(l_buf, l_len);
		if (!l_ret) {
			MGL_WARNING(MGL_CTX, "Pb when sending\n");
			return mgl_ko;
		}
		MGL_TRACE(MGL_CTX, MGL_TRACE_CHANNEL, "(distant via multicast) Yes.\n");
	}
	return mgl_ok;
}

/* Managers configuration ***/
mgl_status mgl_blocmgr::setChannelsConfiguration(mgl_channeldesc *ip_conf)
{
	_channel_conf=ip_conf;
	return mgl_ok;
}


mgl_status mgl_blocmgr::blocmgrOpenChannelConnection()
{
	long l_ret;
	long l_cpt;

	MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Initializing multicast channels\n");

	// There is a need to extablish connection between bloc mgrs
	// only when there are more than one :)
	if (!_channel_conf) { return mgl_ok; }
	
	// Open multicast sockets
	l_cpt=1;
	while (_channel_conf[l_cpt].port>0) {
		MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, "Channel %d (\"%s\":%d) : ", l_cpt, _channel_conf[l_cpt].ip.get(), _channel_conf[l_cpt].port);
		l_ret = _channel_conf[l_cpt].socket.openSocket(_channel_conf[l_cpt].ip.get(), _channel_conf[l_cpt].port, 1);
		if (l_ret) {
			MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, " Ok\n");
		} else {
			MGL_TRACE(MGL_CTX, MGL_TRACE_MAIN_STEP, " Failed\n");
		}
		// Ask event mgr to handle it
		_pEventmgr->addFd(l_ret, -5);
		l_cpt++;
	}
	return mgl_ok;
}


void mgl_blocmgr::usageCommandLineArguments(char *ip_name)
{
	printf("Usage: %s <options>\n", ip_name);
	printf("Options:\n");
	printf("\t-h                      : Help\n");
	printf("\t-mgl_remote_cmd IP PORT : Set the remote command configuration\n");
	printf("\tex: -mgl_remote_cmd localhost 4567\n");
	printf("\t-mgl_no_mgr_cx          : Disable connections with other managers\n");
}



void mgl_blocmgr::parseCommandLineArguments(int *iop_argc, char **iop_argv)
{
	long l_cpt;

	for (l_cpt=1; l_cpt<(*iop_argc); l_cpt++) {
		// Help
		if (strcmp(iop_argv[l_cpt], "-h")==0) {
			usageCommandLineArguments(iop_argv[0]);
		} else
		// Remote control
		if (strcmp(iop_argv[l_cpt], "-mgl_remote_cmd")==0) {
			setRemoteCtrl(iop_argv[l_cpt+1], atoi(iop_argv[l_cpt+2]));
			l_cpt+=2;
		} else
		// No mgr cx
		if (strcmp(iop_argv[l_cpt], "-mgl_no_mgr_cx")==0) {
			_blocmgrOutput_activated_flag=mgl_false;
		} else {
			usageCommandLineArguments(iop_argv[0]);
		} 
	}
}

mgl_status mgl_blocmgr::addFd(long i_fd, mgl_id i_blocid)
{
	if (!_pEventmgr) { 
		MGL_WARNING(MGL_CTX, "Try to set bloc's eventMgr when eventMgr is not instanciated\n");
		return mgl_ko; 
	}
	return _pEventmgr->addFd(i_fd, i_blocid);
}

mgl_status mgl_blocmgr::removeFd(long i_fd)
{
	if (!_pEventmgr) { 
		MGL_WARNING(MGL_CTX, "Try to set bloc's eventMgr when eventMgr is not instanciated\n");
		return mgl_ko; 
	}
	return _pEventmgr->removeFd(i_fd);
}


mgl_msg *mgl_blocmgr::allocateNewMessage()
{
	return _pEventmgr->allocateNewMessage();
}



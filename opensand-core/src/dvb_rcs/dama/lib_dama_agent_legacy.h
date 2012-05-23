/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file lib_dama_agent_legacy.h
 * @brief This is the Legacy algorithm.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef LIB_DAMA_AGENT_Legacy_H
#define LIB_DAMA_AGENT_Legacy_H

#include "lib_dama_agent.h"
#include "msg_dvb_rcs.h"
#include "lib_circular_buffer.h"
#include "NetBurst.h"
#include "DvbRcsFrame.h"


/**
 * @class DvbRcsDamaAgentLegacy
 * @brief This is the Legacy DAMA agent
 */
class DvbRcsDamaAgentLegacy: public DvbRcsDamaAgent
{

 private:

	// frame management
	long m_currentFrameNumber;  ///< current frame Number

	// UL carrier BW and allocation in kbits/s
	int m_ulCarrierBw;    ///< UL carrier BW - in kbits/s
	int m_craBw;          ///< fixed bandwith requested by the ST
	                      ///  at logon - in kbits/s

	// UL allocation in number of time-slots per frame
	int m_dynAlloc;       ///< dynamic bandwith allocated in nb of
	                      ///  time-slots/frame for current frame
	                      ///  (its also contains allocated CRA)
	int m_remainingAlloc; ///< total number of time-slots which still
	                      ///  can be used during current frame

	// parameters for RBDC capacity request computation
	bool m_rbdcStatus;    ///< true if RBDC capacity request is
	                      ///  activated for at least 1 MAC fifo
	int m_obrPeriod;      ///< OBR period in frame number
	int m_maxRbdc;        ///< Max RBDC in kbits/s
	int m_rbdcTimeout;    ///< RBDC timeout in frame number
	int m_rbdcTimer;      ///< RBDC timer in frame number: nb of frames
	                      ///  since last RBDC request sent
	CircularBuffer *m_RbdcReqBuf;  ///< circular buffer used to manage sum
	                               ///  of RBDC request -in kbits/s- during
	                               ///  last MSL
	bool m_firstFifoHasOnlyCra;    ///< indicates if the first FIFO is taken
	                               /// into account for RBDC CR
	int m_mslDuration;             ///< Minimum Scheduling Latency
	                               /// (in frames number)
	bool m_getIpOutputFifoSizeOnly;  ///< indicates if IP input fifo is
	                                 /// taken into account for CR

	// parameters for VBDC capacity request computation
	bool m_vbdcStatus;   ///< true if VBDC capacity request is activated
	                     ///  for at least 1 MAC fifo
	long m_vbdcCredit;   ///< in cells number
	int m_maxVbdc;       ///< in cells number

	int m_nbPvc;         ///< the number of PVCs


 public:

	DvbRcsDamaAgentLegacy(EncapPlugin::EncapPacketHandler *packet,
	                      double frame_duration);
	virtual ~DvbRcsDamaAgentLegacy();

	int initComplete(dvb_fifo *dvb_fifos,
	                 int dvb_fifos_number,
	                 double frameDuration,
	                 int craBw,
	                 int obrPeriod);

	int buildCR(dvb_fifo *dvb_fifo,
	            int dvb_fifos_number,
	            unsigned char *frame,
	            long length);

	// at each superframe
	int hereIsSOF(unsigned char *buf, long len);
	int hereIsTBTP(unsigned char *buf, long len);

	// at each frame
	int processOnFrameTick();
	int globalSchedule(dvb_fifo *dvb_fifos,
	                   int dvb_fifos_number,
	                   int &remainingAlloc,
	                   int encap_packet_type,
	                   std::list<DvbFrame *> *complete_dvb_frames);


 protected:

	// utility functions to get MAC / IP fifo buffers size/arrivals
	// (in number of equivalent encap packets)
	int getMacBufferLength(int crType,
	                       dvb_fifo *dvb_fifos,
	                       int dvb_fifos_number);
	                       int getMacBufferArrivals(int crType,
	                       dvb_fifo *dvb_fifos,
	                       int dvb_fifos_number);

	// compute CR requets functions
	int rbdcRequestCompute(dvb_fifo *dvb_fifos, int dvb_fifos_number);
	int vbdcRequestCompute(dvb_fifo *dvb_fifos, int dvb_fifos_number);

	// MAC scheduling
	int macSchedule(dvb_fifo *dvb_fifos,
	                int dvb_fifos_number,
	                int pvc,
	                int &extractedEncapPacketsNb,
	                int encap_packet_type,
	                std::list<DvbFrame *> *complete_dvb_frames);


	int createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame,
	                                int encap_packet_type);

};

#endif

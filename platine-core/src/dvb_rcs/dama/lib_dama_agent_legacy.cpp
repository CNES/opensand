/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under
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
 * @file lib_dama_agent_legacy.cpp
 * @brief This library defines Legacy DAMA agent
 * @author Viveris Technologies
 */

#include <string>
#include <math.h>
#include <stdlib.h>

#include "platine_margouilla/mgl_time.h"
#include "lib_dvb_rcs.h"
#include "lib_dama_agent_legacy.h"
#include "lib_dama_utils.h"
#include "DvbRcsFrame.h"
#include "MacFifoElement.h"

#include "platine_conf/conf.h"

// log file
#define DBG_PACKAGE PKG_DAMA_DA
#define DA_DBG_PREFIX        "[Legacy]"
#include "platine_conf/uti_debug.h"

// constants
const double C_MAX_RBDC_IN_SAC = 8160.0; // 8160 kbits/s, limitation due
                                         // to CR value size in to SAC field
const int C_MAX_VBDC_IN_SAC = 4080;      // 4080 kbits/s, limitation due to CR
                                         // value size in to SAC field

/**
 * Constructor
 */
DvbRcsDamaAgentLegacy::DvbRcsDamaAgentLegacy():DvbRcsDamaAgent()
{
	m_frameDuration = 0.0;
	m_currentFrameNumber = 1;
	m_ulCarrierBw = 0;
	m_craBw = 0;
	m_dynAlloc = 0;
	m_remainingAlloc = 0;
	m_rbdcStatus = false;
	m_obrPeriod = 0;;
	m_maxRbdc = 0;
	m_rbdcTimeout = 0;
	m_rbdcTimer = 0;
	m_RbdcReqBuf = NULL;
	m_firstFifoHasOnlyCra = false;
	m_mslDuration = 0;
	m_getIpOutputFifoSizeOnly = false;
	m_vbdcStatus = false;
	m_vbdcCredit = 0;
	m_maxVbdc = 0;
	this->m_nbPvc = 0;

	// NB: stats are already init by DvbRcsDamaAgent
}

/**
 * Destructor
 */
DvbRcsDamaAgentLegacy::~DvbRcsDamaAgentLegacy()
{
	if(m_RbdcReqBuf != NULL)
		delete m_RbdcReqBuf;
}


/**
 * Initializes legacy dama agent - with more parameters
 * !a new name is required because dynamical link allowed thanks to "virtual" keyword
 * is canceled if functions do not have the same arguments list
 *
 * Note: some parameters are directly retrieved from configuration file:
 *  \li UL carrier bandwith in kbits/s
 *  \li max Rbdc in kbits/s,
 *  \li rbdc Timeout in frame number,
 *  \li max Vbdc in cells number
 *  \li Minimum Scheduling Latency- MSL- duration in frame number
 *  \li pointer on burst under build is given by globalSchedule function
 *
 * @param dvb_fifos         the array of DVB FIFOs to schedule encapsulation packets from
 * @param dvb_fifos_number  the number of DVB FIFOs in the array
 * @param frameDuration     the frame duration
 * @param craBw             the CRA bandwidth (in kbits/s)
 * @param obrPeriod         the OBR period in frame number
 *
 * @return 0 on success -1 otherwise
 */
int DvbRcsDamaAgentLegacy::initComplete(dvb_fifo *dvb_fifos,
                                     int dvb_fifos_number,
                                     double frameDuration,
                                     int craBw,
                                     int obrPeriod)
{
	const char *FUNCNAME = DA_DBG_PREFIX "[initComplete]";
	int i;
	int val;
	string strConfig;

	/* set variables given by the MAC layer */
	m_craBw = craBw;
	m_frameDuration = frameDuration; // in seconds
	m_obrPeriod = obrPeriod; // in frame number

	/* retrieve other informations directly from configuration file */

	// UL carrier rate in kbits/s
	if(!globalConfig.getIntegerValue(DA_MAC_LAYER_SECTION,
	                                 DA_CARRIER_TRANS_RATE, val))
	{
		val = DFLT_CARRIER_TRANS_RATE;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n",
		          FUNCNAME, DA_CARRIER_TRANS_RATE, val);
	}
	m_ulCarrierBw = val;
	if(m_ulCarrierBw < m_craBw)
	{
		UTI_ERROR("%s fixed bandwidth (%d) > UL carrier BW (%d).\n",
		          FUNCNAME, m_craBw, m_ulCarrierBw);
		goto error;
	}

	// Max RBDC (in kbits/s) and RBDC timeout (in frame number)
	if(!globalConfig.getIntegerValue(DA_TAL_SECTION, DA_MAX_RBDC_DATA, val))
	{
		val = DA_DFLT_MAX_RBDC_DATA;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n",
		          FUNCNAME, DA_MAX_RBDC_DATA, val);
	}
	m_maxRbdc = val;
	if(!globalConfig.getIntegerValue(DA_TAL_SECTION,
	                                 DA_RBDC_TIMEOUT_DATA, val))
	{
		val = DFLT_RBDC_TIMEOUT;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n",
		          FUNCNAME, DA_RBDC_TIMEOUT_DATA, val);
	}
	m_rbdcTimeout = val;

	// Max VBDC -- in ATM cells/MPEG packets number
	if(!globalConfig.getIntegerValue(DA_TAL_SECTION, DA_MAX_VBDC_DATA, val))
	{
		val = DA_DFLT_MAX_VBDC_DATA;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n",
		          FUNCNAME, DA_MAX_VBDC_DATA, val);
	}
	m_maxVbdc = val;

	// MSL duration -- in frames number
	if(!globalConfig.getIntegerValue(DA_TAL_SECTION, DA_MSL_DURATION, val))
	{
		val = DA_DFLT_MSL_DURATION;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n",
		          FUNCNAME, DA_MSL_DURATION, val);
	}
	m_mslDuration = val;

	// CR computation rule
	if(!globalConfig.getStringValue(DA_TAL_SECTION, DA_CR_RULE, strConfig))
	{
		strConfig = DA_DFLT_CR_RULE;
		UTI_ERROR("%s Missing %s, taking default value (%s).\n",
		          FUNCNAME, DA_CR_RULE, strConfig.c_str());
	}
	if(strcmp(strConfig.c_str(), "yes") == 0)
	{
		// for Legacy algo only OUPUT IP fifo sizes are taken into acount
		m_getIpOutputFifoSizeOnly = true;
	}
	else
	{
		// for Legacy algo, both INPUT and OUPUT IP fifo sizes are taken into acount
		m_getIpOutputFifoSizeOnly = false;
	}

	UTI_INFO("%s ULCarrierBw %d kbits/s, maxRbdc %d kbits/s, rbdcTimeout %d "
	         "frame, maxVbdc %d kbits/s, mslDuration %d frames, "
	         "getIpOutputFifoSizeOnly %d\n", FUNCNAME, m_ulCarrierBw, m_maxRbdc,
	         m_rbdcTimeout, m_maxVbdc, m_mslDuration, m_getIpOutputFifoSizeOnly);

	// init other variables
	m_NRTMaxBandwidth = // in kbits/s (set for compatibiliy purpose
		(long) (m_ulCarrierBw - m_craBw);

	UTI_INFO("%s requested CRA %d kbits/s\n", FUNCNAME, m_craBw);

	m_dynAlloc = 0;       // nb of time-slots/frame
	m_next_allocated = 0; // nb of time-slots/frame
	m_remainingAlloc = 0; // total number of time-slots which still can
	                      // be used during current frame
	m_currentFrameNumber = 0;
	m_rbdcTimer = 0;
	m_vbdcCredit = 0;

	// check if RBDC or VBDC capacity request are activated
	m_firstFifoHasOnlyCra = false;
	m_rbdcStatus = false; // = true if RBDC capacity request is activated
	                      // for at least 1 MAC fifo
	m_vbdcStatus = false; // = true if VBDC capacity request is activated
	                      // for at least 1 MAC fifo

	for(i = 0; i < dvb_fifos_number; i++)
	{
		if(dvb_fifos[i].getCrType() == DVB_FIFO_CR_RBDC)
			m_rbdcStatus = true;
		else if(dvb_fifos[i].getCrType() == DVB_FIFO_CR_VBDC)
			m_vbdcStatus = true;
		else if(dvb_fifos[i].getCrType() == DVB_FIFO_CR_NONE)
		{
			// it is possible only for EF
			if(dvb_fifos[i].getKind() != DVB_FIFO_EF)
			{
				UTI_ERROR("NONE CR set for a non-EF FIFO\n");
				goto error;
			}
			else
			{
				m_firstFifoHasOnlyCra = true;
			}
		}
		else
		{
			UTI_ERROR("unknown Capacity Request (CR) type for FIFO #%d: %d\n",
			          i + 1, dvb_fifos[i].getCrType());
			goto error;
		}
	}

	// set the number of PVC = the maximum PVC is (first PVC isd is 1)
	this->m_nbPvc = 0;
	for(i = 0; i < dvb_fifos_number; i++)
	{
		this->m_nbPvc = MAX(dvb_fifos[i].getPvc(), this->m_nbPvc);
	}

	if(m_rbdcStatus)
	{
		// create circular buffer for saving last RBDC requests during the past
		// MSL duration with size = integer part of C_MSL_DURATION / m_obrPeriod
		// (in frame number)
		// NB: if size = 0, only last req is saved and sum is always 0
		m_RbdcReqBuf = new CircularBuffer((int) m_mslDuration / m_obrPeriod);
		if(this->m_RbdcReqBuf == NULL)
		{
			UTI_ERROR("%s cannot create circular buffer to save "
			          "the last RBDC requests\n", FUNCNAME);
			goto error;
		}
	}

	UTI_INFO("m_rbdcStatus %d, m_getIpOutputFifoSizeOnly %d, m_vbdcStatus %d\n",
	         m_rbdcStatus, m_getIpOutputFifoSizeOnly, m_vbdcStatus);

	return 0;

error:
	return -1;
}

/**
 * Used to pass to DAMA the TBTP received
 * @param buf points to the TBTP buffer
 * @param len length of the buffer
 * @return 0 on success, -1 otherwise
 */
int DvbRcsDamaAgentLegacy::hereIsTBTP(unsigned char *buf, long len)
{
	int ret = 0;

	// call mother class function
	ret = DvbRcsDamaAgent::hereIsTBTP(buf, len);

	// test that allocated capacity is smaller than UL carrier
	if(this->m_next_allocated >
	   (int) m_converter->ConvertFromKbitsToCellsPerFrame(m_ulCarrierBw))
	{
		UTI_ERROR("allocation received in TBTP (%ld cells/frame) is larger "
		          "than UL carrier bandwidth (%d cells/frame)\n",
		          this->m_next_allocated,
		          (int) m_converter->ConvertFromKbitsToCellsPerFrame(m_ulCarrierBw));
		ret = -1;
	}

	return ret;
}


/**
 * Compute and build RBDC and or VBDC capacity requests - Legacy algorithm
 *
 * @param dvb_fifos         the array of DVB FIFOs to schedule encapsulation packets from
 * @param dvb_fifos_number  the number of DVB FIFOs in the array
 * @param frame             the memory area available to build the Capacity
 *                          Request (CR) frame
 * @param length            the maximum length (in bytes) of the memory area
 *                          available for the CR frame
 * @return                  0 if at least 1 CR is computed,
 *                          -1 in case of error or if 0 CR built
 */
int DvbRcsDamaAgentLegacy::buildCR(dvb_fifo *dvb_fifos,
                                int dvb_fifos_number,
                                unsigned char *frame,
                                long length)
{
	const char *FUNCNAME = DA_DBG_PREFIX "[buildCR]";
	T_DVB_SAC_CR *init_cr; // Cast for buf
	int i, j;
	bool sendRbdcRequest = false;
	bool sendVbdcRequest = false;
	int rbdcRequest = -1; // RBDC request in kbits/s
	int vbdcRequest = -1; // VBDC request in cells number

	if(length < 0 || ((unsigned long) length) < sizeof(T_DVB_SAC_CR))
	{
		UTI_ERROR("buffer size too small to fit T_DVB_SAC_CR\n");
		goto error;
	}

	// compute RBDC request if required
	if(m_rbdcStatus)
	{
		UTI_DEBUG("%s compute RBDC request\n", FUNCNAME);
		rbdcRequest = rbdcRequestCompute(dvb_fifos, dvb_fifos_number);

		// send RBDC request only if RBDC_timer > RBDCTimeout /2,
		// or if CR is different than previous one
		if(rbdcRequest != 0)
		{
#ifdef OPTIMIZE
			if(rbdcRequest != (int) m_RbdcReqBuf->GetPreviousValue() ||
			   m_rbdcTimer > (m_rbdcTimeout / 2))
			{
#endif
				sendRbdcRequest = true;
#ifdef OPTIMIZE
			}
#endif
		}
		else
		{
			if(rbdcRequest != (int) m_RbdcReqBuf->GetPreviousValue())
				sendRbdcRequest = true;
		}
	}

	// compute VBDC request if required
	if(m_vbdcStatus)
	{
		UTI_DEBUG("%s compute VBDC request\n", FUNCNAME);
		vbdcRequest = vbdcRequestCompute(dvb_fifos, dvb_fifos_number);

		// send VBDC request only if it is not null
		if(vbdcRequest != 0)
			sendVbdcRequest = true;
	}

	// if no valid CR can be built: skipping it
	if(!sendRbdcRequest && !sendVbdcRequest)
	{
		UTI_DEBUG_L3("%s RBDC cr = %d, VBDC cr = %d, no CR built.\n",
		             FUNCNAME, rbdcRequest, vbdcRequest);
		goto error;
	}

	init_cr = (T_DVB_SAC_CR *) frame;
	init_cr->hdr.msg_length = sizeof(T_DVB_SAC_CR);
	init_cr->hdr.msg_type = MSG_TYPE_CR;

	if(!sendRbdcRequest || !sendVbdcRequest)
		init_cr->cr_number = 1;
	else
		init_cr->cr_number = 2;

	UTI_DEBUG("%s SF#%ld: RBDC cr = %d kbits/s, VBDC cr = %d cells, "
	          "CR built with %d requests\n", FUNCNAME, m_currentSuperFrame,
	          rbdcRequest, vbdcRequest, init_cr->cr_number);

	// set RBDC request (if any) in SAC
	i = 0;
	if(sendRbdcRequest)
	{
		init_cr->cr[i].route_id = 0;
		init_cr->cr[i].type = DVB_CR_TYPE_RBDC;
		init_cr->cr[i].channel_id = 0;
		encode_request_value(&(init_cr->cr[i]), rbdcRequest);
		init_cr->cr[i].group_id = m_groupId;
		init_cr->cr[i].logon_id = m_talId;
		init_cr->cr[i].M_and_C = 0;

		// update variables used for next RBDC CR computation
		m_rbdcTimer = 0;
		m_RbdcReqBuf->Update((double) rbdcRequest);

		// reset counter of arrival cells in MAC FIFOs related to RBDC
		for(j = 0; j < dvb_fifos_number; j++)
		{
			dvb_fifos[j].resetFilled(DVB_FIFO_CR_RBDC);
		}

		i++;
	}

	// set VBDC request (if any) in SAC
	if(sendVbdcRequest)
	{
		init_cr->cr[i].route_id = 0;
		init_cr->cr[i].type = DVB_CR_TYPE_VBDC;
		init_cr->cr[i].channel_id = 0;
		encode_request_value(&(init_cr->cr[i]), vbdcRequest);
		init_cr->cr[i].group_id = m_groupId;
		init_cr->cr[i].logon_id = m_talId;
		init_cr->cr[i].M_and_C = 0;
	}

	// save allocations stat
	m_statContext.rbdcRequest = rbdcRequest;
	m_statContext.vbdcRequest = vbdcRequest;

	return 0;

error:
	return -1;
}


/**
 * Called when the DVB RCS layer receive a start of Frame
 * Process the frame, set the SuperFrame number and validate
 * previous received authorizations
 *
 * @param buf  a pointer to a DVB frame structure
 * @param len  the length of the structure
 * @return     0 on success , -1 otherwise
 */
int DvbRcsDamaAgentLegacy::hereIsSOF(unsigned char *buf, long len)
{
	T_DVB_SOF *sof;

	sof = (T_DVB_SOF *) buf;
	if(sof->hdr.msg_type != MSG_TYPE_SOF)
	{
		UTI_ERROR("non SOF msg type (%ld)\n", sof->hdr.msg_type);
		return -1;
	}

	m_currentSuperFrame = sof->frame_nr;
	m_rbdcTimer++;
	// update dynamic allocation for next SF with allocation received
	// through TBTP during last sf (see hereIsTBTP function in
	// DvbRcsDamaAgent class)
	m_dynAlloc = m_next_allocated;
	m_next_allocated = 0;

	return 0;
}


/**
 * Called by the DVB RCS layer each frame
 *
 * @return  the global allocation for the current frame
 */
int DvbRcsDamaAgentLegacy::processOnFrameTick()
{
	// update counters
	m_currentFrameNumber++;

	// NB : RBDC timer is updated by the rbdcRequestCompute() function itself

	// update total allocation available for this frame: CRA allocation
	// is also sent in ONE_TIME_ASSGINMENT type via TBTP
	m_remainingAlloc = m_dynAlloc;

	// save allocations stat
	m_statContext.craAlloc = m_craBw;       // this is CRA requested by the ST in the logon

	// this is global ST alloc (CRA + RBDC + VBDC + FCA), received via TBTP
	m_statContext.globalAlloc =
	  (int) m_converter->ConvertFromCellsPerFrameToKbits(m_remainingAlloc);

	// and inform MAC layer of this allocation
	return (m_remainingAlloc);
}


/**
 * Called by the DVB RCS layer each frame (called 1 time)
 * to perform both MAC scheduling
 *
 * @param dvb_fifos                 the array of DVB FIFOs to schedule
 *                                  encapsulation packets from
 * @param dvb_fifos_number          the number of DVB FIFOs in the array
 * @param outRemainingAlloc         IN/OUT: remaining allocation after extraction
 *                                  of encapsulation packets number from MAC fifos
 * @param encap_packet_type         The type of packet encapsulation
 * @param complete_dvb_frames       IN/OUT: the list of complete DVB frames
 *                                  created with encapsulation packets extracted
 *                                  from the DVB FIFOs
 * @return                          0 if KO, -1 if failure
 */
int DvbRcsDamaAgentLegacy::globalSchedule(dvb_fifo *dvb_fifos,
                                       int dvb_fifos_number,
                                       int &outRemainingAlloc,
                                       int encap_packet_type,
                                       std::list<DvbFrame *> *complete_dvb_frames)
{
	int ret = 0;
	int pvcId;

	outRemainingAlloc = this->m_remainingAlloc;

	// for each PVC, schedule MAC Fifos
	for(pvcId = 1; pvcId < this->m_nbPvc + 1; pvcId++)
	{
		int extractedEncapPacketsNb = 0;

		// extract and send encap packets from MAC FIFOs, in function of
		// UL allocation
		if(this->macSchedule(dvb_fifos,
		                     dvb_fifos_number,
		                     pvcId,
		                     extractedEncapPacketsNb,
		                     encap_packet_type,
		                     complete_dvb_frames) < 0)
		{
			UTI_ERROR("MAC scheduling failed\n");
			ret = -1;
		}

		// returned outRemainingAlloc is updated only with encap packets
		// scheduled from MAC fifos because packets issued from IP scheduling
		// still need to be received by MAC layer
		outRemainingAlloc -= extractedEncapPacketsNb;
	}

	// update MAC statitics
	m_statContext.unusedAlloc =  // unused bandwith in kbits/s
		(int) m_converter->ConvertFromCellsPerFrameToKbits(m_remainingAlloc);

	return (ret);
}


/**
 * extract encapsulation packets from MAC FIFOs (only for requested PVC)
 *
 * @param dvb_fifos                 the array of DVB FIFOs to schedule
 *                                  encapsulation packets from
 * @param dvb_fifos_number          the number of DVB FIFOs in the array
 * @param pvc                       id of the PVC to schedule
 * @param extractedEncapPacketsNb   IN/OUT: number of extracted encapsulation
 *                                  packets
 * @param encap_packet_type         The type of packet encapsulation
 * @param complete_dvb_frames       IN/OUT: the list of complete DVB frames
 *                                  created with encapsulation packets extracted
 *                                  from the DVB FIFOs
 * @return                          0 if KO, -1 if failure
 */
int DvbRcsDamaAgentLegacy::macSchedule(dvb_fifo *dvb_fifos,
                                    int dvb_fifos_number,
                                    int pvc,
                                    int &extractedEncapPacketsNb,
                                    int encap_packet_type,
                                    std::list<DvbFrame *> *complete_dvb_frames)
{
	const unsigned int frame_index = 0;
	unsigned int complete_frames_count;
	DvbRcsFrame *incomplete_dvb_frame = NULL;
	int fifoIndex;
	int initialAlloc;
	int ret = 0;

	initialAlloc = m_remainingAlloc;

	UTI_DEBUG("SF#%ld: frame %ld: attempt to extract encap packets from "
	          "MAC FIFOs for PVC %d (remaining allocation = %d packets)\n",
	          this->m_currentSuperFrame, this->m_currentFrameNumber,
	          pvc, this->m_remainingAlloc);

	// create an incomplete DVB-RCS frame
	if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame, encap_packet_type))
	{
		goto error;
	}

	// extract encap packets from MAC FIFOs while some UL capacity is available
	// (MAC fifos priorities are in MAC IDs order)
	fifoIndex = 0;
	complete_frames_count = 0;
	while(fifoIndex < dvb_fifos_number &&
	      this->m_remainingAlloc > 0)
	{
		NetPacket *encap_packet;
		MacFifoElement *elem;

		if(dvb_fifos[fifoIndex].getPvc() != pvc)
		{
			// ignore FIFO with a different PVC
			UTI_DEBUG_L3("SF#%ld: frame %ld: ignore MAC FIFO "
			             "with ID %d: PVC is %d not %d\n",
			             this->m_currentSuperFrame,
			             this->m_currentFrameNumber,
			             dvb_fifos[fifoIndex].getId(),
			             dvb_fifos[fifoIndex].getPvc(),
			             pvc);
			// pass to next fifo
			fifoIndex++;
		}
		else if(dvb_fifos[fifoIndex].getCount() <= 0)
		{
			// FIFO is on correct PVC but got no data
			UTI_DEBUG_L3("SF#%ld: frame %ld: ignore MAC FIFO "
			             "with ID %d: correct PVC %d but no data "
			             "(left) to schedule\n",
			             this->m_currentSuperFrame,
			             this->m_currentFrameNumber,
			             dvb_fifos[fifoIndex].getId(),
			             dvb_fifos[fifoIndex].getPvc());
			// pass to next fifo
			fifoIndex++;
		}
		else
		{
			// FIFO with correct PVC and awaiting data
			UTI_DEBUG_L3("SF#%ld: frame %ld: extract packet from "
			             "MAC FIFO with ID %d: correct PVC %d and "
			             "%ld awaiting packets (remaining "
			             "allocation = %d)\n",
			             this->m_currentSuperFrame,
			             this->m_currentFrameNumber,
			             dvb_fifos[fifoIndex].getId(),
			             dvb_fifos[fifoIndex].getPvc(),
			             dvb_fifos[fifoIndex].getCount(),
			             this->m_remainingAlloc);

			// extract next encap packet context from MAC fifo
			elem = (MacFifoElement *) dvb_fifos[fifoIndex].remove();

			// delete elem context (keep only the packet)
			encap_packet = elem->getPacket();
			delete elem;
			encap_packet->addTrace(HERE());

			// is there enough free space in the DVB frame
			// for the encapsulation packet ?
			if(encap_packet->totalLength() >
			   incomplete_dvb_frame->getFreeSpace())
			{
				UTI_DEBUG_L3("SF#%ld: frame %ld: DVB frame #%u "
				             "is full, change for next one\n",
				             this->m_currentSuperFrame,
				             this->m_currentFrameNumber,
				             complete_frames_count + 1);

				complete_dvb_frames->push_back(incomplete_dvb_frame);

				// create another incomplete DVB-RCS frame
				if(!this->createIncompleteDvbRcsFrame(&incomplete_dvb_frame, encap_packet_type))
				{
					goto error;
				}

				complete_frames_count++;

				// is there enough free space in the next DVB-RCS frame ?
				if(encap_packet->totalLength() >
				   incomplete_dvb_frame->getFreeSpace())
				{
					UTI_ERROR("DVB-RCS frame #%u got no enough "
					          "free space, this should never "
					          "append\n", complete_frames_count + 1);
					delete encap_packet;
					continue;
				}
			}

			// add the encapsulation packet to the current DVB-RCS frame
			if(incomplete_dvb_frame->addPacket(encap_packet) != true)
			{
				UTI_ERROR("SF#%ld: frame %ld: cannot add "
				          "extracted MAC cell in "
				          "DVB frame #%u\n",
				          this->m_currentSuperFrame,
				          this->m_currentFrameNumber,
				          complete_frames_count + 1);
				encap_packet->addTrace(HERE());
				delete encap_packet;
				ret = -1;
				continue;
			}

			UTI_DEBUG_L3("SF#%ld: frame %ld: extracted packet added "
			             "to DVB frame #%u\n",
			             this->m_currentSuperFrame,
			             this->m_currentFrameNumber,
			             frame_index);
			delete encap_packet;

			// update allocation
			this->m_remainingAlloc--;
		}
	}

	// add the incomplete DVB-RCS frame to the list of complete DVB-RCS frame
	// if it is not empty
	if(incomplete_dvb_frame != NULL)
	{
		if(incomplete_dvb_frame->getNumPackets() > 0)
		{
			complete_dvb_frames->push_back(incomplete_dvb_frame);

			// increment the counter of complete frames
			complete_frames_count++;
		}
		else
		{
			delete incomplete_dvb_frame;
		}
	}
	extractedEncapPacketsNb = initialAlloc - this->m_remainingAlloc;

	// print status
	UTI_DEBUG("SF#%ld: frame %ld: %d packets extracted from MAC FIFOs "
	          "for PVC %d, %u DVB frame(s) were built (remaining allocation "
	          "= %d packets, ret = %d)\n",
	          this->m_currentSuperFrame, this->m_currentFrameNumber,
	          extractedEncapPacketsNb, pvc, complete_frames_count,
	          this->m_remainingAlloc, ret);

	return ret;
error:
	return -1;
}

/**
 * @brief Create an incomplete DVB-RCS frame
 *
 * @param incomplete_dvb_frame OUT: the DVB-RCS frame that will be created
 * return                      1 on success, 0 on error
*/
int DvbRcsDamaAgentLegacy::createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame,
                                                    int encap_packet_type)
{
	if(encap_packet_type != PKT_TYPE_ATM &&
	   encap_packet_type != PKT_TYPE_MPEG)
	{
		UTI_ERROR("invalid packet type (%d) in DvbRcsDamaAgent\n",
		          encap_packet_type);
		goto error;
	}

	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{
		UTI_ERROR("failed to create DVB-RCS frame\n");
		goto error;
	}

	// set the max size of the DVB-RCS frame, also set the type
	// of encapsulation packets the DVB-RCS frame will contain
	(*incomplete_dvb_frame)->setMaxSize(MSG_DVB_RCS_SIZE_MAX);
	(*incomplete_dvb_frame)->setEncapPacketType(encap_packet_type);

	return 1;

error:
	return 0;
}



/**
 * @brief Compute RBDC request in static or dynamic mode
 *
 * @param dvb_fifos         the array of DVB FIFOs to schedule encapsulation
 *                          packets from
 * @param dvb_fifos_number  the number of DVB FIFOs in the array
 * @return                  RBDC Request in kbits/s (ready to be set in SAC field)
 */
int DvbRcsDamaAgentLegacy::rbdcRequestCompute(dvb_fifo *dvb_fifos,
                                           int dvb_fifos_number)
{
	const char *FUNCNAME = DA_DBG_PREFIX "[rbdcCompute]";
	double RateNeed;      // cells /sec
	double RbdcRequest;   //  kbits/s
	double RbdcLimit;
	int RbdcRLength;      // in cells number
	int RbdcRCellArrival; // in cells number
	double sumRbdcPreceedingMSL_inCellsPs;

	/* get number of outstanding cells in RBDC related MAC FIFOs (in cells number) */
	RbdcRLength = getMacBufferLength(DVB_FIFO_CR_RBDC, dvb_fifos,
	                                 dvb_fifos_number);

	// get number of cells arrived in RBDC related IP FIFOs since last RBDC request sent
	//(in cells number)
	// NB: arrivals in MAC FIfos must NOT be taken into account because these cells
	// represent only cells buffered because there is no more available allocation, but its arrival
	// has been taken into account in IP fifos
	RbdcRCellArrival = getMacBufferArrivals(DVB_FIFO_CR_RBDC, dvb_fifos,
	                                        dvb_fifos_number);

	/* rbdc timer = (current frame - last rbdcrequest frame) * frame duration in sec */
	// m_rbdcTimer = m_currentFrameNumber - m_lastRbdcRequestFrame;

	// convert sum of RBDC request during last MSL from kbits/s to cells/s
	sumRbdcPreceedingMSL_inCellsPs =
		m_converter->ConvertFromKbitsToCellsPerSec((int) m_RbdcReqBuf->GetSum());

	/* compute RateNeed : estimation of the need of bandwith for traffic - in cell/sec */
	if(m_rbdcTimer != 0.0)
	{
		RateNeed =
			(double) (RbdcRCellArrival) / (double) (m_rbdcTimer * m_frameDuration) +
			MAX(0, ((double) (RbdcRLength) - (double) (RbdcRCellArrival) -
			       (m_rbdcTimer * m_frameDuration * sumRbdcPreceedingMSL_inCellsPs)) /
			       (double) (m_frameDuration * m_mslDuration));
	}
	else
	{
		RateNeed =
			MAX(0, ((double) (RbdcRLength) - (double) (RbdcRCellArrival) -
			       (m_rbdcTimer * m_frameDuration * sumRbdcPreceedingMSL_inCellsPs)) /
			       (double) (m_frameDuration * m_mslDuration));
	}

	UTI_DEBUG_L3("%s frame = %ld, rbdcTimer = %d, RbdcRLength = %d , "
	             "RbdcRCellArrival = %d, sumRbdcPreceedingMSL = %3.f cell/s, "
	             "RateNeed = %3.f cell/s\n", FUNCNAME, m_currentFrameNumber,
	             m_rbdcTimer, RbdcRLength, RbdcRCellArrival,
	             sumRbdcPreceedingMSL_inCellsPs, RateNeed);

	/* Compute actual RBDC request to be sent in Kbit/sec */
	RbdcRequest = m_converter->ConvertFromCellsPerSecToKbits(RateNeed);
	UTI_DEBUG("%s frame=%ld,  theoretical RbdcRequest =%3.f kbits/s",
	           FUNCNAME, m_currentFrameNumber, RbdcRequest);

	/* adjust request in function of max RBDC and fixed allocation */
	if(!m_firstFifoHasOnlyCra)
	{
		RbdcLimit = (double) (m_maxRbdc + m_craBw);
	}
	else
	{
		RbdcLimit = (double) m_maxRbdc;
	}
	RbdcRequest = ceil(MIN(RbdcRequest, RbdcLimit));
	UTI_DEBUG_L3("\t updated RbdcRequest=%3.f kbits/s (in fonction of maxRBDc and CRA)\n",
	             RbdcRequest);

	/* reduce the request value to the maximum theorical value if required */
	if(RbdcRequest > C_MAX_RBDC_IN_SAC)
		RbdcRequest = C_MAX_RBDC_IN_SAC;

	UTI_DEBUG_L3("%s frame=%ld,  updated RbdcRequest =%3.f kbits/s in SAC\n",
	             FUNCNAME, m_currentFrameNumber, RbdcRequest);

	return (int (RbdcRequest));
}

/*
 * Purpose : Compute VBDC request
 *
 * @param dvb_fifos         the array of DVB FIFOs to schedule encapsulation
 *                          packets from
 * @param dvb_fifos_number  the number of DVB FIFOs in the array
 * @return                  the VBDC Request in number of ATM cells
 *                          ready to be set in SAC field
 *                          (TODO: is it really in ATM cells ?)
 */
int DvbRcsDamaAgentLegacy::vbdcRequestCompute(dvb_fifo *dvb_fifos,
                                           int dvb_fifos_number)
{
	int VbdcNeed;
	int VbdcRequest;

	/* Compute VBDC_need : */
	/* get number of outstanding cells in VBDC related MAC and IP FIFOs (in cells number) */
	VbdcNeed = getMacBufferLength(DVB_FIFO_CR_VBDC, dvb_fifos, dvb_fifos_number);
	UTI_DEBUG_L3("frame %ld: getMacBufferLength = %d, m_vbdcCredit = %ld\n",
	             m_currentFrameNumber, VbdcNeed, m_vbdcCredit);

	/* compute VBDC_Request: actual Vbdc request to be sent - in Slot */
	VbdcRequest = MAX(0, (VbdcNeed - m_vbdcCredit));
	UTI_DEBUG_L3("frame %ld: theoretical VbdcRequest =%d cells",
	             m_currentFrameNumber, VbdcRequest);

	/* adjust request in function of MaxVbdc value */
	VbdcRequest = MIN(VbdcRequest, m_maxVbdc);
	/* tune VBDC_Request so that is not greater than SAC field size limitation */
	VbdcRequest = MIN(VbdcRequest, C_MAX_VBDC_IN_SAC);
	UTI_DEBUG_L3("\t--> updated VbdcRequest=%d cells in fonction of "
	             "maxVbdc and max VBDC in SAC\n", VbdcRequest);

	/* update VBDC_Credit here */
	/* NB: the computed VBDC is always really sent if not null */
	m_vbdcCredit += VbdcRequest;
	UTI_DEBUG_L3("\tupdated VbdcRequest=%d cells in SAC, vbdc credit = %ld\n",
	             VbdcRequest, m_vbdcCredit);

	/* return */
	return (VbdcRequest);
}


/**
 * Utility function to get total buffer size of all MAC fifos
 * associated to the concerned CR type
 *
 * @param crType            the type of capacity request
 * @param dvb_fifos         the array of DVB FIFOs to schedule encapsulation
 *                          packets from
 * @param dvb_fifos_number  the number of DVB FIFOs in the array
 * @return                  total buffers size in ATM cells number
 *                          (TODO: is it really in ATM ?)
 */
int DvbRcsDamaAgentLegacy::getMacBufferLength(int crType,
                                           dvb_fifo *dvb_fifos,
                                           int dvb_fifos_number)
{
	int nb_cells_in_fifo; // absolute number of cells in fifo
	int i;

	nb_cells_in_fifo = 0;
	for(i = 0; i < dvb_fifos_number; i++)
	{
		if(dvb_fifos[i].getCrType() == crType)
		{
			nb_cells_in_fifo += (int) dvb_fifos[i].getCount();
		}
	}

	return nb_cells_in_fifo;
}


/**
 * Utility function to get total number of "last arrived" cells (since last CR) of all MAC fifos
 * associated to the concerned CR type
 *
 * @param crType            the type of capacity request
 * @param dvb_fifos         the array of DVB FIFOs to schedule encapsulation
 *                          packets from
 * @param dvb_fifos_number  the number of DVB FIFOs in the array
 * @return                  total number of "last arrived" cells
 *                          in ATM cells number
 *                          (TODO: is it really in ATM ?)
 */
int DvbRcsDamaAgentLegacy::getMacBufferArrivals(int crType,
                                             dvb_fifo *dvb_fifos,
                                             int dvb_fifos_number)
{
	int nb_cells_input; // # cells that filled the queue since last RBDC request
	int i;

	nb_cells_input = 0;
	for(i = 0; i < dvb_fifos_number; i++)
	{
		if(dvb_fifos[i].getCrType() == crType)
		{
			nb_cells_input += (int) dvb_fifos[i].getFilledWithNoReset();
		}
	}

	return nb_cells_input;
}

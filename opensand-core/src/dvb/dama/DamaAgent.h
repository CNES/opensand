/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file    DamaAgent.h
 * @brief   This class defines the DAMA Agent interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#ifndef _DAMA_AGENT_H_
#define _DAMA_AGENT_H_

#include "Ttp.h"
#include "Sac.h"
#include "DvbFifo.h"
#include "DvbFrame.h"
#include "OpenSandCore.h"
#include "EncapPlugin.h"
#include "Logon.h"

#include <opensand_output/Output.h>

#include <map>
#include <list>

using std::map;
using std::list;

/**
 * @class DamaAgent
 * @brief Define methods to Manage DAMA requests and uplink scheduling in the ST,
 *        should be inherited for actual implementation.
 *
 * This class is used as a common central point for implementing a set of DAMA
 */
class DamaAgent
{
 public:

	/**
	 * Build a Dama agent.
	 *
	 */
	DamaAgent();

	/**
	 * Destroy the Dama Agent.
	 */
	virtual ~DamaAgent();

	/**
	 * @brief  Initalize the DAMA Agent common parameters
	 *
	 * @param frame_duration_ms      The frame duration (in ms)
	 * @param cra_kbps               The CRA value (in kbits/s)
	 * @param max_rbdc_kbps          The maximum RBDC value (in kbits/s)
	 * @param rbdc_timeout_sf        The RBDC timeout (in superframe number)
	 * @param max_vbdc_kb            The maximum VBDC value (in kbits)
	 * @param msl_sf                 The MSL (Minimum Scheduling Latency) value
	 *                               (time between CR emission and TTP reception
	 *                                in superframe number)
	 * @param sync_period_sf         The SYNC period ( former OBR (OutBand request) period)
	 *                               (used to determine when a request should
	 *                                be sent in superframe number)
	 * @param packet_handler         The packet handler
	 * @param dvb_fifos              The MAC FIFOs
	 * @return true on success, false otherwise
	 */
	bool initParent(time_ms_t frame_duration_ms,
	                rate_kbps_t cra_kbps,
	                rate_kbps_t max_rbdc_kbps,
	                time_sf_t rbdc_timeout_sf,
	                vol_kb_t max_vbdc_kb,
	                time_sf_t msl_sf,
	                time_sf_t sync_period_sf,
	                EncapPlugin::EncapPacketHandler *pkt_hdl,
	                const fifos_t &dvb_fifos);

	/**
	 * @brief Initialize the instatiated Dama Agent
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool init() = 0;

	// Protocol frames processing

	/**
	 * Extract a valid tal id and logon Id from the logonResp buffer.
	 *
	 * @brief Process a Logon Response frame.
	 *
	 * @param response logon response.
	 * @return true on success, false otherwise.
	 */
	virtual bool hereIsLogonResp(const LogonResponse *response);

	/**
	 * @brief Called when the DVB RCS layer receive a SOF.
	 *
	 * Called when the DVB layer receive a SOF.
	 * Set the SuperFrame number and validate
	 * previous received authorizations.
	 *
	 * @param superframe_number_sf superframe number.
	 * @return true on success, false otherwise.
	 */
	virtual bool hereIsSOF(time_sf_t superframe_number_sf);

	/**
	 * @brief Process a TTP frame.
	 *
	 * @param ttp received TTP.
	 * @return true on success, false otherwise.
	 */
	virtual bool hereIsTTP(Ttp *ttp) = 0;

	/**
	 * @brief Build SAC.
	 *
	 * @param cr_type   CR type to compute CR on.
	 * @param sac       SAC built.
	 * @param empty     flag if CR is 0.
	 * @return true on success, false otherwise.
	 */
	virtual bool buildSAC(ret_access_type_t cr_type,
	                      Sac *sac,
	                      bool &empty) = 0;

	/**
	 * @brief Schedule uplink packets emission.
	 *
	 * @param complete_dvb_frames  created DVB frames.
	 * @return true on success, false otherwise.
	 */
	virtual bool returnSchedule(list<DvbFrame *> *complete_dvb_frames) = 0;

	/**
	 * @brief   Called at each SoF.
	 *
	 * @return  true on success, false otherwise.
	 */
	virtual bool processOnFrameTick() = 0;

	/**
	 * @brief  Update the DAMA statistics
	 *         Called each frame
	 *
	 * @param period_ms  The period of statistics refreshing
	 */
	virtual void updateStatistics(time_ms_t UNUSED(period_ms)) {};


protected:

	/**
	 * @brief	Init the output probes and stats
	 *
	 * @return true on success, false otherwise.
	 */
	bool initOutput();

	/** Flag if initialisation of base class has been done */
	bool is_parent_init;

	/** The packet representation */
	EncapPlugin::EncapPacketHandler *packet_handler;

	/** The MAC FIFOs */
	fifos_t dvb_fifos;

	/** Terminal ID of the ST */
	tal_id_t tal_id;
	/** Group ID of the ST */
	group_id_t group_id;

	/** Current superframe number */
	time_sf_t current_superframe_sf;

	/** Flags if RBDC requests are enabled */
	bool rbdc_enabled;
	/** Flags if VBDC requests are enabled */
	bool vbdc_enabled;

	/** Frame duration (in ms) */
	time_ms_t frame_duration_ms;
	/** CRA value for ST (in kb/s) */
	rate_kbps_t cra_kbps;
	/** RBDC max value (in kb/s) */
	rate_kbps_t max_rbdc_kbps;
	/** RBDC timeout (in frame number) */
	time_sf_t rbdc_timeout_sf;
	/** VBDC maximal value (in kb) */
	vol_kb_t max_vbdc_kb;
	/** Minimum Scheduling Latency (in frame number) */
	time_sf_t msl_sf;
	/** SYNC period: period between two CR (in frame number) */
	time_sf_t sync_period_sf;
	/** If true, compute only output FIFO size for CR generation */
	bool cr_output_only;

	// Output Log
	OutputLog *log_init;
	OutputLog *log_frame_tick;
	OutputLog *log_schedule;
	OutputLog *log_ttp;
	OutputLog *log_sac;
	OutputLog *log_request;

	/** Output probes and stats */
		// Requests sizes
			// RBDC
	Probe<int> *probe_st_rbdc_req_size;
			// VBDC
	Probe<int> *probe_st_vbdc_req_size;
		// Allocation
			// Total
	Probe<int> *probe_st_total_allocation;
			// Remaining
	Probe<int> *probe_st_remaining_allocation;

};

#endif


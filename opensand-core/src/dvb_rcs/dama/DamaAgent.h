/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
#include "CapacityRequest.h"
#include "DvbFifo.h"
#include "DvbFrame.h"
#include "OpenSandCore.h"

#include "EncapPlugin.h"

#include <vector>
#include <list>


/// DAMA agent statistics context
typedef struct
{
	rate_kbps_t rbdc_request_kbps; ///< RBDC request sent at this frame (in kbits/s)
	vol_pkt_t vbdc_request_pkt;        ///< VBDC request sent at this frame (in cell nb)
	rate_kbps_t cra_alloc_kbps;    ///< fixed bandwith allocated in kbits/s
	rate_kbps_t global_alloc_kbps; ///< global bandwith allocated in kbits/s
	rate_kbps_t unused_alloc_kbps;  ///< unused bandwith in kbits/s
} da_stat_context_t;


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
	 * @param packet_handler the packet handler
	 * @param dvb_fifos                The MAC FIFOs
	 */
	DamaAgent(const EncapPlugin::EncapPacketHandler *pkt_hdl,
	          const std::map<unsigned int, DvbFifo *> &dvb_fifos);

	/**
	 * Destroy the Dama Agent.
	 */
	virtual ~DamaAgent();

	/**
	 * @brief  Initalize the DAMA Agent common parameters
	 *
	 * @param superframe_duration_ms   The superframe duration (in ms)
	 * @param cra_kbps                 The CRA value (in kbits/s)
	 * @param max_rbdc_kbps            The maximum RBDC value (in kbits/s)
	 * @param rbdc_timeout_sf          The RBDC timeout (in superframe number)
	 * @param max_vbdc_pkt             The maximum VBDC value (in number of packets/cells)
	 * @param msl_sf                   The MSL (Minimum Scheduling Latency) value
	 *                                 (time between CR emission and TTP reception
	 *                                  in superframe number)
	 * @param obr_period_sf            The OBR (OutBand request) period
	 *                                 (used to determine when a request should
	 *                                  be sent in superframe number)
	 * @param cr_output_only           Whether only output FIFO size is computed
	 *                                 for CR generation
	 * @return true on success, false otherwise
	 */
    bool initParent(time_ms_t superframe_duration_ms,
                    rate_kbps_t cra,
                    rate_kbps_t max_rbdc_kb,
                    time_sf_t rbdc_timeout_sf,
                    vol_pkt_t max_vbdc_pkt,
                    time_sf_t msl_sf,
                    time_sf_t obr_period_sf,
                    bool cr_output_only);

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
	virtual bool hereIsLogonResp(const LogonResponse &response);

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
	virtual bool hereIsTTP(const Ttp &ttp) = 0;

	/**
	 * @brief Build Capacity Requests.
	 *
	 * @param cr_type           CR type to compute CR on.
	 * @param capacity_request  capacity request built.
	 * @param empty             flag if CR is 0.
	 * @return true on success, false otherwise.
	 */
	virtual bool buildCR(cr_type_t cr_type,
	                     CapacityRequest **capacity_request,
	                     bool &empty) = 0;

	/**
	 * @brief Schedule uplink packets emission.
	 *
	 * @param complete_dvb_frames  created DVB frames.
	 * @return true on success, false otherwise.
	 */
	virtual bool uplinkSchedule(std::list<DvbFrame *> *complete_dvb_frames) = 0;

	/**
	 * @brief   Called at each SoF.
	 *
	 * @return  true if success, false otherwise.
	 */
	virtual bool processOnFrameTick();

	/**
	 * @brief  Get the statistics context.
	 *
	 * @return Statistics context.
	 */
	da_stat_context_t getStatsCxt() const;

	/**
	 * @brief Reset the statistics context.
	 */
	void resetStatsCxt();

	/*** Wrappers TODO remove !! ***/
	virtual bool hereIsSOF(unsigned char *buf, size_t len);
	virtual bool hereIsLogonResp(unsigned char *buf, size_t len);

	virtual bool buildCR(cr_type_t type,
	                     unsigned char *frame,
	                     size_t &len,
	                     bool &empty) = 0;
	virtual bool hereIsTTP(unsigned char *buf, size_t len) = 0;


protected:
	/** Flag if initialisation of base class has been done */
	bool is_parent_init;

    /** The packet representation */
	const EncapPlugin::EncapPacketHandler *packet_handler;

	/** The MAC FIFOs */
    const std::map<unsigned int, DvbFifo *> dvb_fifos;

	/** Terminal ID of the ST */
	tal_id_t tal_id;
	/** Group ID of the ST */
	group_id_t group_id;

	/** Current superframe number */
	time_sf_t current_superframe_sf;

	/** Stats context */
	da_stat_context_t stat_context;

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
	vol_pkt_t max_vbdc_pkt;
	/** Minimum Scheduling Latency (in frame number) */
	time_sf_t msl_sf;
	/** OBR period: period between two CR (in frame number) */
	time_sf_t obr_period_sf;
	/** If true, compute only output FIFO size for CR generation */
	bool cr_output_only;

};
#endif


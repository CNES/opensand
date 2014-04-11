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
 * @file    DamaCtrl.h
 * @brief   This class defines the DAMA controller interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#ifndef _DAMA_CONTROLLER_H_
#define _DAMA_CONTROLLER_H_
#endif


#include "Sac.h"
#include "Ttp.h"
#include "TerminalContext.h"
#include "TerminalCategory.h"
#include "FmtSimulation.h"
#include "PepRequest.h"
#include "UnitConverter.h"
#include "OpenSandFrames.h"
#include "Logon.h"
#include "Logoff.h"

#include <opensand_output/Output.h>

#include <cstdio>
#include <map>

using std::map;

/**
 * @class DamaCtrl
 * @brief Define methods to process DAMA request in the NCC.
 *
 * This class is an abstract clas used as a common central point
 * for implementing a set of DAMA.
 *
 */
class DamaCtrl
{
 public:
	// Ctor & Dtor
	DamaCtrl();
	virtual ~DamaCtrl();

	// Initialization
	/**
	 * @brief  Initialize DAMA controller
	 *
	 * @param   frame_duration_ms       duration of the frame (in ms).
	 * @param   frames_per_superframe   The number of frames per superframe
	 * @param   with_phy_layer          Whether the physical layer is enabled or not
	 * @param   packet_length_bytes     The packet length in bytes, for constant length
	 * @param   rbdc_timeout_sf         RBDC timeout in superframe number.
	 * @param   fca_kbps                The FCA maximum value (in kbits/s)
	 * @param   categories              pointer to category list.
	 * @param   terminal_affectation    mapping of terminal Id <-> category
	 * @param   default_category        default category for non-affected
	 *                                  terminals
	 * @param   ret_fmt_simu            The list of simulated up/return FMT
	 * @param   simulated               Whether there is simulated requests
	 * @return  true on success, false otherwise.
	 */
	virtual bool initParent(time_ms_t frame_duration_ms,
	                        unsigned int frames_per_superframe,
	                        bool with_phy_layer,
	                        vol_bytes_t packet_length_bytes,
	                        bool cra_decrease,
	                        time_sf_t rbdc_timeout_sf,
	                        rate_kbps_t fca_kbps,
	                        TerminalCategories categories,
	                        TerminalMapping terminal_affectation,
	                        TerminalCategory *default_category,
	                        FmtSimulation *const ret_fmt_simu,
	                        bool simulated);

	// Protocol frames processing

	/**
	 * @brief  Process a Logon request frame.
	 *
	 * @param   logon  logon request.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsLogon(const LogonRequest *logon);

	/**
	 * @brief  Process a Logoff request frame.
	 *
	 * @param   logoff  logoff request.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsLogoff(const Logoff *logoff);

	/**
	 * @brief  Process a SAC frame.
	 * @warning Should set enable_rbdc or enable_vbdc to true depending on
	 *          the type of CR it receives
	 *
	 * @param   sac             SAC frame.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsSAC(const Sac *sac) = 0;

	/**
	 * @brief  Build the TTP frame.
	 *
	 * @param   ttp  the TTP built.
	 * @return  true on succes, false otherwise.
	 */
	virtual bool buildTTP(Ttp *ttp) = 0;

	/**
	 * @brief  Apply a PEP command
	 *         Update the ST resources allocations according to given PEP request
	 *
	 * @param   request  PEP request
	 * @return  true on success, false otherwise
	 */
	//virtual bool applyPepCommand(const PepRequest &request) = 0;
	virtual bool applyPepCommand(const PepRequest* request) = 0;

	/**
	 * @brief  To be called on each SuperFrame change (when SOF is received)
	 *
	 * @param   superframe_number_sf  frame number
	 * @return  true on success, false otherwise
	 */
	virtual bool runOnSuperFrameChange(time_sf_t superframe_number_sf);

	/**
	 * @brief  Update the DAMA statistics
	 *         Called each frame
	 *
	 * @param period_ms  The period of statistics refreshing
	 */
	void updateStatistics(time_ms_t period_ms);

	/**
	 * @brief Set the file for simulation statistic and events record
	 *
	 * @param event_stream  The events file
	 */
	virtual void setRecordFile(FILE * event_stream);

 protected:

	/**
	 * @brief 	Init the output probes and stats
	 *
	 * @return	true if success, false otherwise.
	 */
	bool initOutput();

	/**
	 * @brief  Create a terminal context.
	 *
	 * @param   terminal        The terminal to create
	 * @param   tal_id          Id of the terminal.
	 * @param   cra_kbps        CRA of the terminal (kb/s).
	 * @param   max_rbdc_kbps   maximum RBDC value (kb/s).
	 * @param   rbdc_timeout_sf RBDC timeout (in superframe number).
	 * @param   max_vbdc_kb     maximum VBDC value (in kbits).
	 * @return  true if success, false otherwise.
	 */
	virtual bool createTerminal(TerminalContext **terminal,
	                            tal_id_t tal_id,
	                            rate_kbps_t cra_kbps,
	                            rate_kbps_t max_rbdc_kbps,
	                            time_sf_t rbdc_timeout_sf,
	                            vol_pkt_t max_vbdc_kb) = 0;

	/**
	 * @brief Run the RBDC computation for DAMA
	 */
	virtual bool runDamaRbdc() = 0;

	/**
	 * @brief Run the VBDC computation for DAMA
	 */
	virtual bool runDamaVbdc() = 0;

	/**
	 * @brief Run the FCA computation for DAMA
	 */
	virtual bool runDamaFca() = 0;

	// Output Log
	OutputLog *log_init;
	OutputLog *log_logon;
	OutputLog *log_super_frame_tick;
	OutputLog *log_run_dama;
	OutputLog *log_sac;
	OutputLog *log_ttp;
	OutputLog *log_pep;
	OutputLog *log_fmt;

	/** Flag if init of THIS DAMA class (DamaCtrl) has been done */
	bool is_parent_init;

	// Helper to simplify context manipulation
	typedef map<tal_id_t, TerminalContext *> DamaTerminalList;

	UnitConverter *converter;  ///< Used to convert from/to KB to encap packets

	/** List of registered terminals */
	// TODO useful? they are in TerminalCategories
	DamaTerminalList terminals;

	/// Physical layer enable
	bool with_phy_layer;

	/** Current SuperFrame number */
	time_sf_t current_superframe_sf;

	/** frame duration (in ms) */
	time_ms_t frame_duration_ms;

	/** number of frames per superframe */
	unsigned int frames_per_superframe;

	/** Decrease request received from ST of CRA value ? */
	bool cra_decrease;

	/** RBDC request timeout (in superframe number) */
	time_sf_t rbdc_timeout_sf;

	/** The maximum available FCA (kbits/s) */
	rate_kbps_t fca_kbps;

	/** Whether RBDC requests are enabled */
	bool enable_rbdc;

	/** Whether VBDC requests are enabled */
	bool enable_vbdc;

	/** Available bandplan (in kHz) */
	freq_khz_t available_bandplan_khz;

	/** List of terminal category configurations. */
	TerminalCategories categories;

	/**
	 * Mapping terminal <-> category.
	 * Used on terminal registration, since the terminal's category is only
	 * defined in the configuration file, which is read by the DVB bloc.
	 */
	TerminalMapping terminal_affectation;

	/**
	 * Default terminal category.
	 * Used on terminals which are not affected to any specific category.
	 */
	TerminalCategory *default_category;

	/** FMT simulation information for up/return link */
	FmtSimulation *ret_fmt_simu;

	/** Roll-off factor */
	double roll_off;

	/** Whethter we used simulated requests */
	bool simulated;

	/**
	 * @brief run the Dama, it allocates exactly what have been asked
	 *        using internal requests, TBTP and contexts.
	 * After DAMA computation, TBTP is completed and context is reinitialized
	 *
	 * @return true on success, false otherwise
	 */
	bool runDama();

	/**
	 * @brief  Reset all Dama settings.
	 *
	 * @return  true on success, false otherwise.
	 */
	virtual bool resetDama() = 0;

	/**
	 * @brief  Update the FMT id for terminal
	 *
	 */
	virtual void updateFmt() = 0;

	/// if set to other than NULL, the fd where recording events
	FILE *event_file;

	/// Output probe and stats

	typedef map<tal_id_t, Probe<int> *> ProbeListPerTerminal;
	typedef map<string, Probe<int> *> ProbeListPerCategory;
	typedef map<string, int> IntListPerCategory;
	typedef map<unsigned int, Probe<int> *> ProbeListPerCarrier;

	/* RBDC request number */
	Probe<int> *probe_gw_rbdc_req_num;
	int gw_rbdc_req_num;

	/* RBDC requested capacity */
	Probe<int> *probe_gw_rbdc_req_size;
	int gw_rbdc_req_size_pktpf;

	/* VBDC request number */
	Probe<int> *probe_gw_vbdc_req_num;
	int gw_vbdc_req_num;

	/* VBDC requested capacity */
	Probe<int> *probe_gw_vbdc_req_size;
	int gw_vbdc_req_size_pkt;

	/* Allocated resources */
		// CRA
	Probe<int> *probe_gw_cra_alloc;
	int gw_cra_alloc_kbps;
		// CRA by ST
	ProbeListPerTerminal probes_st_cra_alloc;
		// RBDC total
	Probe<int> *probe_gw_rbdc_alloc;
	int gw_rbdc_alloc_pktpf;
		// RBDC by ST
	ProbeListPerTerminal probes_st_rbdc_alloc;
		// RBDC max
	Probe<int> *probe_gw_rbdc_max;
	int gw_rbdc_max_kbps;
		// RBDC max by ST
	ProbeListPerTerminal probes_st_rbdc_max;
		// VBDC	total
	Probe<int> *probe_gw_vbdc_alloc;
	int gw_vbdc_alloc_pkt;
		// VBDC by ST
	ProbeListPerTerminal probes_st_vbdc_alloc;
		// FCA total
	Probe<int> *probe_gw_fca_alloc;
	int gw_fca_alloc_pktpf;
		// FCA by ST
	ProbeListPerTerminal probes_st_fca_alloc;

	/* Logged ST number  */
	Probe<int> *probe_gw_st_num;
	int gw_st_num;

		// Total and unused capacity
	Probe<int> *probe_gw_return_total_capacity;
	int gw_return_total_capacity_pktpf;
	Probe<int> *probe_gw_return_remaining_capacity;
	int gw_remaining_capacity_pktpf;
		// Capacity per category
	ProbeListPerCategory probes_category_return_capacity;
	int category_return_capacity_pktpf;
	ProbeListPerCategory probes_category_return_remaining_capacity;
	map<string, int> category_return_remaining_capacity_pktpf;
		// Capacity per carrier
	ProbeListPerCarrier probes_carrier_return_capacity;
	ProbeListPerCarrier probes_carrier_return_remaining_capacity;
	map<unsigned int, int> carrier_return_remaining_capacity_pktpf;

};

#define DC_RECORD_EVENT(fmt,args...) \
{ \
	if (this->event_file != NULL) \
	{ \
		fprintf(this->event_file, "SF%u "fmt"\n", \
		        this->current_superframe_sf, ##args); \
	} \
}


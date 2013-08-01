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


#include "CapacityRequest.h"
#include "Ttp.h"
#include "TerminalContext.h"
#include "TerminalCategory.h"
#include "FmtSimulation.h"
#include "PepRequest.h"
#include "UnitConverter.h"
// TODO remove once we do not use wrappers anymore
#include "lib_dvb_rcs.h"

#include <cstdio>
#include <map>

using std::map;


/// DAMA controller statistics context
//TODO remove
typedef struct
{
	unsigned int terminal_number;         ///> The number of logged terminals
	unsigned int rbdc_requests_number;    ///> The number of RBDC requests
	unsigned int vbdc_requests_number;    ///> The number of VBDC requests
	rate_kbps_t total_capacity_kbps;           ///> The total capacity (kbits/s)
	rate_kbps_t total_cra_kbps;           ///> The total CRA (kbits/s)
	rate_kbps_t total_max_rbdc_kbps;      ///> The total RBDC max value (kbits/s)
	rate_kbps_t rbdc_requests_sum_kbps;   ///> The sum of RBDC requests (kbits/s)
	vol_kb_t vbdc_requests_sum_kb;        ///> The sum of VBDC requests (kbits)
	rate_kbps_t rbdc_allocation_kbps;     ///> The RBDC allocation (kbits/s)
	vol_kb_t vbdc_allocation_kb;          ///> The VBDC allocation (kbits)
	double fair_share;                    ///> The fair share ratio
} dc_stat_context_t;


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
	 * @param   packet_length_bytes     The packet length in bytes, for constant length
	 * @param   max_rbdc_kbps           RBDC maximal value (kb/s).
	 * @param   rbdc_timeout_sf         RBDC timeout in superframe number.
	 * @param   min_vbdc_pkt            VBDC minimal value (in packets/cells number).
	 * @param   fca_kbps                The FCA maximum value (in kbits/s)
	 * @param   available_bandplan_khz  available bandplan (in MHz).
	 * @param   roll_off                roll-off factor
	 * @param   categories              pointer to category list.
	 * @param   terminal_affectation    mapping of terminal Id <-> category
	 * @param   default_category        default category for non-affected
	 *                                  terminals
	 * @param   fmt_simu  The list of simulated FMT
	 * @return  true on success, false otherwise.
	 */
	virtual bool initParent(time_ms_t frame_duration_ms,
	                        unsigned int frames_per_superframe,
	                        vol_bytes_t packet_length_bytes,
	                        bool cra_decrease,
	                        rate_kbps_t max_rbdc_kbps,
	                        time_sf_t rbdc_timeout_sf,
	                        vol_pkt_t min_vbdc_pkt,
	                        rate_kbps_t fca_kbps,
	                        freq_khz_t available_bandplan_khz,
	                        double roll_off,
	                        TerminalCategories categories,
	                        TerminalMapping terminal_affectation,
	                        TerminalCategory *default_category,
	                        const FmtSimulation *fmt_simu);

	// Protocol frames processing

	/**
	 * @brief  Process a Logon request frame.
	 *
	 * @param   logon  logon request.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsLogon(const LogonRequest &logon);

	/**
	 * @brief  Process a Logoff request frame.
	 *
	 * @param   logoff  logoff request.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsLogoff(const LogoffRequest &logoff);

	/**
	 * @brief  Process a Capacity Request frame.
	 * @warning Should set enable_rbdc or enable_vbdc to true depending on
	 *          the type of CR it receives
	 *
	 * @param   capacity_request  capacity Request frame.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsCR(const CapacityRequest &capacity_request) = 0;

	/**
	 * @brief  Build the TTP frame.
	 *
	 * @param   ttp  the TTP built.
	 * @return  true on succes, false otherwise.
	 */
	virtual bool buildTTP(Ttp &ttp) = 0;

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
	 */
	virtual void updateStatistics() = 0;

	/**
	 * @brief Set the file for simulation statistic and events record
	 *
	 * @param event_stream  The events file
	 */
	virtual void setRecordFile(FILE * event_stream);

 protected:

	/**
	 * @brief  Create a terminal context.
	 *
	 * @param   terminal        The terminal to create
	 * @param   tal_id          Id of the terminal.
	 * @param   cra_kbps        CRA of the terminal (kb/s).
	 * @param   max_rbdc_kbps   maximum RBDC value (kb/s).
	 * @param   rbdc_timeout_sf RBDC timeout (in superframe number).
	 * @param   min_vbdc_pkt    minimal VBDC value (in packets/cells number).
	 * @return  true if success, false otherwise.
	 */
	virtual bool createTerminal(TerminalContext **terminal,
	                            tal_id_t tal_id,
	                            rate_kbps_t cra_kbps,
	                            rate_kbps_t max_rbdc_kbps,
	                            time_sf_t rbdc_timeout_sf,
	                            vol_pkt_t min_vbdc_pkt) = 0;

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

	/** Flag if init of THIS DAMA class (DamaCtrl) has been done */
	bool is_parent_init;

	// Helper to simplify context manipulation
	typedef map<tal_id_t, TerminalContext *> DamaTerminalList;

	UnitConverter *converter;  ///< Used to convert from/to KB to encap packets

	/** List of registered terminals */
	// TODO useful? they are in TerminalCategories
	DamaTerminalList terminals;

	/** Current SuperFrame number */
	time_sf_t current_superframe_sf;

	/** frame duration (in ms) */
	time_ms_t frame_duration_ms;

	/** number of frames per superframe */
	unsigned int frames_per_superframe;

	/** Decrease request received from ST of CRA value ? */
	bool cra_decrease;

	/** Max RBDC request value (kb/s) */
	rate_kbps_t max_rbdc_kbps;

	/** RBDC request timeout (in superframe number) */
	time_sf_t rbdc_timeout_sf;

	/** Min VBDC request value (in packets/cells number) */
	vol_pkt_t min_vbdc_pkt;

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

	/** FMT simulation information */
	const FmtSimulation *fmt_simu;

	/** Roll-off factor */
	double roll_off;

	/** Stats context */
	dc_stat_context_t stat_context;

	/**
	 * @brief run the Dama, it allocates exactly what have been asked
	 *        using internal SACT, TBTP and context.
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

	/**
	 * @brief  Compute the bandplan.
	 *
	 * Compute available carrier frequency for each carriers group in each
	 * category, according to the current number of users in these groups.
	 *
	 * @return  true on success, false otherwise.
	 */
	bool computeBandplan();


	// TODO ofstream
	FILE *event_file;  ///< if set to other than NULL, the fd where recording
	                   ///< event
};

// TODO replace !!!
/**
 * used to record event only valid if event_file != NULL
 */
#define DC_RECORD_EVENT(fmt,args...){                                 \
	if (this->event_file != NULL) {                                         \
		fprintf(this->event_file, "SF%u "fmt"\n", this->current_superframe_sf, ##args); \
	}                                                                   \
}



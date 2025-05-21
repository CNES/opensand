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
 * @file    DamaCtrl.h
 * @brief   This class defines the DAMA controller interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef DAMA_CONTROLLER_H
#define DAMA_CONTROLLER_H


#include "Sac.h"
#include "Ttp.h"
#include "TerminalContextDama.h"
#include "TerminalCategoryDama.h"
#include "StFmtSimu.h"
#include "PepRequest.h"
#include "OpenSandFrames.h"
#include "Logon.h"
#include "Logoff.h"

#include <cstdio>
#include <map>


class OutputLog;
template<typename> class Probe;


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
	DamaCtrl( spot_id_t spot);
	virtual ~DamaCtrl();


	// Initialization
	/**
	 * @brief  Initialize DAMA controller
	 *
	 * @param   frame_duration          duration of the frame.
	 * @param   rbdc_timeout_sf         RBDC timeout in superframe number.
	 * @param   fca_kbps                The FCA maximum value (in kbits/s)
	 * @param   categories              pointer to category list.
	 * @param   terminal_affectation    mapping of terminal Id <-> category
	 * @param   default_category        default category for non-affected
	 *                                  terminals
	 * @param   input_sts               The list of Sts with modcod information for input
	 * @param   input_modcod_def        The MODCOD definition table for input link
	 * @param   simulated               Whether there is simulated requests
	 * @return  true on success, false otherwise.
	 */
	virtual bool initParent(time_us_t frame_duration,
	                        time_sf_t rbdc_timeout_sf,
	                        rate_kbps_t fca_kbps,
	                        TerminalCategories<TerminalCategoryDama> categories,
	                        TerminalMapping<TerminalCategoryDama> terminal_affectation,
	                        std::shared_ptr<TerminalCategoryDama> default_category,
	                        std::shared_ptr<const StFmtSimuList> input_sts,
	                        FmtDefinitionTable *const input_modcod_def,
	                        bool simulated);

	// Protocol frames processing

	/**
	 * @brief  Process a Logon request frame.
	 *
	 * @param   logon  logon request.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsLogon(Rt::Ptr<LogonRequest> logon);

	/**
	 * @brief  Process a Logoff request frame.
	 *
	 * @param   logoff  logoff request.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsLogoff(Rt::Ptr<Logoff> logoff);

	/**
	 * @brief  Process a SAC frame.
	 * @warning Should set enable_rbdc or enable_vbdc to true depending on
	 *          the type of CR it receives
	 *
	 * @param   sac             SAC frame.
	 * @return  true on success, false otherwise.
	 */
	virtual bool hereIsSAC(Rt::Ptr<Sac> sac) = 0;

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
	virtual bool applyPepCommand(std::unique_ptr<PepRequest> request) = 0;

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
	 * @brief  Update the required FMTs
	 */
	virtual void updateRequiredFmts() = 0;

	/**
	 * @brief Set the file for simulation statistic and events record
	 *
	 * @param event_stream  The events file
	 */
	virtual void setRecordFile(std::ostream *event_stream);

	/**
	 * @brief    Get a pointer to the categories
	 * @warning  the categories can be modified
	 *
	 * @return   pointer to the categories
	 */
	TerminalCategories<TerminalCategoryDama> *getCategories();

protected:
	/**
	 * @brief 	Init the output probes and stats
	 *
	 * @return	true on success, false otherwise.
	 */
	bool initOutput();

	/**
	 * @brief  Generate a probe for Gw capacity
	 *
	 * @param name            the probe name
	 * @return                the probe
	 */
	virtual std::shared_ptr<Probe<int>> generateGwCapacityProbe(std::string name) const = 0;

	/**
	 * @brief  Generate a probe for category capacity
	 *
	 * @param name            the probe name
	 * @param category_label  the category label
	 * @return                the probe
	 */
	virtual std::shared_ptr<Probe<int>> generateCategoryCapacityProbe(
		std::string category_label,
		std::string name) const = 0;

	/**
	 * @brief  Generate a probe for carrier capacity
	 *
	 * @param name            the probe name
	 * @param category_label  the category label
	 * @param carrier_id      the carrier id
	 * @return                the probe
	 */
	virtual std::shared_ptr<Probe<int>> generateCarrierCapacityProbe(
		std::string category_label,
		unsigned int carrier_id,
		std::string name) const = 0;

	/**
	 * @brief  Create a terminal context.
	 *
	 * @param   terminal        The terminal to create
	 * @param   tal_id          Id of the terminal.
	 * @param   cra_kbps        CRA of the terminal (kb/s).
	 * @param   max_rbdc_kbps   maximum RBDC value (kb/s).
	 * @param   rbdc_timeout_sf RBDC timeout (in superframe number).
	 * @param   max_vbdc_kb     maximum VBDC value (in kbits).
	 * @return  true on success, false otherwise.
	 */
	virtual bool createTerminal(std::shared_ptr<TerminalContextDama> &terminal,
	                            tal_id_t tal_id,
	                            rate_kbps_t cra_kbps,
	                            rate_kbps_t max_rbdc_kbps,
	                            time_sf_t rbdc_timeout_sf,
	                            vol_kb_t max_vbdc_kb) = 0;

	/**
	 * @brief  Reset the capacity of carriers
	 *
	 * @return  true on success, false otherwise
	 */
	virtual bool resetCarriersCapacity() = 0;

	/**
	 * @brief  Update all wave forms.
	 *
	 * @return  true on success, false otherwise.
	 */
	virtual bool updateWaveForms() = 0;

	/**
	 * @brief Compute the terminals alllocations, it allocates exactly what
	 *        have been asked using internal requests, TBTP and contexts.
	 * After te termianls allocations, TBTP is completed and context is reinitialized
	 *
	 * @return true on success, false otherwise
	 */
	bool computeTerminalsAllocations();

	/**
	 * @brief  Reset all terminals allocations.
	 *
	 * @return  true on success, false otherwize
	 */
	virtual bool resetTerminalsAllocations() = 0;

	/**
	 * @brief Compute the terminals CRA allocation
	 */
	virtual bool computeTerminalsCraAllocation() = 0;

	/**
	 * @brief Compute the terminals RBDC allocation
	 */
	virtual bool computeTerminalsRbdcAllocation() = 0;

	/**
	 * @brief Compute the terminals VBDC allocation
	 */
	virtual bool computeTerminalsVbdcAllocation() = 0;

	/**
	 * @brief Compute the terminals FCA allocation
	 */
	virtual bool computeTerminalsFcaAllocation() = 0;

	/**
	 * @brief Get the context of terminal
	 *
	 * @param tal_id      The terminal id
	 * @return            The context of the terminal
	 *
	 */
	virtual std::shared_ptr<TerminalContextDama> getTerminalContext(tal_id_t tal_id) const;

	// Output Log
	std::shared_ptr<OutputLog> log_init;
	std::shared_ptr<OutputLog> log_logon;
	std::shared_ptr<OutputLog> log_super_frame_tick;
	std::shared_ptr<OutputLog> log_run_dama;
	std::shared_ptr<OutputLog> log_sac;
	std::shared_ptr<OutputLog> log_ttp;
	std::shared_ptr<OutputLog> log_pep;
	std::shared_ptr<OutputLog> log_fmt;

	/** Flag if init of THIS DAMA class (DamaCtrl) has been done */
	bool is_parent_init;

	// Helper to simplify context manipulation
	typedef std::map<tal_id_t, std::shared_ptr<TerminalContextDama>> DamaTerminalList;

	/** List of registered terminals */
	DamaTerminalList terminals;

	/** Current SuperFrame number */
	time_sf_t current_superframe_sf;

	/** frame duration (in ms) */
	time_us_t frame_duration;

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
	TerminalCategories<TerminalCategoryDama> categories;

	/**
	 * Mapping terminal <-> category.
	 * Used on terminal registration, since the terminal's category is only
	 * defined in the configuration file, which is read by the DVB bloc.
	 */
	TerminalMapping<TerminalCategoryDama> terminal_affectation;

	/**
	 * Default terminal category.
	 * Used on terminals which are not affected to any specific category.
	 */
	std::shared_ptr<TerminalCategoryDama> default_category;

	/** list of Sts with modcod informations for input link */
	std::shared_ptr<const StFmtSimuList> input_sts;

	/** Fmt Definition table for input link */
	FmtDefinitionTable *input_modcod_def;

	/** Whethter we used simulated requests */
	bool simulated;

	/// if set to other than NULL, the fd where recording events
	std::ostream* event_file;
	template<typename Arg, typename... Args>
	void record_event(Arg&& arg, Args&&... args);

	/// Output probe and stats

	typedef std::map<tal_id_t, std::shared_ptr<Probe<int> > > ProbeListPerTerminal;
	typedef std::map<std::string, std::shared_ptr<Probe<int> > > ProbeListPerCategory;
	typedef std::map<unsigned int, std::shared_ptr<Probe<int> > > ProbeListPerCarrier;
	typedef std::map<std::string, ProbeListPerCarrier> ProbeListPerCategoryPerCarrier;

	/* RBDC request number */
	std::shared_ptr<Probe<int>> probe_gw_rbdc_req_num;
	int gw_rbdc_req_num;

	/* RBDC requested capacity */
	std::shared_ptr<Probe<int>> probe_gw_rbdc_req_size;

	/* VBDC request number */
	std::shared_ptr<Probe<int>> probe_gw_vbdc_req_num;
	int gw_vbdc_req_num;

	/* VBDC requested capacity */
	std::shared_ptr<Probe<int>> probe_gw_vbdc_req_size;

	/* Allocated resources */
		// CRA
	std::shared_ptr<Probe<int>> probe_gw_cra_alloc;
	int gw_cra_alloc_kbps;
		// CRA by ST
	ProbeListPerTerminal probes_st_cra_alloc;
		// RBDC total
	std::shared_ptr<Probe<int>> probe_gw_rbdc_alloc;
		// RBDC by ST
	ProbeListPerTerminal probes_st_rbdc_alloc;
		// RBDC max
	std::shared_ptr<Probe<int>> probe_gw_rbdc_max;
	int gw_rbdc_max_kbps;
		// RBDC max by ST
	ProbeListPerTerminal probes_st_rbdc_max;
		// VBDC	total
	std::shared_ptr<Probe<int>> probe_gw_vbdc_alloc;
		// VBDC by ST
	ProbeListPerTerminal probes_st_vbdc_alloc;
		// FCA total
	std::shared_ptr<Probe<int>> probe_gw_fca_alloc;
		// FCA by ST
	ProbeListPerTerminal probes_st_fca_alloc;

	/* Logged ST number  */
	std::shared_ptr<Probe<int>> probe_gw_st_num;
	int gw_st_num;

		// Total and unused capacity
	std::shared_ptr<Probe<int>> probe_gw_return_total_capacity;
	std::shared_ptr<Probe<int>> probe_gw_return_remaining_capacity;
	int gw_remaining_capacity;
		// Capacity per category
	ProbeListPerCategory probes_category_return_capacity;
	ProbeListPerCategory probes_category_return_remaining_capacity;
	std::map<std::string, int> category_return_remaining_capacity;
		// Capacity per carrier
	ProbeListPerCategoryPerCarrier probes_carrier_return_capacity;
	ProbeListPerCategoryPerCarrier probes_carrier_return_remaining_capacity;
	std::map<std::string, std::map<unsigned int, int> >  carrier_return_remaining_capacity;

	// Spot ID
	spot_id_t spot_id;

	std::string output_prefix;
};


template<typename Arg, typename... Args>
void DamaCtrl::record_event(Arg&& arg, Args&&... args)
{
	if (this->event_file)
	{
		(*event_file) << "SF" << this->current_superframe_sf << ' ' << std::forward<Arg>(arg);
		(((*event_file) << std::forward<Args>(args)), ...);
		(*event_file) << "\n";
	}
}


#endif

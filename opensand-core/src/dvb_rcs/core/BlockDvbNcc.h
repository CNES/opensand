/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file BLockDvbNcc.h
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author SatIP6
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 *
 * <pre>
 *
 *            ^
 *            | encap burst
 *            v
 *    ------------------
 *   |                  |
 *   |  DVB-RCS Ncc     |
 *   |  Dama Controler  |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame / BBFrame
 *            v
 *
 * </pre>
 *
 */

#ifndef BLOCK_DVB_NCC_H
#define BLOCk_DVB_NCC_H

#include "BlockDvb.h"

#include "DamaCtrlRcs.h"
#include "NccPepInterface.h"
#include "Scheduling.h"


class BlockDvbNcc: public BlockDvb, NccPepInterface
{

 private:

	/// The DAMA controller
	DamaCtrlRcs *dama_ctrl;

	/// The uplink of forward scheduling depending on satellite
	Scheduling *scheduling;

	/// carrier ids
	long m_carrierIdDvbCtrl;
	long m_carrierIdSOF;
	long m_carrierIdData;

	/// frame timer, used to awake the block every frame period
	event_id_t frame_timer;

	/// ST unique mac id (configuration param)
	tal_id_t macId;

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	std::list<DvbFrame *> complete_dvb_frames;

	/// The TTP
	Ttp ttp;

	/// The SAC
	Sac sac;

	/// timer used to awake the block every second in order to retrieve
	/// the current MODCODs
	event_id_t scenario_timer;

	/// a fifo to keep the received packet from encap bloc
	DvbFifo data_dvb_fifo;

	/// The terminal categories for forward band
	TerminalCategories categories;

	/// The terminal affectation for forward band
	TerminalMapping terminal_affectation;

	/// The default terminal category for forward band
	TerminalCategory *default_category;

	// TODO remove FMT groups from attributes
	/// FMT groups for down/forward
	fmt_groups_t fwd_fmt_groups;

	/// FMT groups for up/return
	fmt_groups_t ret_fmt_groups;

	/// The C/N0 for downlink in regenerative scenario
	double cni;

	/**** NGN network / Policy Enforcement Point (PEP) ****/

	/// timer used for applying resources allocations received from PEP
	event_id_t pep_cmd_apply_timer;

	/// Delay for allocation requests from PEP (in ms)
	int pepAllocDelay;

	/// parameters for request simulation
	FILE *event_file;
	FILE *stat_file;
	FILE *simu_file;
    enum
	{
		none_simu,
		file_simu,
		random_simu,
	} simulate;
	long simu_st;
	long simu_rt;
	long simu_max_rbdc;
	long simu_max_vbdc;
	long simu_cr;
	long simu_interval;
	event_id_t simu_timer;

	//events

	/// logon request reveived
	Event *event_logon_req;
	/// logon response sent
	Event *event_logon_resp;

	map<uint16_t, uint16_t> column_list;

	// TODO: a specific timer for statistics update
	/*
	/// The statistics period
	unsigned int stats_period_ms;

	/// Statistics timer
	event_id_t stats_timer;
	*/

 public:

	/// Class constructor
	/// Use mgl_bloc default constructor
	BlockDvbNcc(const string &name);

	~BlockDvbNcc();

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();

	/// number of the next BBFrame
	int nb_sequencing;


	/* Methods */

 private:

	/** Read configuration for the request simulation
	 *
	 * @return  true on success, false otherwise
	 */
	bool initRequestSimulation();

	/**
	 * Read configuration for the downward timers
	 *
	 * @return  true on success, false otherwise
	 */
	bool initDownwardTimers();

	/**
	 * Read configuration for simulated FMT columns ID
	 *
	 * @return  true on success, false otherwise
	 */
	bool initColumns();

	/**
	 * @brief Initialize the transmission mode
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMode();

	/**
	 * Read configuration for the carrier IDs
	 *
	 * @return  true on success, false otherwise
	 */
	bool initCarrierIds();

	/**
	 * @brief Read configuration for the different files and open them
	 *
	 * @return  true on success, false otherwise
	 */
	bool initFiles();

	/**
	 * Read configuration for the DAMA algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	bool initDama();

	/**
	 * @brief Read configuration for the FIFO
	 *
	 * @return  true on success, false otherwise
	 */
	bool initFifo();

	/**
	 * @brief Initialize the statistics
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput(void);

	/// DVB frame from lower layer
	bool onRcvDvbFrame(unsigned char *ip_buf, int l_len);
	void onRcvLogonReq(unsigned char *ip_buf, int l_len);
	void onRcvLogoffReq(unsigned char *ip_buf, int l_len);

	// NCC functions
	void sendTTP();
	void sendSOF();

	// event simulation

	/**
	 * Simulate event based on an input file
	 * @return true on success, false otherwise
	 */
	bool simulateFile();

	/**
	 * Simulate event based on random generation
	 */
	void simulateRandom();

	/**
	 * @brief Send a SAC message containing ACM parameters
	 *
	 * @return true on success, false otherwise
	 */
	bool sendAcmParameters();

	// Output probes and stats

		// Rates

			// Layer 2 to SAT
	Probe<int> *probe_gw_l2_to_sat_before_sched;
	int l2_to_sat_bytes_before_sched;
	Probe<int> *probe_gw_l2_to_sat_after_sched;
	int l2_to_sat_bytes_after_sched;
			// PHY to SAT
	Probe<int> *probe_gw_phy_to_sat;
			// Layer 2 from SAT
	Probe<int> *probe_gw_l2_from_sat;
	int l2_from_sat_bytes;
			// PHY from SAT
	Probe<int> *probe_gw_phy_from_sat;
	int phy_from_sat_bytes;

		// Frame interval
	Probe<float> *probe_frame_interval;

		// Queue sizes
	Probe<int> *probe_gw_queue_size;
	Probe<int> *probe_gw_queue_size_kb;


	/// TODO following



	// statistics update
	void updateStatsOnFrame();
};

#endif

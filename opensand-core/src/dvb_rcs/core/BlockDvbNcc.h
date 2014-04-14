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

#define SIMU_BUFF_LEN 255

class BlockDvbNcc: public BlockDvb
{

 public:

	/// Class constructor
	BlockDvbNcc(const string &name);

	~BlockDvbNcc();

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();

	/// number of the next BBFrame
	int nb_sequencing;


	class Upward: public DvbUpward
	{
	 public:
		Upward(Block *const bl);
		~Upward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 protected:

		/**
		 * @brief Initialize the transmission mode
		 *
		 * @return  true on success, false otherwise
		 */
		bool initMode(void);

		/**
		 * @brief Initialize the statistics
		 *
		 * @return  true on success, false otherwise
		 */
		bool initOutput(void);

		/// DVB frame from lower layer
		bool onRcvDvbFrame(DvbFrame *dvb_frame);
		bool onRcvLogonReq(DvbFrame *dvb_frame);
		bool onRcvLogoffReq(DvbFrame *dvb_frame);

		// statistics update
		void updateStats(void);

		/**
		 * Transmist a frame to the opposite channel
		 *
		 * @param frame  The dvb frame
		 * @return true on success, false otherwise
		 */
		bool shareFrame(DvbFrame *frame);

		/// ST unique mac id
		tal_id_t mac_id;

		// Output probes and stats
			// Rates
				// Layer 2 from SAT
		Probe<int> *probe_gw_l2_from_sat;
		int l2_from_sat_bytes;
			// Physical layer information
		Probe<int> *probe_received_modcod;
		Probe<int> *probe_rejected_modcod;

		/// logon request received
		OutputEvent *event_logon_req;
	};


	class Downward: public DvbDownward, NccPepInterface
	{
	  public:
		Downward(Block *const bl);
		~Downward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 protected:

		/**
		 * Read configuration for the downward timers
		 *
		 * @return  true on success, false otherwise
		 */
		bool initTimers(void);

		/**
		 * Read configuration for the carrier IDs
		 *
		 * @return  true on success, false otherwise
		 */
		bool initCarrierIds(void);

		/**
		 * @brief Initialize the transmission mode
		 *
		 * @return  true on success, false otherwise
		 */
		bool initMode(void);

		/**
		 * Read configuration for the DAMA algorithm
		 *
		 * @return  true on success, false otherwise
		 */
		bool initDama(void);

		/**
		 * @brief Read configuration for the FIFO
		 *
		 * @return  true on success, false otherwise
		 */
		bool initFifo(void);

		/**
		 * @brief Read configuration for the different files and open them
		 *
		 * @return  true on success, false otherwise
		 */
		bool initModcodSimu(void);

		/**
		 * Read configuration for simulated FMT columns ID
		 *
		 * @return  true on success, false otherwise
		 */
		bool initColumns(void);

		/**
		 * @brief Initialize the statistics
		 *
		 * @return  true on success, false otherwise
		 */
		bool initOutput(void);

		/** Read configuration for the request simulation
		 *
		 * @return  true on success, false otherwise
		 */
		bool initRequestSimulation(void);

		/**
		 * Send a Terminal Time Plan
		 */
		void sendTTP(void);

		/**
		 * Send a start of frame
		 */
		void sendSOF(void);

		/**
		 * @brief Send a SAC message containing ACM parameters
		 *
		 * @return true on success, false otherwise
		 */
		bool sendAcmParameters(void);

		/**
		 * @brief Handle a DVB frame transmitted from upward channel
		 *
		 * @param dvb_frame  The frame
		 * @return true on success, false otherwise
		 */
		bool handleDvbFrame(DvbFrame *dvb_frame);

		/**
		 *  @brief Handle a logon request transmitted by the opposite
		 *         block
		 *
		 *  @param dvb_frame  The frame contining the logon request
		 *  @return true on success, false otherwise
		 */
		bool handleLogonReq(DvbFrame *dvb_frame);
		/**
		 *  @brief Handle a logoff request transmitted by the opposite
		 *         block
		 *
		 *  @param dvb_frame  The frame contining the logoff request
		 *  @return true on success, false otherwise
		 */
		bool handleLogoffReq(DvbFrame *dvb_frame);

		// statistics update
		void updateStats(void);

		/**
		 * Simulate event based on an input file
		 * @return true on success, false otherwise
		 */
		bool simulateFile(void);

		/**
		 * Simulate event based on random generation
		 */
		void simulateRandom(void);

		/// The DAMA controller
		DamaCtrlRcs *dama_ctrl;

		/// The uplink of forward scheduling depending on satellite
		Scheduling *scheduling;

		/// frame timer for return, used to awake the block every frame period
		event_id_t frame_timer;

		/// frame timer for forward, used to awake the block every frame period
		event_id_t fwd_timer;

		/// counter for forward frames
		time_sf_t fwd_frame_counter;

		/// carrier ids
		uint8_t ctrl_carrier_id;
		uint8_t sof_carrier_id;
		uint8_t data_carrier_id;

		/// a fifo to keep the received packet from encap bloc
		DvbFifo *data_dvb_fifo;

		/// the list of complete DVB-RCS/BB frames that were not sent yet
		std::list<DvbFrame *> complete_dvb_frames;

		/// The terminal categories for forward band
		TerminalCategories categories;

		/// The terminal affectation for forward band
		TerminalMapping terminal_affectation;

		/// The default terminal category for forward band
		TerminalCategory *default_category;

		/// The up/return packet handler
		EncapPlugin::EncapPacketHandler *up_return_pkt_hdl;

		// TODO remove FMT groups from attributes
		/// FMT groups for down/forward
		fmt_groups_t fwd_fmt_groups;

		/// FMT groups for up/return
		fmt_groups_t ret_fmt_groups;

		/// The MODCOD simulation elements for up/return link
		FmtSimulation up_ret_fmt_simu;
		/// The MODCOD simulation elements for down/forward link
		FmtSimulation down_fwd_fmt_simu;

		/// timer used to awake the block every second in order to retrieve
		/// the current MODCODs
		/// In regenerative case with physical layer, is it used to send
		// ACM parameters to satellite
		event_id_t scenario_timer;

		/// The C/N0 for downlink in regenerative scenario that will be transmited
		//  to satellite in SAC
		//  For transparent scenario the return link cni will be used to update return
		//  MODCOD id for terminals (not this one)
		double cni;

		/// The column ID for FMT simulation
		map<tal_id_t, uint16_t> column_list;

		/// timer used for applying resources allocations received from PEP
		event_id_t pep_cmd_apply_timer;

		/// Delay for allocation requests from PEP (in ms)
		int pep_alloc_delay;

		/// parameters for request simulation
		FILE *event_file;
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
		bool simu_eof;
		char simu_buffer[SIMU_BUFF_LEN];
		event_id_t simu_timer;

		// Output probes and stats
			// Rates
				// Layer 2 to SAT
		Probe<int> *probe_gw_l2_to_sat_before_sched;
		int l2_to_sat_bytes_before_sched;
		Probe<int> *probe_gw_l2_to_sat_after_sched;
		int l2_to_sat_bytes_after_sched;
			// Frame interval
		Probe<float> *probe_frame_interval;
			// Queue sizes
		Probe<int> *probe_gw_queue_size;
		Probe<int> *probe_gw_queue_size_kb;
			// Physical layer information
		Probe<int> *probe_used_modcod;

		// Output logs and events
		OutputLog *log_request_simulation;

		/// logon response sent
		OutputEvent *event_logon_resp;
	};
};

#endif

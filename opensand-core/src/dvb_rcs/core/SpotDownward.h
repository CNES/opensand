/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file SpotDownward.h
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SPOT_DOWNWARD_H
#define SPOT_DOWNWARD_H

#include "BlockDvb.h"
#include "DamaCtrlRcs.h"
#include "Scheduling.h"
#include "SlottedAlohaNcc.h"

#define SIMU_BUFF_LEN 255

enum Simulate
{
	none_simu,
	file_simu,
	random_simu,
} ;


class SpotDownward: public DvbChannel, public NccPepInterface
{
	public:
		SpotDownward(spot_id_t spot_id,
		             tal_id_t mac_id,
		             time_ms_t fwd_down_frame_duration,
		             time_ms_t ret_up_frame_duration,
		             time_ms_t stats_period,
		             sat_type_t sat_type,
		             EncapPlugin::EncapPacketHandler *pkt_hdl,
		             bool phy_layer);
		~SpotDownward();
		bool onInit(void);

		/**
		 * @brief Handle the Slotted Aloha ACKs
		 *
		 * @param ack_frames  The Slotted Aloha ACKs
		 * @return true on success, false otherwise
		 */
		bool handleSalohaAcks(const list<DvbFrame *> *ack_frames);

		/**
		 * @brief Handle an encapsulated packet
		 *
		 * @param packet  The encapsulated packet
		 * @return true on success, false otherwise
		 */
		bool handleEncapPacket(NetPacket *packet);
	
		/**
		 * @brief Handle a logon request transmitted by the opposite
		 *        block
		 *
		 * @param logon_req  The frame contining the logon request
		 * @return true on success, false otherwise
		 */
		bool handleLogonReq(const LogonRequest *logon_req);

		/**
		 * @brief Handle a logoff request transmitted by the opposite
		 *        block
		 *
		 * @param dvb_frame  The frame contining the logoff request
		 * @return true on success, false otherwise
		 */
		bool handleLogoffReq(const DvbFrame *dvb_frame);

		/**
		 * @brief check if Dama is existing
		 *
		 * @return true if Dama is null, false otherwise
		 */
		bool checkDama();
		
		/**
		 * @brief handler a frame timer and update frame counter
		 *
		 * @param super_frame_counter  the superframe counter
		 * @return true on success, false otherwise
		 */
		bool handleFrameTimer(time_sf_t super_frame_counter);
		
		/**
		 * @brief handler a forward frame timer and update forward frame counter
		 *
		 * @param fwd_frame_counter  The forward frame counter
		 * @return true on success, false otherwise
		 */
		bool handleFwdFrameTimer(time_sf_t fwd_frame_counter);

		/**
		 * @brief  handle a SAC frame
		 *
		 * @param dvb_frame The SAC frame
		 * @return true on success, false otherwise
		 */
		bool handleSac(const DvbFrame *dvb_frame);

		/**
		 * @brief handle Corrupted Dvb Frame
		 *
		 * @param dvb_frame the Dvb Frame corrupted
		 * @return true on succes, flase otherwise
		 */
		bool handleCorruptedFrame(DvbFrame *dvb_frame);

		/**
		 * @brief go to fmt simu next scenario
		 *
		 * @return true on success, false otherwise
		 */
		bool goNextScenarioStep();

		/**
		 * @brief update FMT in DAMA controller
		 */
		void updateFmt(void);

		/**
		 * @briel apply pep commande
		 * @param pep_request the pep request
		 * @return true on success, false otherwise
		 */
		bool applyPepCommand(PepRequest *pep_request);

		/**
		 * @brief Build a TTP
		 *
		 * @param ttp  OUT: The TTP
		 * @return true on success, false otherwise
		 */
		bool buildTtp(Ttp *ttp);

		double getCni(void) const;
		void setCni(double cni);

		uint8_t getCtrlCarrierId(void) const;
		uint8_t getSofCarrierId(void) const;
		uint8_t getDataCarrierId(void) const;

		list<DvbFrame *> &getCompleteDvbFrames(void);
		
		void setPepCmdApplyTimer(event_id_t pep_cmd_a_timer);
		event_id_t getPepCmdApplyTimer(void);

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
		 * Simulate event based on an input file
		 * @return true on success, false otherwise
		 */
		bool simulateFile(void);

		/**
		 * Simulate event based on random generation
		 * @return true on success, false otherwise
		 */
		bool simulateRandom(void);

		// statistics update
		void updateStatistics(void);

		/// The DAMA controller
		DamaCtrlRcs *dama_ctrl;

		/// The uplink of forward scheduling depending on satellite
		Scheduling *scheduling;

		/// counter for forward frames
		time_sf_t fwd_frame_counter;

		/// carrier ids
		uint8_t ctrl_carrier_id;
		uint8_t sof_carrier_id;
		uint8_t data_carrier_id;

		/// spot id
		uint8_t spot_id;
		
		// gw tal id
		uint8_t mac_id;

		/* Fifos */
		/// map of FIFOs per MAX priority to manage different queues
		fifos_t dvb_fifos;
		/// the default MAC fifo index = fifo with the smallest priority
		unsigned int default_fifo_id;

		/// the list of complete DVB-RCS/BB frames that were not sent yet
		list<DvbFrame *> complete_dvb_frames;

		/// The terminal categories for forward band
		TerminalCategories<TerminalCategoryDama> categories;

		/// The terminal affectation for forward band
		TerminalMapping<TerminalCategoryDama> terminal_affectation;

		/// The default terminal category for forward band
		TerminalCategoryDama *default_category;

		/// The up/return packet handler
		EncapPlugin::EncapPacketHandler *up_return_pkt_hdl;

		// TODO remove FMT groups from attributes
		// TODO we may create a class that inherit from fmt_groups_t (map) with
		//      a destructor that erases the map elements
		/// FMT groups for down/forward
		fmt_groups_t fwd_fmt_groups;

		/// FMT groups for up/return
		fmt_groups_t ret_fmt_groups;

		/// The MODCOD simulation elements for up/return link
		FmtSimulation up_ret_fmt_simu;
		/// The MODCOD simulation elements for down/forward link
		FmtSimulation down_fwd_fmt_simu;

		/// The C/N0 for downlink in regenerative scenario that will be transmited
		//  to satellite in SAC
		//  For transparent scenario the return link cni will be used to update return
		//  MODCOD id for terminals (not this one)
		double cni;

		/// The column ID for FMT simulation
		map<tal_id_t, uint16_t> column_list;

		/// timer used for applying resources allocations received from PEP
		event_id_t pep_cmd_apply_timer;

		/// parameters for request simulation
		FILE *event_file;
		FILE *simu_file;
		Simulate simulate;
		long simu_st;
		long simu_rt;
		long simu_max_rbdc;
		long simu_max_vbdc;
		long simu_cr;
		long simu_interval;
		bool simu_eof;
		char simu_buffer[SIMU_BUFF_LEN];

		// Output probes and stats
		// Queue sizes
		map<unsigned int, Probe<int> *> probe_gw_queue_size;
		map<unsigned int, Probe<int> *> probe_gw_queue_size_kb;
		// Queue loss
		map<unsigned int, Probe<int> *> probe_gw_queue_loss;
		map<unsigned int, Probe<int> *> probe_gw_queue_loss_kb;
		// Rates
		map<unsigned int, Probe<int> *> probe_gw_l2_to_sat_before_sched;
		map<unsigned int, Probe<int> *> probe_gw_l2_to_sat_after_sched;
		Probe<int> *probe_gw_l2_to_sat_total;
		int l2_to_sat_total_bytes;
		// Frame interval
		Probe<float> *probe_frame_interval;
		// Physical layer information
		Probe<int> *probe_used_modcod;

		// Output logs and events
		OutputLog *log_request_simulation;

		/// logon response sent
		OutputEvent *event_logon_resp;
};

#endif

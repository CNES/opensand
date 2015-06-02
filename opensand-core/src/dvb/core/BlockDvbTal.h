/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file BlockDvbTal.h
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal,
 *        compatible with Legacy Dama agent.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef BLOCK_DVB_TAL_H
#define BLOCK_DVB_TAL_H

#include "BlockDvb.h"

#include "PhysicStd.h" 
#include "DamaAgent.h"
#include "SlottedAlohaTal.h"
#include "Scheduling.h"
#include "UnitConverter.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include <errno.h>
#include <netdb.h>        // for h_errno and hstrerror
#include <arpa/inet.h>    // for inet_ntoa
#include <sys/socket.h>


// Adjust timer to linux timer precision (10 ms):
// e.g., if a frame lasts 53 ms, but we wake up every 50 ms
// so as to consume all allocated bandwidth during a superframe
// TODO find a way to detect if the linux kernel contain the RT patch
//      to disable this macro consequently
//      see https://rt.wiki.kernel.org/index.php/RT_PREEMPT_HOWTO#Runtime_detection_of_an_RT-PREEMPT_Kernel
//      BUT do we really need that ? Do wee need to add it on the other timers ?
//      maybe not useful on new kernels
#define DVB_TIMER_ADJUST(x)  ((long)x/10)*10


/// the current state of the ST
typedef enum
{
	state_null,            /**< non-existant state */
	state_off,             /**< The ST is stopped */
	state_initializing,    /**< The ST is begin started */
	state_wait_logon_resp, /**< The ST is not logged yet */
	state_running,         /**< The ST is operational */
} tal_state_t;



/**
 * @class BlockDvbTal
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal,
 *        compatible with Legacy Dama agent.
 *
 *
 *      ^           |
 *      | encap     | encap packets with QoS
 *      | packets   v
 *    ------------------
 *   |                  |
 *   |  DVB-RCS Tal     |
 *   |  Dama Agent      |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame / Slotted Aloha packets
 *            v
 *
 */
class BlockDvbTal: public BlockDvb
{
 public:

	BlockDvbTal(const string &name, tal_id_t mac_id);
	~BlockDvbTal();


	class Upward: public DvbUpward
	{
	 public:
		Upward(Block *const bl, tal_id_t mac_id);
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
		 * @brief Initialize the output
		 *
		 * @return  true on success, false otherwise
		 */
		bool initOutput(void);

		/**
		 * Upon reception of a SoF:
		 * - update allocation with TBTP received last superframe (in DAMA agent)
		 * - reset timers
		 * @param dvb_frame  The DVB frame
		 * @return true on success, false otherwise
		 */
		bool onStartOfFrame(DvbFrame *dvb_frame);

		/**
		 * When receive a frame tick, send a constant DVB burst size for RT traffic,
		 * and a DVB burst for NRT allocated by the DAMA agent
		 * @return true on success, false if failed
		 */
		bool processOnFrameTick(void);

		/**
		 * Manage the receipt of the DVB Frames
		 *
		 * @param dvb_frame  The DVB frame
		 * @return true on success, false otherwise
		 */
		bool onRcvDvbFrame(DvbFrame *dvb_frame);

		/**
		 * Manage logon response: inform opposite channel and upper layer
		 * that the link is now up and running
		 *
		 * @param dvb_frame  The frame containing the logon response
		 * @return true on success, false otherwise
		 */
		bool onRcvLogonResp(DvbFrame *dvb_frame);

		/**
		 * Transmist a frame to the opposite channel
		 *
		 * @param frame  The dvb frame
		 * @return true on success, false otherwise
		 */
		bool shareFrame(DvbFrame *frame);

		// statistics update
		void updateStats(void);

		/// reception standard (DVB-RCS or DVB-S2)      
		PhysicStd *reception_std; 

		/// the MAC ID of the ST (as specified in configuration)
		int mac_id;
		/// the group ID sent by NCC (only valid in state_running)
		group_id_t group_id;
		/// the logon ID sent by NCC (only valid in state_running,
		/// should be the same as the mac_id)
		tal_id_t tal_id;
		spot_id_t spot_id;

		/// the current state of the ST
		tal_state_t state;

		/* Output probes and stats */
			// Rates
					// Layer 2 from SAT
		Probe<int> *probe_st_l2_from_sat;
		int l2_from_sat_bytes;
			// Physical layer information
		Probe<int> *probe_st_real_modcod;
		Probe<int> *probe_st_received_modcod;
		Probe<int> *probe_st_rejected_modcod;
			// Stability
		Probe<float> *probe_sof_interval;
	};


	class Downward: public DvbDownward
	{
	 public:
		Downward(Block *const bl, tal_id_t mac_id);
		~Downward();
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
		 * Read configuration for the carrier ID
		 *
		 * @return  true on success, false otherwise
		 */
		bool initCarrierId(void);

		/**
		 * Read configuration for the MAC FIFOs
		 *
		 * @return  true on success, false otherwise
		 */
		bool initMacFifo(void);

		/**
		 * Read configuration for the DAMA algorithm
		 *
		 * @return  true on success, false otherwise
		 */
		bool initDama(void);

		/**
		 * Read configuration for the Slotted Aloha algorithm
		 *
		 * @return  true on success, false otherwise
		 */
		bool initSlottedAloha(void);
		
		/**
		 * Read configuration for the SCPC algorithm
		 *
		 * @return  true on success, false otherwise
		 */
		bool initScpc(void);


		/**
		 * @brief Initialize the output
		 *
		 * @return  true on success, false otherwise
		 */
		bool initOutput(void);

		/**
		 * Read configuration for the QoS Server
		 *
		 * @return  true on success, false otherwise
		 */
		bool initQoSServer(void);

		/**
		 * @brief Initialize the timers
		 *
		 *
		 * @return  true on success, false otherwise
		 */
		bool initTimers(void);

		// statistics update
		void updateStats(void);

		/**
		 * This method send a Logon Req message
		 *
		 * @return true on success, false otherwise
		 */
		bool sendLogonReq(void);

		/**
		 * Manage logon response: inform dama that the link is now up and running
		 *
		 * @param frame  The frame containing the logon response
		 * @return true on success, false otherwise
		 */
		bool handleLogonResp(DvbFrame *frame);

		/**
		 * Upon reception of a SoF:
		 * - update allocation with TBTP received last superframe (in DAMA agent)
		 * - reset timers
		 * @param dvb_frame  The DVB frame
		 * @return true on success, false otherwise
		 */
		bool handleStartOfFrame(DvbFrame *dvb_frame);

		/**
		 * When receive a frame tick, send a constant DVB burst size for RT traffic,
		 * and a DVB burst for NRT allocated by the DAMA agent
		 * @return true on success, false if failed
		 */
		bool processOnFrameTick(void);

		/**
		 * Manage the receipt of the DVB Frames
		 *
		 * @param dvb_frame  The DVB frame
		 * @return true on success, false otherwise
		 */
		bool handleDvbFrame(DvbFrame *dvb_frame);

		/**
		 * Send a SAC
		 * @return true on success, false otherwise
		 */
		bool sendSAC(void);

		// communication with QoS Server:
		bool connectToQoSServer();
		// TODO REMOVE STATIC
		static void closeQosSocket(int sig);

		/**
		 * Delete packets in dvb_fifo
		 */
		void deletePackets(void);

		/// the MAC ID of the ST (as specified in configuration)
		int mac_id;

		/// the current state of the ST
		tal_state_t state;

		/// the group ID sent by NCC (only valid in state_running)
		group_id_t group_id;
		/// the logon ID sent by NCC (only valid in state_running,
		/// should be the same as the mac_id)
		tal_id_t tal_id;
		spot_id_t spot_id;

		/// fixed bandwidth (CRA) in kbits/s
		rate_kbps_t cra_kbps;
		/// Maximum RBDC in kbits/s
		rate_kbps_t max_rbdc_kbps;
		/// Maximum VBDC in kbits
		vol_kb_t max_vbdc_kb;

		/// the DAMA agent
		DamaAgent *dama_agent;

		/// The Slotted Aloha for terminal
		SlottedAlohaTal *saloha;
		
		/// SCPC Carrier duration in ms
		time_ms_t scpc_carr_duration_ms;
	
		/// frame timer for scpc, used to awake the block every frame period	
		event_id_t scpc_timer;
		
		/// FMT groups for up/return
		fmt_groups_t ret_fmt_groups;
		
		// The MODCOD simulation elements for down/forward link
		FmtSimulation scpc_fmt_simu;
		
		/// The uplink of forward scheduling depending on satellite	
		Scheduling *scpc_sched;
		
		/// counter for SCPC frames
		time_sf_t scpc_frame_counter;
			
		/* carrier IDs */
		uint8_t carrier_id_ctrl;  ///< carrier id for DVB control frames emission
		uint8_t carrier_id_logon; ///< carrier id for Logon req  emission
		uint8_t carrier_id_data;  ///< carrier id for traffic emission

		/* Fifos */
		/// map of FIFOs per MAX priority to manage different queues
		fifos_t dvb_fifos;
		/// the default MAC fifo index = fifo with the smallest priority
		unsigned int default_fifo_id;
		
		/* OBR */
		/// OBR period -in number of frames- and Obr slot
		/// position within the multi-frame,
		time_frame_t sync_period_frame;
		time_frame_t obr_slot_frame;

		/// the list of complete DVB-RCS/BB frames that were not sent yet
		std::list<DvbFrame *> complete_dvb_frames;

		/// Upon each logon timer event retry logon
		event_id_t logon_timer;

		/// The C/N0 for downlink in scenario
		double cni;

		/* QoS Server / Policy Enforcement Point (PEP) on ST side */
		static int qos_server_sock;    ///< The socket for the QoS Server
		std::string qos_server_host;   ///< The hostname of the QoS Server
		int qos_server_port;           ///< The TCP port of the QoS Server
		event_id_t qos_server_timer;   ///< The timer for connection retry to QoS Server

		// Output events
		OutputEvent *event_login;

		// Output Logs
		OutputLog *log_frame_tick;
		OutputLog *log_qos_server;
		/// log for slotted aloha
		OutputLog *log_saloha;

		/* Output probes and stats */
			// Queue sizes
		map<unsigned int, Probe<int> *> probe_st_queue_size;
		map<unsigned int, Probe<int> *> probe_st_queue_size_kb;
			// Queue loss
		map<unsigned int, Probe<int> *> probe_st_queue_loss;
		map<unsigned int, Probe<int> *> probe_st_queue_loss_kb;
			// Rates
				// Layer 2 to SAT
		map<unsigned int, Probe<int> *> probe_st_l2_to_sat_before_sched;
		map<unsigned int, int> l2_to_sat_cells_before_sched;
		map<unsigned int, Probe<int> *> probe_st_l2_to_sat_after_sched;
		int l2_to_sat_total_bytes;
		Probe<int> *probe_st_l2_to_sat_total;
				// PHY to SAT
		Probe<int> *probe_st_phy_to_sat;
				// Layer 2 from SAT
	};

 protected:

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit(void);	

};

#endif


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 */

#ifndef BLOCK_DVB_TAL_H
#define BLOCK_DVB_TAL_H

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>

#include "BlockDvb.h"

#include "DvbChannel.h"
#include "PhysicStd.h"
#include "DamaAgent.h"
#include "SlottedAlohaTal.h"
#include "Scheduling.h"
#include "UnitConverter.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Block.h>

#include <errno.h>
#include <netdb.h>        // for h_errno and hstrerror
#include <arpa/inet.h>    // for inet_ntoa
#include <sys/socket.h>


/// the current state of the ST
enum class TalState
{
	null,            /**< non-existant state */
	off,             /**< The ST is stopped */
	initializing,    /**< The ST is begin started */
	wait_logon_resp, /**< The ST is not logged yet */
	running,         /**< The ST is operational */
};


template<>
class Rt::UpwardChannel<class BlockDvbTal>: public DvbChannel, public Channels::Upward<UpwardChannel<BlockDvbTal>>, public DvbFmt
{
 public:
	UpwardChannel(const std::string &name, dvb_specific specific);
	~UpwardChannel();

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const MessageEvent& event) override;

 protected:
	/**
	 * @brief Initialize the transmission mode
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMode();

	/**
	 * @brief Read configuration for the different files and open them
	 *
	 * @return  true on success, false otherwise
	 */
	bool initModcodSimu();

	/**
	 * @brief Initialize the output
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput();

	/**
	 * Upon reception of a SoF:
	 * - update allocation with TBTP received last superframe (in DAMA agent)
	 * - reset timers
	 * @param dvb_frame  The DVB frame
	 * @return true on success, false otherwise
	 */
	bool onStartOfFrame(DvbFrame &dvb_frame);

	/**
	 * When receive a frame tick, send a constant DVB burst size for RT traffic,
	 * and a DVB burst for NRT allocated by the DAMA agent
	 * @return true on success, false if failed
	 */
	bool processOnFrameTick();

	/**
	 * Manage the receipt of the DVB Frames
	 *
	 * @param dvb_frame  The DVB frame
	 * @return true on success, false otherwise
	 */
	bool onRcvDvbFrame(Ptr<DvbFrame> dvb_frame);

	/**
	 * Manage logon response: inform opposite channel and upper layer
	 * that the link is now up and running
	 *
	 * @param dvb_frame  The frame containing the logon response
	 * @return true on success, false otherwise
	 */
	bool onRcvLogonResp(Ptr<DvbFrame> dvb_frame);

	/**
	 * Transmist a frame to the opposite channel
	 *
	 * @param frame The dvb frame
	 * @return true on success, false otherwise
	 */ 
	bool shareFrame(Ptr<DvbFrame> frame);

	// statistics update
	void updateStats();

	/// reception standard (DVB-RCS or DVB-S2)
	PhysicStd *reception_std; 

	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;
	/// the group ID sent by NCC (only valid in state_running)
	group_id_t group_id;
	/// the logon ID sent by NCC (only valid in state_running,
	/// should be the same as the mac_id)
	tal_id_t tal_id;
	tal_id_t gw_id;
	// is the terminal scpc
	bool is_scpc;

	/// the current state of the ST
	TalState state;

	/* Output probes and stats */
	// Rates
	// Layer 2 from SAT
	std::shared_ptr<Probe<int>> probe_st_l2_from_sat;
	int l2_from_sat_bytes;
	// Physical layer information
	std::shared_ptr<Probe<int>> probe_st_received_modcod; // MODCOD of DVB burst received
	std::shared_ptr<Probe<int>> probe_st_rejected_modcod; // MODCOD of DVB burst rejected
	// Stability
	std::shared_ptr<Probe<float>> probe_sof_interval;

	bool disable_control_plane;
	bool disable_acm_loop;
};


template<>
class Rt::DownwardChannel<class BlockDvbTal>: public DvbChannel, public Channels::Downward<DownwardChannel<BlockDvbTal>>, public DvbFmt
{
 public:
	DownwardChannel(const std::string &name, dvb_specific specific);
	~DownwardChannel();

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const TimerEvent& event) override;
	bool onEvent(const MessageEvent& event) override;

protected:
	/**
	 * @brief Read the common configuration parameters for downward channels
	 *
	 * @return true on success, false otherwise
	 */
	bool initDown();

	/**
	 * @brief Initialize the transmission mode
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMode();

	/**
	 * Read configuration for the carrier ID
	 *
	 * @return  true on success, false otherwise
	 */
	bool initCarrierId();

	/**
	 * Read configuration for the MAC FIFOs
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMacFifo();

	/**
	 * Read configuration for the DAMA algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	bool initDama();

	/**
	 * Read configuration for the Slotted Aloha algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	bool initSlottedAloha();

	/**
	 * Read configuration for the SCPC algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	bool initScpc();

	/**
	 * @brief Initialize the output
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput();

	/**
	 * Read configuration for the QoS Server
	 *
	 * @return  true on success, false otherwise
	 */
	bool initQoSServer();

	/**
	 * @brief Initialize the timers
	 *
	 *
	 * @return  true on success, false otherwise
	 */
	bool initTimers();

	// statistics update
	void updateStats();

	/**
	 * This method send a Logon Req message
	 *
	 * @return true on success, false otherwise
	 */
	bool sendLogonReq();

	/**
	 * Manage logon response: inform dama that the link is now up and running
	 *
	 * @param frame  The frame containing the logon response
	 * @return true on success, false otherwise
	 */
	bool handleLogonResp(Ptr<DvbFrame> frame);

	/**
	 * Send the complete DVB frames created
	 * by \ref DvbRcsStd::scheduleEncapPackets or
	 * \ref DvbRcsDamaAgent::globalSchedule for Terminal
	 *
	 * @param carrier_id      the ID of the carrier where to send the frames
	 * @return true on success, false otherwise
	 */
	bool sendBursts(uint8_t carrier_id);

	/**
	 * @brief Send message to lower layer with the given DVB frame
	 *
	 * @param frame       the DVB frame to put in the message
	 * @param carrier_id  the carrier ID used to send the message
	 * @return            true on success, false otherwise
	 */
	bool sendDvbFrame(Ptr<DvbFrame> frame, uint8_t carrier_id);

	/**
	 * Upon reception of a SoF:
	 * - update allocation with TBTP received last superframe (in DAMA agent)
	 * - reset timers
	 * @param dvb_frame  The DVB frame
	 * @return true on success, false otherwise
	 */
	bool handleStartOfFrame(Ptr<DvbFrame> dvb_frame);

	/**
	 * When receive a frame tick, send a constant DVB burst size for RT traffic,
	 * and a DVB burst for NRT allocated by the DAMA agent
	 * @return true on success, false if failed
	 */
	bool processOnFrameTick();

	/**
	 * Manage the receipt of the DVB Frames
	 *
	 * @param dvb_frame  The DVB frame
	 * @return true on success, false otherwise
	 */
	bool handleDvbFrame(Ptr<DvbFrame> dvb_frame);

	/**
	 * Send a SAC
	 * @return true on success, false otherwise
	 */
	bool sendSAC();

	// communication with QoS Server:
	bool connectToQoSServer();
	// TODO REMOVE STATIC
	static void closeQosSocket(int sig);

	/**
	 * add Cni extension into Gse packet
	 * @return true on success, false otherwise
	 */ 
	bool addCniExt();

	/**
	 * Delete packets in dvb_fifo
	 */
	void deletePackets();

	/// reception standard (DVB-RCS or DVB-S2)
	PhysicStd *reception_std; 

	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;

	/// the current state of the ST
	TalState state;

	bool disable_control_plane;

	/// the group ID sent by NCC (only valid in state_running)
	group_id_t group_id;
	/// the logon ID sent by NCC (only valid in state_running,
	/// should be the same as the mac_id)
	tal_id_t tal_id;
	tal_id_t gw_id;
	// is the terminal scpc
	bool is_scpc;

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
	std::list<Ptr<DvbFrame>> complete_dvb_frames;

	/// Upon each logon timer event retry logon
	event_id_t logon_timer;

	/* QoS Server / Policy Enforcement Point (PEP) on ST side */
	static int qos_server_sock;    ///< The socket for the QoS Server
	std::string qos_server_host;   ///< The hostname of the QoS Server
	int qos_server_port;           ///< The TCP port of the QoS Server
	event_id_t qos_server_timer;   ///< The timer for connection retry to QoS Server

	// Output events
	std::shared_ptr<OutputEvent> event_login;

	// Output Logs
	std::shared_ptr<OutputLog> log_frame_tick;
	std::shared_ptr<OutputLog> log_qos_server;
	/// log for slotted aloha
	std::shared_ptr<OutputLog> log_saloha;

	/* Output probes and stats */
	// Queue sizes
	std::map<unsigned int, std::shared_ptr<Probe<int> > > probe_st_queue_size;
	std::map<unsigned int, std::shared_ptr<Probe<int> > > probe_st_queue_size_kb;
	// Queue loss
	std::map<unsigned int, std::shared_ptr<Probe<int> > > probe_st_queue_loss;
	std::map<unsigned int, std::shared_ptr<Probe<int> > > probe_st_queue_loss_kb;
	// Rates
	// Layer 2 to SAT
	std::map<unsigned int, std::shared_ptr<Probe<int> > > probe_st_l2_to_sat_before_sched;
	std::map<unsigned int, int> l2_to_sat_cells_before_sched;
	std::map<unsigned int, std::shared_ptr<Probe<int> > > probe_st_l2_to_sat_after_sched;
	int l2_to_sat_total_bytes;
	std::shared_ptr<Probe<int>> probe_st_l2_to_sat_total;
	// PHY to SAT
	std::shared_ptr<Probe<int>> probe_st_phy_to_sat;
	// Layer 2 from SAT

	// Physical layer information
	std::shared_ptr<Probe<int>> probe_st_required_modcod; // MODCOD required for next DVB bursts. Correspond to the value put in SAC
};


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
class BlockDvbTal: public Rt::Block<BlockDvbTal, dvb_specific>, public BlockDvb
{
 public:
	BlockDvbTal(const std::string &name, dvb_specific specific);
	~BlockDvbTal();

	static void generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane);

	bool initListsSts();

 protected:
	bool onInit() override;

	bool disable_control_plane;

	/// The list of Sts with return/up modcod
	StFmtSimuList* input_sts;
	StFmtSimuList* output_sts;
};


#endif

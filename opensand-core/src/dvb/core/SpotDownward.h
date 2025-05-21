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
 * @file SpotDownward.h
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SPOT_DOWNWARD_H
#define SPOT_DOWNWARD_H

#include <list>
#include <opensand_rt/Types.h>

#include "OpenSandCore.h"
#include "DvbChannel.h"
#include "DvbFifoTypes.h"
#include "RequestSimulator.h"
#include "TerminalCategoryDama.h"


class NetPacket;
class LogonRequest;
class PepRequest;
class SvnoRequest;
class Ttp;
class DamaCtrlRcs2;
class Scheduling;


class SpotDownward: public DvbChannel, public DvbFmt
{
public:
	SpotDownward(spot_id_t spot_id,
	             tal_id_t mac_id,
	             time_us_t fwd_down_frame_duration,
	             time_us_t ret_up_frame_duration,
	             time_ms_t stats_period,
	             StackPlugin *upper_encap,
	             std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl,
	             std::shared_ptr<StFmtSimuList> input_sts,
	             std::shared_ptr<StFmtSimuList> output_sts);

	~SpotDownward();

	static void generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane);

	/**
	 * @brief Spot Downward initialisation
	 *
	 * @return true on success, false otherwise
	 */ 
	bool onInit();

	/**
	 * @brief Handle the Slotted Aloha ACKs
	 *
	 * @param ack_frames  The Slotted Aloha ACKs
	 * @return true on success, false otherwise
	 */
	bool handleSalohaAcks(Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> ack_frames);

	/**
	 * @brief Handle an encapsulated packet
	 *
	 * @param packet  The encapsulated packet
	 * @return true on success, false otherwise
	 */
	bool handleEncapPacket(Rt::Ptr<NetPacket> packet);

	/**
	 * @brief Handle a logon request transmitted by the opposite
	 *        block
	 *
	 * @param logon_req  The frame contining the logon request
	 * @return true on success, false otherwise
	 */
	bool handleLogonReq(Rt::Ptr<LogonRequest> logon_req);

	/**
	 * @brief Handle a logoff request transmitted by the opposite
	 *        block
	 *
	 * @param dvb_frame  The frame contining the logoff request
	 * @return true on success, false otherwise
	 */
	bool handleLogoffReq(Rt::Ptr<DvbFrame> dvb_frame);

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
	bool handleSac(Rt::Ptr<DvbFrame> dvb_frame);

	/**
	 * @brief update FMT in DAMA controller
	 */
	void updateFmt();

	/**
	 * @briel apply PEP command
	 * @param pep_request the pep request
	 * @return true on success, false otherwise
	 */
	bool applyPepCommand(std::unique_ptr<PepRequest> pep_request);

	/**
	 * @briel apply SVNO command
	 * @param svno_request the SVNO request
	 * @return true on success, false otherwise
	 */
	bool applySvnoCommand(std::unique_ptr<SvnoRequest> svno_request);

	/**
	 * @brief Build a TTP
	 *
	 * @param ttp  OUT: The TTP
	 * @return true on success, false otherwise
	 */
	bool buildTtp(Ttp& ttp);

	uint8_t getCtrlCarrierId() const;
	uint8_t getSofCarrierId() const;
	uint8_t getDataCarrierId() const;

	std::list<Rt::Ptr<DvbFrame>> &getCompleteDvbFrames();

	void setPepCmdApplyTimer(Rt::event_id_t pep_cmd_a_timer);
	Rt::event_id_t getPepCmdApplyTimer();

protected:
	/**
	 * Read configuration for the downward timers
	 *
	 * @return  true on success, false otherwise
	 */
	bool initTimers();

	/**
	 * Read configuration for the carrier IDs
	 *
	 * @return  true on success, false otherwise
	 */
	bool initCarrierIds();

	/**
	 * @brief Initialize the transmission mode
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMode();

	/**
	 * Read configuration for the DAMA algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	bool initDama();

	/**
	 * @brief Read configuration for the FIFOs
	 *
	 * @param  The FIFOs to initialize
	 * @return  true on success, false otherwise
	 */
	bool initFifo(std::shared_ptr<fifos_t> fifos);

	/**
	 * @brief Initialize the statistics
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput();

	/** Read configuration for the request simulation
	 *
	 * @return  true on success, false otherwise
	 */
	bool initRequestSimulation();

	/**
	 * Simulate event based on an input file
	 * @return true on success, false otherwise
	 */
	bool simulateFile();

	/**
	 * Simulate event based on random generation
	 * @return true on success, false otherwise
	 */
	bool simulateRandom();

	// statistics update
	void updateStatistics();
	
	/**
	 * @brief add Cni extension into GSE packet (for SCPC)
	 *
	 * @return true on success, false otherwise
	 */ 
	bool addCniExt();

	/// The DAMA controller
	std::unique_ptr<DamaCtrlRcs2> dama_ctrl;

	/// The uplink or forward scheduling per category
	std::map<std::string, std::unique_ptr<Scheduling>> scheduling;

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

	/// is terminal scpc map
	std::list<tal_id_t> is_tal_scpc;

	/* Fifos */
	/// FIFOs per MAX priority to manage different queues for each category
	std::map<std::string, std::shared_ptr<fifos_t>> dvb_fifos;
	/// the default MAC fifo index = fifo with the smallest priority
	unsigned int default_fifo_id;

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	std::list<Rt::Ptr<DvbFrame>> complete_dvb_frames;

	/// The terminal categories for forward band
	TerminalCategories<TerminalCategoryDama> categories;

	/// The terminal affectation for forward band
	TerminalMapping<TerminalCategoryDama> terminal_affectation;

	/// The default terminal category for forward band
	std::shared_ptr<TerminalCategoryDama> default_category;

	/// FMT groups for down/forward
	fmt_groups_t fwd_fmt_groups;

	/// FMT groups for up/return
	fmt_groups_t ret_fmt_groups;
	

	/// The C/N0 for downlink in regenerative scenario that will be transmited
	//  to satellite in SAC
	//  For transparent scenario the return link cni will be used to update return
	//  MODCOD id for terminals (not this one)
	double cni;

	/// timer used for applying resources allocations received from PEP
	Rt::event_id_t pep_cmd_apply_timer;

	std::unique_ptr<RequestSimulator> request_simu;

	/// parameters for request simulation
	std::ostream* event_file;
	Simulate simulate;

	// Output probes and stats
	using ProbeListPerId = std::map<unsigned int, std::shared_ptr<Probe<int>>>;
	// Queue sizes
	std::map<std::string, ProbeListPerId> probe_gw_queue_size;
	std::map<std::string, ProbeListPerId> probe_gw_queue_size_kb;
	// Queue loss
	std::map<std::string, ProbeListPerId> probe_gw_queue_loss;
	std::map<std::string, ProbeListPerId> probe_gw_queue_loss_kb;
	// Rates
	std::map<std::string, ProbeListPerId> probe_gw_l2_to_sat_before_sched;
	std::map<std::string, ProbeListPerId> probe_gw_l2_to_sat_after_sched;
	std::map<std::string, std::shared_ptr<Probe<int>>> probe_gw_l2_to_sat_total;
	std::map<std::string, int> l2_to_sat_total_bytes;
	// Frame interval
	std::shared_ptr<Probe<float>> probe_frame_interval;
	// Physical layer information
	std::shared_ptr<Probe<int>> probe_sent_modcod;

	// Output logs and events
	std::shared_ptr<OutputLog> log_request_simulation;

	/// logon response sent
	std::shared_ptr<OutputEvent> event_logon_resp;
};

#endif

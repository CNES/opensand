/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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

#include "DvbChannel.h"
#include "DamaCtrlRcsCommon.h"
#include "Scheduling.h"
#include "SlottedAlohaNcc.h"
#include "RequestSimulator.h"
#include "SvnoRequest.h"


class SpotDownward: public DvbChannel, public DvbFmt
{
 public:
	SpotDownward(spot_id_t spot_id,
	             tal_id_t mac_id,
	             time_ms_t fwd_down_frame_duration,
	             time_ms_t ret_up_frame_duration,
	             time_ms_t stats_period,
	             sat_type_t sat_type,
	             EncapPlugin::EncapPacketHandler *pkt_hdl,
	             StFmtSimuList *input_sts,
	             StFmtSimuList *output_sts);

	virtual ~SpotDownward();
	
	/**
	 * @brief Spot Downward initialisation
	 *
	 * @return true on success, false otherwise
	 */ 
	virtual bool onInit(void);

	/**
	 * @brief Handle the Slotted Aloha ACKs
	 *
	 * @param ack_frames  The Slotted Aloha ACKs
	 * @return true on success, false otherwise
	 */
	virtual bool handleSalohaAcks(const list<DvbFrame *> *ack_frames);

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
	virtual bool handleFwdFrameTimer(time_sf_t fwd_frame_counter);

	/**
	 * @brief  handle a SAC frame
	 *
	 * @param dvb_frame The SAC frame
	 * @return true on success, false otherwise
	 */
	bool handleSac(const DvbFrame *dvb_frame);

	/**
	 * @brief go to fmt simu next scenario
	 *
	 * @param next_step  time of the next step
	 *
	 * @return true on success, false otherwise
	 */
	bool goNextScenarioStep(double &next_step);

	/**
	 * @brief update FMT in DAMA controller
	 */
	void updateFmt(void);
	
	/**
	 * @briel apply PEP command
	 * @param pep_request the pep request
	 * @return true on success, false otherwise
	 */
	bool applyPepCommand(PepRequest *pep_request);

	/**
	 * @briel apply SVNO command
	 * @param svno_request the SVNO request
	 * @return true on success, false otherwise
	 */
	bool applySvnoCommand(SvnoRequest *svno_request);

	/**
	 * @brief Build a TTP
	 *
	 * @param ttp  OUT: The TTP
	 * @return true on success, false otherwise
	 */
	bool buildTtp(Ttp *ttp);

	uint8_t getCtrlCarrierId(void) const;
	uint8_t getSofCarrierId(void) const;
	uint8_t getDataCarrierId(void) const;

	list<DvbFrame *> &getCompleteDvbFrames(void);

	void setPepCmdApplyTimer(event_id_t pep_cmd_a_timer);
	event_id_t getPepCmdApplyTimer(void);

	void setAcmTimer(event_id_t new_acm_timer);
	event_id_t getAcmTimer(void);

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
	virtual bool initMode(void) = 0;

	/**
	 * Read configuration for the DAMA algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	virtual bool initDama(void) = 0;

	/**
	 * @brief Read configuration for the FIFOs
	 *
	 * @param  The FIFOs to initialize
	 * @return  true on success, false otherwise
	 */
	bool initFifo(fifos_t &fifos);

	/**
	 * @brief Initialize the statistics
	 *
	 * @return  true on success, false otherwise
	 */
	virtual bool initOutput(void);

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
	
	/**
	 * @brief add Cni extension into GSE packet (for SCPC)
	 *
	 * @return true on success, false otherwise
	 */ 
	virtual bool addCniExt(void) = 0;

	/// The DAMA controller
	DamaCtrlRcsCommon *dama_ctrl;

	/// The uplink or forward scheduling per category
	map<string, Scheduling*> scheduling;

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
	list<tal_id_t> is_tal_scpc;

	/* Fifos */
	/// FIFOs per MAX priority to manage different queues for each category
	map<string, fifos_t> dvb_fifos;
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
	

	/// The C/N0 for downlink in regenerative scenario that will be transmited
	//  to satellite in SAC
	//  For transparent scenario the return link cni will be used to update return
	//  MODCOD id for terminals (not this one)
	double cni;

	/// timer used for applying resources allocations received from PEP
	event_id_t pep_cmd_apply_timer;

	/// timer used to send acm parameter (only for Regenerative)
	event_id_t acm_timer;

	RequestSimulator *request_simu;

	/// parameters for request simulation
	FILE *event_file;
	Simulate simulate;

	// Output probes and stats
	typedef map<unsigned int, Probe<int> *> ProbeListPerId; 
	// Queue sizes
	map<string, ProbeListPerId> * probe_gw_queue_size;
	map<string, ProbeListPerId> *probe_gw_queue_size_kb;
	// Queue loss
	map<string, ProbeListPerId> *probe_gw_queue_loss;
	map<string, ProbeListPerId> *probe_gw_queue_loss_kb;
	// Rates
	map<string, ProbeListPerId> *probe_gw_l2_to_sat_before_sched;
	map<string, ProbeListPerId> *probe_gw_l2_to_sat_after_sched;
	map<string, Probe<int> *> probe_gw_l2_to_sat_total;
	map<string, int> l2_to_sat_total_bytes;
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

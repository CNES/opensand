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


class BlockDvbNcc: public BlockDvb, NccPepInterface
{

 private:

	/// The DAMA controller
	DamaCtrlRcs *dama_ctrl;

	/// carrier ids
	long m_carrierIdDvbCtrl;
	long m_carrierIdSOF;
	long m_carrierIdData;

	/// the current super frame number
	long super_frame_counter;
	/// the current frame number inside the current super frame
	unsigned int frame_counter;

	/// frame timer, used to awake the block every frame period
	event_id_t frame_timer;

	/// ST unique mac id (configuration param)
	int macId;

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	std::list<DvbFrame *> complete_dvb_frames;

	/// The TTP
	Ttp ttp;

	/// The capacity requests
	CapacityRequest capacity_request;

	/// timer used to awake the block every second in order to retrieve
	/// the current MODCODs
	event_id_t scenario_timer;

	/// a fifo to keep the received packet from encap bloc
	DvbFifo data_dvb_fifo;

	/// FMT groups
	fmt_groups_t fmt_groups;

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
	long simu_cr;
	long simu_interval;
	event_id_t simu_timer;

	//events

	/// logon request reveived
	Event *event_logon_req;
	/// logon response sent
	Event *event_logon_resp;

	map<uint16_t, uint16_t> column_list;

 public:

	/// Class constructor
	/// Use mgl_bloc default constructor
	BlockDvbNcc(const string &name);

	~BlockDvbNcc();

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();

	/// a map of bbframes to manage different bbframes
	std::map<int, T_DVB_BBFRAME *> *m_bbframe;
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

	bool getBBFRAMEDuration(unsigned int modcod_id, float *duration);

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

	// throughput from upper layer and associated probe
	unsigned int incoming_size;
	Probe<float> *probe_incoming_throughput;

	// statistics update
	void updateStatsOnFrame();
};

#endif

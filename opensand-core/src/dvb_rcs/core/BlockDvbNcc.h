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
#include "SpotUpward.h"
#include "SpotDownward.h"
#include "DamaCtrlRcs.h"

class BlockDvbNcc: public BlockDvb
{
 public:

	/// Class constructor
	BlockDvbNcc(const string &name);

	~BlockDvbNcc();

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();


	class Upward: public DvbUpward
	{
	 public:
		/// upward block table (1 per slot) 
		map<spot_id_t, SpotUpward *> spot_upward_map;

		Upward(Block *const bl);
		~Upward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 protected:
		/**
		 * Transmist a frame to the opposite channel
		 *
		 * @param frame The dvb frame
		 * @return true on success, false otherwise
		 */ 
		bool shareFrame(DvbFrame *frame);

		// log for slotted aloha
		OutputLog *log_saloha;

	};


	class Downward: public DvbDownward, NccPepInterface
	{
	  public:
		/// downward block table (1 per slot)
		map<spot_id_t, SpotDownward *> spot_downward_map;
		
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
		
		/** Read configuration for the resquest simulation
		 * 
		 * @return true on success, false otherwise
		 */ 
		bool initRequestSimulation(void);

		/**
		 * Read configuration for simulated FMT columns ID
		 *
		 * @return  true on success, false otherwise
		 */
		bool initColumns(void);

		/**
		 * @brief Read configuration for the different files and open them
		 *
		 * @return  true on success, false otherwise
		 */
		bool initModcodSimu(void);
		
		/**
		 * Send a Terminal Time Plan
		 */
		void sendTTP(SpotDownward *spot_downward);

		/**
		 * Send a start of frame
		 */
		void sendSOF(SpotDownward *spot_downward);

		/**
		 * @brief Send a SAC message containing ACM parameters
		 *
		 * @return true on success, false otherwise
		 */
		bool sendAcmParameters(SpotDownward *spot_downward);
		
		// statistics update
		void updateStats(void);
		void resetStatsCxt(void);


		/**
		 * Simulate event based on an input file
		 * @return true on success, false otherwise
		 */
		bool simulateFile(void);

		/**
		 * Simulate event based on random generation
		 */
		void simulateRandom(void);

		/// frame timer for return, used to awake the block every frame period
		event_id_t frame_timer;

		/// frame timer for forward, used to awake the block every frame period
		event_id_t fwd_timer;

		/// The MODCOD simulation elements for up/return link
		FmtSimulation up_ret_fmt_simu;
		/// The MODCOD simulation elements for down/forward link
		FmtSimulation down_fwd_fmt_simu;

		/// timer used to awake the block every second in order to retrieve
		/// the current MODCODs
		/// In regenerative case with physical layer, is it used to send
		// ACM parameters to satellite
		event_id_t scenario_timer;

		// The column ID for FMT simulation
		map<tal_id_t, uint16_t> column_list;

		// Frame interval
		Probe<float> *probe_frame_interval;
	};
};

#endif

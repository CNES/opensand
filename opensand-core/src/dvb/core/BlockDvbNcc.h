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
 * @file BLockDvbNcc.h
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 *
 * <pre>
 *
 *
 *        |    encap   ^
 *        |    burst   |
 *        v            |
 *   +-----------------------+
 *   | downward  |   upward  |
 *   |           |           |
 *   | +-------+ | +-------+ |
 *   | | spots | | | spots | |
 *   | +-------+ | +-------+ |
 *    -----------+-----------+
 *        |            ^
 *        | DVB Frame  |
 *        v  BBFrame   |
 *
 * For spots description
 * @ref SpotDownward and @ref SpotUpward
 *
 * </pre>
 *
 */

#ifndef BLOCK_DVB_NCC_H
#define BLOCk_DVB_NCC_H

#include "BlockDvb.h"

#include "NccPepInterface.h"
#include "NccSvnoInterface.h"
#include "DvbChannel.h"


class SpotDownward;
class SpotUpward;


class BlockDvbNcc: public BlockDvb
{
 public:
	/// Class constructor
	BlockDvbNcc(const string &name, struct dvb_specific specific);

	~BlockDvbNcc();

	static void generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane);

	bool onInit();


	class Upward: public DvbUpward, public DvbFmt
	{
	 public:
		Upward(const string &name, struct dvb_specific specific);
		~Upward();
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 protected:
		/**
		 * @brief Initialize the output
		 *
		 * @return  true on success, false otherwise
		 */
		bool initOutput(void);
		
		bool onRcvDvbFrame(DvbFrame *frame);

		/// the MAC ID of the ST (as specified in configuration)
		int mac_id;

		SpotUpward* spot;

		// log for slotted aloha
		std::shared_ptr<OutputLog> log_saloha;

		// Physical layer information
		std::shared_ptr<Probe<int>> probe_gw_received_modcod; // MODCOD of BBFrame received
		std::shared_ptr<Probe<int>> probe_gw_rejected_modcod; // MODCOD of BBFrame rejected
	};


	class Downward: public DvbDownward, public DvbFmt
	{
		public:
			Downward(const string &name, struct dvb_specific specific);
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

			bool handleDvbFrame(DvbFrame *frame);

			/**
			 * Send a Terminal Time Plan
			 */
			void sendTTP(SpotDownward *spot_downward);

			/**
			 * Send a start of frame
			 */
			void sendSOF(unsigned int sof_carrier_id);

			/**
			 *  @brief Handle a logon request transmitted by the opposite
			 *         block
			 *
			 *  @param dvb_frame  The frame contining the logon request
			 *  @param spot       The spot concerned by the request
			 *  @return true on success, false otherwise
			 */
			bool handleLogonReq(DvbFrame *dvb_frame, SpotDownward *spot);

			/**
			 * @brief Send a SAC message containing ACM parameters
			 *
			 * @return true on success, false otherwise
			 */
			bool sendAcmParameters(SpotDownward *spot_downward);

			// statistics update
			void updateStats(void);

			/// The interface between Ncc and PEP
			NccPepInterface pep_interface;

			/// The interface between Ncc and SVNO
			NccSvnoInterface svno_interface;

			/// the MAC ID of the ST (as specified in configuration)
			tal_id_t mac_id;
			bool disable_control_plane;

			/// counter for forward frames
			time_ms_t fwd_frame_counter;

			/// frame timer for return, used to awake the block every frame period
			event_id_t frame_timer;

			/// frame timer for forward, used to awake the block every frame period
			event_id_t fwd_timer;

			/// Delay for allocation requests from PEP (in ms)
			int pep_alloc_delay;

			SpotDownward* spot;

			// Frame interval
			std::shared_ptr<Probe<float>> probe_frame_interval;
	};

 protected:
	bool initListsSts();

	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;
	bool disable_control_plane;

	/// The list of Sts with forward/down modcod for this spot
	StFmtSimuList* output_sts;

	/// The list of Sts with return/up modcod for this spot
	StFmtSimuList* input_sts;
};


#endif

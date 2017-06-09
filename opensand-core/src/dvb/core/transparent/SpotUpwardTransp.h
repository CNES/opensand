/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file SpotUpwardTransp.h
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SPOT_UPWARD_TRANSP_H
#define SPOT_UPWARD_TRANSP_H

#include "SpotUpward.h"
#include "PhysicStd.h"  
#include "NetBurst.h"
#include "SlottedAlohaNcc.h"
#include "TimeSeriesGenerator.h"

#define SIMU_BUFF_LEN 255

class SpotUpwardTransp: public SpotUpward
{
	public:
		SpotUpwardTransp(spot_id_t spot_id,
		                 tal_id_t mac_id,
		                 StFmtSimuList *input_sts,
		                 StFmtSimuList *output_sts);
		virtual ~SpotUpwardTransp();
		bool onInit();


		/**
		 * @brief Handle a DVB frame
		 *
		 * @param frame  The frame
		 * @param burst  OUT: the burst of packets
		 * @return true on success, false otherwise
		 */
		bool handleFrame(DvbFrame *frame, NetBurst **burst);

		/**
		 * @brief get CNI in a frame
		 *
		 * @param dvb_frame the Dvb Frame corrupted
		 */
		void handleFrameCni(DvbFrame *dvb_frame);

		/**
		 * @brief Schedule Slotted Aloha carriers
		 *
		 *	@param dvb_frame   a SoF
		 *  @param ack_frames  OUT: The generated ACK frames
		 *  @param sa_burst    OUT: The Slotted Aloha bursts received
		 *  @return true on success, false otherwise
		 */
		bool scheduleSaloha(DvbFrame *dvb_frame,
		                    list<DvbFrame *>* &ack_frames,
		                    NetBurst **sa_burst);

		/**
		 *  @brief Handle a logon request transmitted by the lower layer
		 *
		 *  @param logon_req  The frame contining the logon request
		 *  @return true on success, false otherwise
		 */
		bool onRcvLogonReq(DvbFrame *dvb_frame);

		/**
		 *  @brief Handle a Slotted Aloha Data Frame
		 *
		 *  @param frame  The Slotted Aloha data frame
		 *  @return true on success, false otherwise
		 */
		bool handleSlottedAlohaFrame(DvbFrame *frame);

		/**
		 * @brief  Add a new line in the MODCOD time series generator file
		 *
		 *  @return true on success, false otherwise
		 */
		bool updateSeriesGenerator(void);

	protected:

		/**
		 * @brief Read configuration for the different files and open them
		 *
		 * @return  true on success, false otherwise
		 */
		bool initModcodSimu(void);

		/**
		 * @brief Initialize the ACM loop margins
		 *
		 * @return  true on success, false otherwise
		 */
		bool initAcmLoopMargin(void);

		/**
		 *  @brief Initialize the time series generators
		 *
		 *  @return  true on success, false otherwise
		 */
		bool initSeriesGenerator(void);

		/**
		 * @brief Initialize the transmission mode
		 *
		 * @return  true on success, false otherwise
		 */
		bool initMode(void);

		/**
		 * Read configuration for the Slotted Aloha algorithm
		 *
		 * @return  true on success, false otherwise
		 */
		bool initSlottedAloha(void);

		/**
		 * @brief Initialize the statistics
		 *
		 * @return  true on success, false otherwise
		 */
		bool initOutput(void);

		/**
		 * Checks if SCPC mode is activated and configured
		 * (Available FIFOs and Carriers for SCPC)
		 *
		 * @sat_type     The satellite type
		 * @return       Whether there are SCPC FIFOs and SCPC Carriers
		 *               available or not
		 */
		bool checkIfScpc();
		
		/// The Slotted Aloha for NCC
		SlottedAlohaNcc *saloha;

		/// is terminal scpc map
		list<tal_id_t> is_tal_scpc;

		/// time series generator for input
		TimeSeriesGenerator *input_series;

		/// time series generator for output
		TimeSeriesGenerator *output_series;

	};

#endif

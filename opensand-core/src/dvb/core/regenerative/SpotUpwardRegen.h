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
 * @file SpotUpwardRegen.h
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SPOT_UPWARD_REGEN_H
#define SPOT_UPWARD_REGEN_H

#include "SpotUpward.h"

#define SIMU_BUFF_LEN 255

class SpotUpwardRegen: public SpotUpward
{
	public:
		SpotUpwardRegen(spot_id_t spot_id,
		                tal_id_t mac_id,
		                StFmtSimuList *input_sts,
		                StFmtSimuList *output_sts);
		virtual ~SpotUpwardRegen();
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

	};

#endif

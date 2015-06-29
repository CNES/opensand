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
 * @file SpotDownwardTransp.h
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SPOT_DOWNWARD_TRANSP_H
#define SPOT_DOWNWARD_TRANSP_H

#include "SpotDownward.h"
#include "DamaCtrlRcs.h"
#include "Scheduling.h"
#include "SlottedAlohaNcc.h"


class SpotDownwardTransp: public SpotDownward
{
	public:
		SpotDownwardTransp(spot_id_t spot_id,
		             tal_id_t mac_id,
		             time_ms_t fwd_down_frame_duration,
		             time_ms_t ret_up_frame_duration,
		             time_ms_t stats_period,
		             sat_type_t sat_type,
		             EncapPlugin::EncapPacketHandler *pkt_hdl,
		             bool phy_layer);
		~SpotDownwardTransp();
		bool onInit(void);
			
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

	
	protected:

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

};

#endif
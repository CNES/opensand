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
		             EncapPlugin::EncapPacketHandler *pkt_hdl,
		             StFmtSimuList *input_sts,
		             StFmtSimuList *output_sts);

		virtual ~SpotDownwardTransp();
		
		/**
		 * @brief Spot Downward initialisation
		 *
		 * @return true on success, false otherwise
		 */ 
		bool onInit(void);
			
		/**
		* @brief Handle the Slotted Aloha ACKs
		 *
		 * @param ack_frames  The Slotted Aloha ACKs
		 * @return true on success, false otherwise
		 */
		bool handleSalohaAcks(const list<DvbFrame *> *ack_frames);
	
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
		 * @brief add Cni extension into GSE packet (for SCPC)
		 *
		 * @return true on success, false otherwise
		 */ 
		bool addCniExt(void);
};

#endif

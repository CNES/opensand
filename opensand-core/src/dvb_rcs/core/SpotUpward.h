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
 * @file SpotUpward.h
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author SatIP6
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
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

#ifndef SPOT_UPWARD_H
#define SPOT_UPWARD_H

#include "BlockDvb.h"
#include "PhysicStd.h"  
#include "NetBurst.h"
#include "DamaCtrlRcs.h"
#include "NccPepInterface.h"
#include "Scheduling.h"
#include "SlottedAlohaNcc.h"

#define SIMU_BUFF_LEN 255

class SpotUpward: public DvbChannel
{
	public:
		SpotUpward();
		~SpotUpward();
		bool onInit();
		/**
		 * set/get upward ispot id
		 */ 
		void setSpotId(uint8_t spot_id);
		uint8_t getSpotId(void);

		PhysicStd *getReceptionStd(void); 
		SlottedAlohaNcc *getSaloha(void);
		Probe<int> *getProbeReceivedModcod(void);
		Probe<int> *getProbeRejectedModcod(void);
		void setL2FromSatBytes(int l2_from_sat);
		int getL2FromSatBytes(void);
		tal_id_t getMacId(void);


		/// DVB frame from lower layer
		bool onRcvLogonReq(DvbFrame *dvb_frame);

		// statistics update
		void updateStats(void);


	protected:
		
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
		 * Transmist a frame to the opposite channel
		 *
		 * @param frame  The dvb frame
		 * @return true on success, false otherwise
		 */
		bool shareFrame(DvbFrame *frame);

		/// Spot Id
		uint8_t spot_id;
		
		/// reception standard (DVB-RCS or DVB-S2)      
		PhysicStd *reception_std; 

		/// The Slotted Aloha for NCC
		SlottedAlohaNcc *saloha;

		/// ST unique mac id
		tal_id_t mac_id;

		/// FMT groups for up/return
		fmt_groups_t ret_fmt_groups;

		// Output probes and stats
		// Rates
		// Layer 2 from SAT
		Probe<int> *probe_gw_l2_from_sat;
		int l2_from_sat_bytes;
		// Physical layer information
		Probe<int> *probe_received_modcod;
		Probe<int> *probe_rejected_modcod;

		/// log for slotted aloha
		OutputLog *log_saloha;

		/// logon request events
		OutputEvent *event_logon_req;
};

#endif

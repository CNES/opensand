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
 * @file DvbS2Std.h
 * @brief DVB-S2 Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DVB_S2_STD_H
#define DVB_S2_STD_H

#include "PhysicStd.h"
#include "BBFrame.h"
#include "FmtDefinitionTable.h"



/**
 * @class DvbS2Std
 * @brief DVB-S2 Transmission Standard
 */
class DvbS2Std: public PhysicStd
{

 private:

	// TODO create a type for modcod_id
	/** The real MODCOD of the ST */
	uint8_t real_modcod;

	/** The received MODCOD */
	uint8_t received_modcod;

 public:

	/**
	 * Build a DVB-S2 Transmission Standard
	 *
	 * @param packet_handler the packet handler
	 * @param frame_duration the frame duration
	 * @param bandwidth      the bandwidth

	 */
	DvbS2Std(const EncapPlugin::EncapPacketHandler *const pkt_hdl);

	/**
	 * Destroy the DVB-S2 Transmission Standard
	 */
	~DvbS2Std();

	/* only for NCC and Terminals */
	int onRcvFrame(unsigned char *frame,
	               long length,
	               long type,
	               tal_id_t tal_id,
	               NetBurst **burst);


#if 0 /* TODO: manage options */
	/**
	 * @brief Create real modcods option which is done when there is an update
	 *
	 * @param comp     the bloc component
	 * @param tal_id   the ID of the Satellite Terminal (ST)
	 * @param modcod   the modcod ID of destination terminal
	 * @param spot_id  the id of the spot of the destination terminal
	 * @return         true in case of success, false otherwise
	 */
	bool createOptionModcod(component_t comp, long pid,
	                        unsigned int modcod, long spot_id);
#endif

 private:

	/**
	 * @brief Get the real MODCOD of the terminal
	 *
	 * @return the real MODCOD
	 */
	int getRealModcod();

	/**
	 * @brief Get the received MODCOD of the terminal
	 *
	 * @return the received MODCOD
	 */
	int getReceivedModcod();


};

#endif

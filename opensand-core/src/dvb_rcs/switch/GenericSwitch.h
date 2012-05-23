/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file GenericSwitch.h
 * @brief Generic switch for Satellite Emulator (SE)
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef GENERIC_SWITCH__H
#define GENERIC_SWITCH__H

#include "NetPacket.h"
#include <map>

/**
 * @class GenericSwitch
 * @brief Generic switch for Satellite Emulator (SE)
 */
class GenericSwitch
{
 protected:

	/// The switch table: association between a terminal id and a
	/// satellite spot ID
	std::map <uint8_t, uint8_t> switch_table;

 public:

	/**
	 * Build a generic switch
	 */
	GenericSwitch();

	/**
	 * Destroy the generic switch
	 */
	virtual ~GenericSwitch();

	/**
	 * Add an entry in the switch table
	 *
	 * @param tal_id  the satellite terminal
	 * @param spot_id the satellite spot associated with the terminal
	 * @return true if entry was successfully added, false otherwise
	 */
	bool add(uint8_t tal_id, uint8_t spot_id);

	/**
	 * Find the satellite spot to send the packet to
	 *
	 * @param packet the encapsulation packet to send
	 * @return the satellite spot ID to send the packet to
	 */
	uint8_t find(NetPacket *packet);
};

#endif

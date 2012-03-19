/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file GseSwitch.h
 * @brief GSE switch for Satellite Emulator (SE)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef GSE_SWITCH__H
#define GSE_SWITCH__H

#include <GenericSwitch.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <platine_conf/conf.h>


/**
 * @class GseSwitch
 * @brief GSE switch for Satellite Emulator (SE)
 */
class GseSwitch: public GenericSwitch
{
 protected:

	std::map < uint8_t, long > frag_id_table;

 public:

	/**
	 * Build a GSE switch
	 */
	GseSwitch();

	/**
	 * Destroy the GSE switch
	 */
	~GseSwitch();

	long find(NetPacket *packet);
};

#endif

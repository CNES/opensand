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
 * @file MpegSwitch.h
 * @brief MPEG switch for Satellite Emulator (SE)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef MPEG_SWITCH__H
#define MPEG_SWITCH__H

#include <GenericSwitch.h>
#include <NetPacket.h>
#include <MpegPacket.h>
#include <platine_conf/conf.h>


/**
 * @class MpegSwitch
 * @brief MPEG switch for Satellite Emulator (SE)
 */
class MpegSwitch: public GenericSwitch
{
 public:

	/**
	 * Build a MPEG switch
	 */
	MpegSwitch();

	/**
	 * Destroy the MPEG switch
	 */
	~MpegSwitch();

	long find(NetPacket *packet);
};

#endif

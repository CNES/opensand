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
 * @file DvbRcsStd.h
 * @brief DVB-RCS Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DVB_RCS_STD_H
#define DVB_RCS_STD_H

#include "PhysicStd.h"
#include "DvbRcsFrame.h"
#include "GenericSwitch.h"


/**
 * @class DvbRcsStd
 * @brief DVB-RCS Transmission Standard
 */
class DvbRcsStd: public PhysicStd
{

 public:

	/**
	 * Build a DVB-RCS Transmission Standard
	 *
	 * @param packet_handler The packet handler
	 */
	DvbRcsStd(const EncapPlugin::EncapPacketHandler *const pkt_hdl = NULL);

	/**
	 * Destroy the DVB-RCS Transmission Standard
	 */
	~DvbRcsStd();

	int onRcvFrame(unsigned char *frame,
	               long length,
	               long type,
	               tal_id_t tal_id,
	               NetBurst **burst);

	/* function for regenerative satellite */
	bool setSwitch(GenericSwitch *generic_switch);

 private:

	/// Switch which manages the different spots
	/// (for regenerative satellite only)
	GenericSwitch *generic_switch;

};

#endif

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
 * @file PhysicStd.h
 * @brief Generic Physical Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien BERNARD / Viveris Technologies
 */

#ifndef PHYSIC_STD_H
#define PHYSIC_STD_H


#include <opensand_rt/Ptr.h>

#include "EncapPlugin.h"
#include "DvbFrame.h"


class NetBurst;


/**
 * @class PhysicStd
 * @brief Generic Physical Transmission Standard
 */
class PhysicStd
{
private:
	/** The type of the DVB standard ("DVB-RCS" or "DVB-S2") */
	std::string type;

protected:
	/** The packet representation */
	std::shared_ptr<EncapPlugin::EncapPacketHandler> packet_handler;

public:
	/**
	 * Build a Physical Transmission Standard
	 *
	 * @param type     the type of the DVB standard
	 * @param pkt_hdl  the packet handler
	 */
	PhysicStd(std::string type,
	          std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl);

	/**
	 * Destroy the Physical Transmission Standard
	 */
	virtual ~PhysicStd() = default;

	/**
	 * Get the type of Physical Transmission Standard (DVB-RCS, DVB-S2, etc.)
	 *
	 * @return the type of Physical Transmission Standard
	 */
	std::string getType();

	/**
	 * Receive frame from lower layer and get the EncapPackets
	 *
	 * @param dvb_frame  the received DVB frame
	 * @param tal_id     the unsique terminal id
	 *                   (only used for DVB-S2)
	 * @param burst      OUT: a burst of encapsulation packets
	 * @return           true on success, false otherwise
	 */
	virtual bool onRcvFrame(Rt::Ptr<DvbFrame> dvb_frame,
	                        tal_id_t tal_id,
	                        Rt::Ptr<NetBurst> &burst) = 0;
};


#endif

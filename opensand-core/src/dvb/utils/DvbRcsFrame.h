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
 * @file DvbRcsFrame.h
 * @brief DVB-RCS frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef DVB_RCS_FRAME_H
#define DVB_RCS_FRAME_H

#include "DvbFrame.h"


/**
 * @class DvbRcsFrame
 * @brief The DVB-RCS Frame is only used for emulation purpose, it should
 *        be choosen with a relevant size in order to be totally included
 *        in one sat_carrier paquets (ie. < MTU for UDP)
 */
class DvbRcsFrame: public DvbFrameTpl<T_DVB_ENCAP_BURST>
{
 public:
	/**
	 * Build a DVB-RCS frame
	 *
	 * @param data    raw data from which a DVB-RCS frame can be created
	 * @param length  length of raw data
	 */
	DvbRcsFrame(const unsigned char *data, size_t length);

	/**
	 * Build a DVB-RCS frame
	 *
	 * @param data  raw data from which a DVB-RCS frame can be created
	 */
	DvbRcsFrame(const Data &data);

	/**
	 * Build a DVB-RCS frame
	 *
	 * @param data    raw data from which a DVB-RCS frame can be created
	 * @param length  length of raw data
	 */
	DvbRcsFrame(const Data &data, size_t length);

	/**
	 * Build an empty DVB-RCS frame
	 */
	DvbRcsFrame();

	/**
	 * Destroy the DVB-RCS frame
	 */
	~DvbRcsFrame();

	/**
	 * Get the number of encapsulation packets stored in the DVB frame
	 *
	 * @return the number of packets stored in the DVB frame
	 */
	uint16_t getNumPackets(void) const;

	/**
	 * @brief Set the MODCOD of the frame
	 *
	 * @param modcod_id  the MODCOD ID of the frame
	 */
	void setModcodId(uint8_t modcod_id);

	/**
	 * @brief Get the MODCOD of the frame
	 *
	 * @return  the MODCOD ID of the frame
	 */
	uint8_t getModcodId(void) const;

	// implementation of virtual functions
	bool addPacket(NetPacket *packet);
	void empty(void);
};

#endif

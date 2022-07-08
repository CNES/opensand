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
 * @file BBFrame.h
 * @brief BB frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BB_FRAME_H
#define BB_FRAME_H

#include "DvbFrame.h"

#include <opensand_output/OutputLog.h>

/**
 * @class BBFrame
 * @brief BB frame
 */
class BBFrame: public DvbFrameTpl<T_DVB_BBFRAME>
{
 public:

	/**
	 * Build a BB frame
	 *
	 * @param data    raw data from which a BB frame can be created
	 * @param length  length of raw data
	 */
	BBFrame(const unsigned char *data, size_t length);

	/**
	 * Build a BB frame
	 *
	 * @param data  raw data from which a BB frame can be created
	 */
	BBFrame(const Data &data);

	/**
	 * Build a BB frame
	 *
	 * @param data    raw data from which a BB frame can be created
	 * @param length  length of raw data
	 */
	BBFrame(const Data &data, size_t length);

	/**
	 * Build an empty BB frame
	 */
	BBFrame();

	/**
	 * Destroy the BB frame
	 */
	~BBFrame();

	// implementation of virtual functions
	bool addPacket(NetPacket *packet);
	void empty(void);

	// BB frame specific

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

	/**
	 * @brief Get the data length in the BBFrame
	 *
	 * @return  the data length
	 */
	uint16_t getDataLength(void) const;


	/**
	 * @brief Get the offset from header beginning to payload
	 *
	 * @return the offset
	 */
	size_t getOffsetForPayload(void); 

	/// The BBFrame log
	static std::shared_ptr<OutputLog> bbframe_log;
};

#endif

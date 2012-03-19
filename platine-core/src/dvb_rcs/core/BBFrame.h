/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file BBFrame.h
 * @brief BB frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BB_FRAME_H
#define BB_FRAME_H

#include <DvbFrame.h>


/**
 * @class BBFrame
 * @brief BB frame
 */
class BBFrame: public DvbFrame
{

 public:

	/**
	 * Build a BB frame
	 *
	 * @param data    raw data from which a BB frame can be created
	 * @param length  length of raw data
	 */
	BBFrame(unsigned char *data, unsigned int length);

	/**
	 * Build a BB frame
	 *
	 * @param data  raw data from which a BB frame can be created
	 */
	BBFrame(Data data);

	/**
	 * Duplicate a BB frame
	 *
	 * @param frame  the BB frame to duplicate
	 */
	BBFrame(BBFrame *frame);

	/**
	 * Build an empty BB frame
	 */
	BBFrame();

	/**
	 * Destroy the BB frame
	 */
	~BBFrame();

	// implementation of virtual functions
	uint16_t getPayloadLength();
	Data getPayload();
	bool addPacket(NetPacket *packet);
	void empty(void);
	void setEncapPacketEtherType(uint16_t type);

	// BB frame specific

	/**
	 * Get the MODCOD of the BB frame
	 *
	 * @return  the MODCOD ID of the frame
	 */
	unsigned int getModcodId(void);

	/**
	 * Set the MODCOD of the BB frame
	 *
	 * @param modcod_id  the MODCOD ID of the frame
	 */
	void setModcodId(unsigned int modcod_id);
};

#endif

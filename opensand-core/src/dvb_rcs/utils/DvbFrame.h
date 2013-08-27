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
 * @file DvbFrame.h
 * @brief DVB frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef DVB_FRAME_H
#define DVB_FRAME_H

#include "OpenSandFrames.h"

#include <NetPacket.h>

// TODO inherit from OpenSandFrame for endianess ?

/**
 * @class DvbFrame
 * @brief DVB frame
 */
class DvbFrame: public NetPacket
{

 protected:

	/** The maximum size (in bytes) of the DVB frame */
	unsigned int max_size;

	/** The number of encapsulation packets added to the DVB frame */
	unsigned int num_packets;

	/** The carrier Id */
	long carrier_id;

 public:

	/**
	 * Build a DVB frame
	 *
	 * @param data    raw data from which a DVB frame can be created
	 * @param length  length of raw data
	 */
	DvbFrame(unsigned char *data, unsigned int length);

	/**
	 * Build a DVB frame
	 *
	 * @param data  raw data from which a DVB frame can be created
	 */
	DvbFrame(Data data);

	/**
	 * Duplicate a DVB frame
	 *
	 * @param frame  the DVB frame to duplicate
	 */
	DvbFrame(DvbFrame *frame);

	/**
	 * Build an empty DVB frame
	 */
	DvbFrame();

	/**
	 * Destroy the DVB frame
	 */
	virtual ~DvbFrame();


	// implementation of virtual functions
	uint16_t getTotalLength();

	// functions dedicated to DVB frames

	/**
	 * Get the maximum size of the DVB frame
	 *
	 * @return  the size (in bytes) of the DVB frame
	 */
	unsigned int getMaxSize(void);

	/**
	 * Set the maximum size of the DVB frame
	 *
	 * @param size  the size (in bytes) of the DVB frame
	 */
	void setMaxSize(unsigned int size);

	/**
	 * Set the carrier ID of the DVB frame
	 *
	 * @param carrier_id  the carrier ID the frame will be sent on
	 */
	void setCarrierId(long carrier_id);

	/**
	 * How many free bytes are available in the DVB frame ?
	 *
	 * @return  the size (in bytes) of the free space in the DVB frame
	 */
	unsigned int getFreeSpace(void);

	/**
	 * Add an encapsulation packet to the DVB frame
	 *
	 * @param packet  the encapsulation packet to add to the DVB frame
	 * @return        true if the packet was added to the DVB frame,
	 *                false if an error occurred
	 */
	virtual bool addPacket(NetPacket *packet);

	/**
	 * Get the number of encapsulation packets stored in the DVB frame
	 */
	unsigned int getNumPackets(void);

	/**
	 * Empty the DVB frame
	 */
	virtual void empty(void) = 0;


	/**
	 * Set the type of encapsulation packets stored in the frame
	 *
	 * @param type  the EtherType of the encapsulated packets
	 */
	virtual void setEncapPacketEtherType(uint16_t type) = 0;

};

#endif

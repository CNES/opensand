/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file MacFifoElement.h
 * @brief Fifo element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef FIFO_ELEMENT_H
#define FIFO_ELEMENT_H


#include <syslog.h>

#include "NetPacket.h"

/**
 * @class MacFifoElement
 * @brief Fifo element
 */
class MacFifoElement
{
 protected:

	/// 0 if the element contains a DVB frame,
	/// 1 if the element contains a NetPacket
	int type;

	/// The data to store in the FIFO (if type = 0)
	unsigned char *data;
	/// The length of data
	size_t length;
	/// The data to store in the FIFO (if type = 1)
	NetPacket *packet;

	/// The arrival time of packet in FIFO (in ms)
	time_t tick_in;
	/// The minimal time the packet will output the FIFO (in ms)
	time_t tick_out;


 public:

	/**
	 * Build a fifo element
	 * @param data     The data to store in the FIFO
	 * @param length   The length of the data
	 * @param tick_in  The arrival time of packet in FIFO (in ms)
	 * @param tick_out The minimal time the packet will output the FIFO (in ms)
	 */
	MacFifoElement(unsigned char *data, size_t length,
	               time_t tick_in, time_t tick_out);

	/**
	 * Build a fifo element
	 * @param packet   The data to store in the FIFO
	 * @param tick_in  The arrival time of packet in FIFO (in ms)
	 * @param tick_out The minimal time the packet will output the FIFO (in ms)
	 */
	MacFifoElement(NetPacket *packet,
	               time_t tick_in, time_t tick_out);

	/**
	 * Destroy the fifo element
	 */
	~MacFifoElement();

	/**
	 * Get the data
	 * @return The data
	 */
	unsigned char *getData();


	/**
	 * Get the length of data
	 * @return The length of data
	 */
	size_t getDataLength();


	/**
	 * Set the packet
	 *
	 * @param packet The new packet
	 */
	void setPacket(NetPacket *packet);


	/**
	 * Get the packet
	 * @return The packet
	 */
	NetPacket *getPacket();


	/**
	 * Get the packet length
	 * @return The packet length
	 */
	size_t getTotalPacketLength();


	/**
	 * Get the type of data in FIFO element
	 * @return The type of data in FIFO element
	 */
	int getType();

	/**
	 * Get the arrival time of packet in FIFO (in ms)
	 * @return The arrival time of packet in FIFO
	 */
	time_t getTickIn();

	/**
	 * Get the minimal time the packet will output the FIFO (in ms)
	 * @return The minimal time the packet will output the FIFO
	 */
	time_t getTickOut();

};

#endif

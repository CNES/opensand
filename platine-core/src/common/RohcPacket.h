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
 * @file RohcPacket.h
 * @brief ROHC packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ROHC_PACKET_H
#define ROHC_PACKET_H

#include <NetPacket.h>


/**
 * @class RohcPacket
 * @brief ROHC packet
 */
class RohcPacket: public NetPacket
{
 protected:

	/// The Quality of Service for the packet
	int _qos;
	/// The MAC identifier of the communication channel used by the packet
	unsigned long _macId;
	/// The identifier for the ST which emited this packet
	long _talId;

 public:

	/**
	 * Build a ROHC packet
	 *
	 * @param data    raw data from which a ROHC packet can be created
	 * @param length  length of raw data
	 */
	RohcPacket(unsigned char *data, unsigned int length);

	/**
	 * Build a ROHC packet
	 *
	 * @param data  raw data from which a ROHC packet can be created
	 */
	RohcPacket(Data data);

	/**
	 * Build an empty ROHC packet
	 */
	RohcPacket();

	/**
	 * Destroy the ROHC packet
	 */
	~RohcPacket();

	// implementation of virtual functions
	bool isValid();
	uint16_t totalLength();
	uint16_t payloadLength();
	Data payload();
	int qos();
	void setQos(int qos);
	unsigned long macId();
	void setMacId(unsigned long macId);
	long talId();
	void setTalId(long talId);

	/**
	 * Create a ROHC packet
	 *
	 * @param data  raw data from which a ROHC packet can be created
	 * @return      the created ROHC packet
	 */
	static NetPacket *create(Data data);

	void setType(uint16_t type);
};

#endif

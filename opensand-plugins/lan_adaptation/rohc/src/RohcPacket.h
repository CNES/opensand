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
 public:

	/**
	 * Build a ROHC packet
	 *
	 * @param data    raw data from which a ROHC packet can be created
	 * @param length  length of raw data
	 * @param type    the type of compressed packet or NET_PROTO_ROHC
	 */
	RohcPacket(unsigned char *data, unsigned int length, uint16_t type);

	/**
	 * Build a ROHC packet
	 *
	 * @param data  raw data from which a ROHC packet can be created
	 * @param type    the type of compressed packet or NET_PROTO_ROHC
	 */
	RohcPacket(Data data, uint16_t type);

	/**
	 * Build an empty ROHC packet
	 *
	 * @param type    the type of compressed packet or NET_PROTO_ROHC
	 */
	RohcPacket(uint16_t type);

	/**
	 * Destroy the ROHC packet
	 */
	~RohcPacket();

	// implementation of virtual functions
	uint16_t getTotalLength();
	uint16_t getPayloadLength();
	Data getPayload();
	uint8_t getQos();
	uint8_t getSrcTalId();
	uint8_t getDstTalId();

	/**
	 * Set the ROHC packet source terminal ID
	 * since it cannot be stored in a header field
	 *
	 * @param tal_id The terminal ID
	 */
	void setSrcTalId(uint8_t tal_id);

	/**
	 * Set the ROHC packet terminal ID
	 * since it cannot be stored in a header field
	 *
	 * @param tal_id The terminal ID
	 */
	void setDstTalId(uint8_t tal_id);

	/**
	 * Set the ROHC packet QoS value
	 * since it cannot be stored in a header field
	 *
	 * @param QoS The QoS value
	 */
	void setQos(uint8_t qos);

};

#endif

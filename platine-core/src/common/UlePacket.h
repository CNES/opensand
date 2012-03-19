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
 * @file UlePacket.h
 * @brief Ule packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ULE_PACKET_H
#define ULE_PACKET_H

#include <NetPacket.h>

/// The length of the ULE header (in bytes)
#define ULE_HEADER_LEN 4
/// The length of the ULE Destination Address field (in bytes)
#define ULE_ADDR_LEN   6
/// The length of the ULE CRC (in bytes)
#define ULE_CRC_LEN    4


/**
 * @class UlePacket
 * @brief ULE packet
 */
class UlePacket: public NetPacket
{
 protected:

	/// The Quality of Service for the IP packet
	int _qos;
	/// The MAC identifier of the communication channel used by the IP packet
	unsigned long _macId;
	/// The identifier for the ST which emited this IP packet
	long _talId;

	/**
	 * Calculate the CRC for ULE data
	 *
	 * @param pos  the index of first byte to use
	 * @param len  the number of bytes to use
	 * @return     the calculated CRC
	 */
	uint32_t calcCrc(unsigned int pos, unsigned int len);

 public:

	/**
	 * Build an ULE packet
	 *
	 * @param data    raw data from which an ULE packet can be created
	 * @param length  length of raw data
	 */
	UlePacket(unsigned char *data, unsigned int length);

	/**
	 * Build an ULE packet
	 *
	 * @param data  raw data from which an ULE packet can be created
	 */
	UlePacket(Data data);

	/**
	 * Build an empty ULE packet
	 */
	UlePacket();

	/**
	 * Build an ULE packet
	 *
	 * @param type     the protocol type of the payload data
	 * @param address  the optional address to add in ULE header (specify NULL
	 *                 to not use ULE destination address field)
	 * @param payload  data of ULE payload
	 */
	UlePacket(uint16_t type, Data *address, Data payload);

	/**
	 * Destroy the ULE packet
	 */
	~UlePacket();

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
	 * @brief Whether the Destination Address field of the ULE header is present
	 *        or not
	 *
	 * @return  true if the Destination Address field is present, false otherwise
	 */
	bool isDestAddrPresent();

	/**
	 * Get the Type field of the ULE header
	 *
	 * @return  the Type field of the ULE header
	 */
	uint16_t payloadType();

	/**
	 * Get the Destination Address field of the ULE header
	 *
	 * @return  the Destination Address field of the ULE header
	 */
	Data destAddr();

	/**
	 * Get the CRC field at the end of the ULE packet
	 *
	 * @return  the CRC field at the end of the ULE packet
	 */
	uint32_t crc();

	/**
	 * Create an ULE packet
	 *
	 * @param data  raw data from which an ULE packet can be created
	 * @return      the created ULE packet
	 */
	static NetPacket * create(Data data);

	/**
	 * Get the length of a given ULE packet
	 *
	 * @param data    raw data which contains at least one ULE packet
	 * @param offset  the offset in data where the ULE packet starts
	 * @return        the length of the ULE packet
	 */
	static unsigned int length(Data *data, unsigned int offset);
};

#endif

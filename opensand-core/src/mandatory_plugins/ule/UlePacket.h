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

	/**
	 * Calculate the CRC for ULE data
	 *
	 * @param pos     the index of first byte to use
	 * @param len     the number of bytes to use
	 * @param enabled is the CRC computing enabled
	 * @return     the calculated CRC
	 */
	uint32_t calcCrc(bool enabled) const;

 public:

	/**
	 * Build an ULE packet
	 *
	 * @param data    raw data from which an ULE packet can be created
	 * @param length  length of raw data
	 */
	UlePacket(const unsigned char *data, size_t length);

	/**
	 * Build an ULE packet
	 *
	 * @param data  raw data from which an ULE packet can be created
	 */
	UlePacket(const Data &data);

	/**
	 * Build an ULE packet
	 *
	 * @param data    raw data from which an ULE packet can be created
	 * @param length  length of raw data
	 */
	UlePacket(const Data &data, size_t length);

	/**
	 * Build an empty ULE packet
	 */
	UlePacket();

	/**
	 * Build an ULE packet
	 *
	 * @param type        the protocol type of the payload data
	 * @param address     the optional address to add in ULE header (specify NULL
	 *                    to not use ULE destination address field)
	 * @param crc_enabled is the CRC computing enabled
	 * @param payload  data of ULE payload
	 */
	UlePacket(uint16_t type, Data *address, Data payload, bool crc_enabled);

	/**
	 * Destroy the ULE packet
	 */
	~UlePacket();

	/**
	 * Check if the ULE packet is valid
	 *
	 * @param crc_enabled is the CRC computing enabled
	 * @return true if the packet is valied, false otherwise
	 */
	bool isValid(bool crc_enabled) const;

	// implementation of virtual functions
	size_t getPayloadLength() const;
	Data getPayload() const;

	/**
	 * @brief Whether the Destination Address field of the ULE header is present
	 *        or not
	 *
	 * @return  true if the Destination Address field is present, false otherwise
	 */
	bool isDstAddrPresent() const;

	/**
	 * Get the Type field of the ULE header
	 *
	 * @return  the Type field of the ULE header
	 */
	uint16_t getPayloadType() const;

	/**
	 * Get the Destination Address field of the ULE header
	 *
	 * @return  the Destination Address field of the ULE header
	 */
	Data destAddr() const;

	/**
	 * Get the CRC field at the end of the ULE packet
	 *
	 * @return  the CRC field at the end of the ULE packet
	 */
	uint32_t crc() const;

	/// The ULE packet log
	static std::shared_ptr<OutputLog> ule_log;
};

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file MpegPacket.h
 * @brief MPEG2-TS packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_PACKET_H
#define MPEG_PACKET_H

#include <NetPacket.h>


#define TS_PACKETSIZE 188   ///< The length of the MPEG2-TS packet (in bytes)
#define TS_HEADERSIZE 4     ///< The length of the MPEG2-TS header (in bytes)

/// The length of the MPEG2-TS payload (in bytes)
#define TS_DATASIZE (TS_PACKETSIZE - TS_HEADERSIZE)


/**
 * @class MpegPacket
 * @brief MPEG2-TS packet
 */
class MpegPacket: public NetPacket
{
 public:

	/**
	 * Build a MPEG2-TS packet
	 * @param data raw data from which a MPEG2-TS packet can be created
	 * @param length length of raw data
	 */
	MpegPacket(const unsigned char *data, size_t length);

	/**
	 * Build a MPEG2-TS packet
	 * @param data raw data from which a MPEG2-TS packet can be created
	 */
	MpegPacket(const Data &data);

	/**
	 * Build a MPEG2-TS packet
	 * @param data raw data from which a MPEG2-TS packet can be created
	 * @param length length of raw data
	 */
	MpegPacket(const Data &data, size_t length);

	/**
	 * Build an empty MPEG2-TS packet
	 */
	MpegPacket();

	/**
	 * Destroy the MPEG2-TS packet
	 */
	~MpegPacket();

	// implementation of virtual functions
	// TODO add isValid in NetPacket
	bool isValid() const;
	uint8_t getQos();
	uint8_t getSrcTalId();
	uint8_t getDstTalId();

	/**
	 * Get the Synchronization byte of the MPEG2-TS header
	 *
	 * @return the Synchronization byte of the MPEG2-TS header
	 */
	uint8_t sync() const;

	/**
	 * @brief Whether the Transport Error Indicator (TEI) bit of the MPEG2-TS
	 *        header is set or not
	 *
	 * @return true if the TEI bit is set, false otherwise
	 */
	bool tei() const;

	/**
	 * @brief Whether the Payload Unit Start Indicator (PUSI) bit of the MPEG2-TS
	 *        header is set or not
	 *
	 * @return true if the PUSI bit is set, false otherwise
	 */
	bool pusi() const;

	/**
	 * @brief Whether the Transport Priority (TP) bit of the MPEG2-TS header is
	 *        set or not
	 *
	 * @return true if the TP bit is set, false otherwise
	 */
	bool tp() const;

	/**
	 * Retrieve the PID field from the MPEG2-TS header
	 *
	 * @return the PID field from the MPEG2-TS header
	 */
	uint16_t getPid() const;

	/**
	 * Get the Transport Scrambling Control (TSC) of the MPEG2-TS header
	 *
	 * @return the TSC of the MPEG2-TS header
	 */
	uint8_t tsc() const;

	/**
	 * Get the Continuity Counter (CC) of the MPEG2-TS header
	 *
	 * @return the CC of the MPEG2-TS header
	 */
	uint8_t cc() const;

	/**
	 * Get the Payload Pointer (PP) of the MPEG2-TS header
	 *
	 * @return the PP of the MPEG2-TS header
	 */
	uint8_t pp() const;

	/**
	 * @brief  Get the PID of a MPEG packet from a NetPacket.
	 *
	 * @param   packet  The packet to read values from.
	 * @return  The PID field
	 */
	static uint16_t getPidFromPacket(NetPacket *packet);

	/// The MPEG packet log
	static OutputLog *mpeg_log;
};

#endif

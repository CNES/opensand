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
	MpegPacket(unsigned char *data, unsigned int length);

	/**
	 * Build a MPEG2-TS packet
	 * @param data raw data from which a MPEG2-TS packet can be created
	 */
	MpegPacket(Data data);

	/**
	 * Build an empty MPEG2-TS packet
	 */
	MpegPacket();

	/**
	 * Destroy the MPEG2-TS packet
	 */
	~MpegPacket();

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
	 * Get the Synchronization byte of the MPEG2-TS header
	 *
	 * @return the Synchronization byte of the MPEG2-TS header
	 */
	uint8_t sync();

	/**
	 * @brief Whether the Transport Error Indicator (TEI) bit of the MPEG2-TS
	 *        header is set or not
	 *
	 * @return true if the TEI bit is set, false otherwise
	 */
	bool tei();

	/**
	 * @brief Whether the Payload Unit Start Indicator (PUSI) bit of the MPEG2-TS
	 *        header is set or not
	 *
	 * @return true if the PUSI bit is set, false otherwise
	 */
	bool pusi();

	/**
	 * @brief Whether the Transport Priority (TP) bit of the MPEG2-TS header is
	 *        set or not
	 *
	 * @return true if the TP bit is set, false otherwise
	 */
	bool tp();

	/**
	 * Retrieve the PID field from the MPEG2-TS header
	 *
	 * @return the PID field from the MPEG2-TS header
	 */
	uint16_t pid();

	/**
	 * Set the PID field of the MPEG2-TS header
	 * PID (13 bits) = MAC id (8 bits) + TAL id (3 bits) + QoS (2 bits)
	 *
	 * @param pid  the PID field of the MPEG2-TS header
	 */
	void setPid(uint16_t pid);

	/**
	 * Get the Transport Scrambling Control (TSC) of the MPEG2-TS header
	 *
	 * @return the TSC of the MPEG2-TS header
	 */
	uint8_t tsc();

	/**
	 * Get the Continuity Counter (CC) of the MPEG2-TS header
	 *
	 * @return the CC of the MPEG2-TS header
	 */
	uint8_t cc();

	/**
	 * Get the Payload Pointer (PP) of the MPEG2-TS header
	 *
	 * @return the PP of the MPEG2-TS header
	 */
	uint8_t pp();

	/**
	 * Get the length of a MPEG2-TS packet (= 188 bytes)
	 * @return the MPEG2-TS packet length
	 */
	static unsigned int length();

	/**
	 * Create a MPEG2-TS packet
	 *
	 * @param data  raw data from which a MPEG2-TS packet can be created
	 * @return      the created MPEG2-TS packet
	 */
	static NetPacket * create(Data data);
};

#endif

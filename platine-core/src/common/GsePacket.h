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
 * @file GsePacket.h
 * @brief GSE packet
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef GSE_PACKET_H
#define GSE_PACKET_H

#include <NetPacket.h>

extern "C"
{
	#include <gse/header_fields.h>
}

/**
 * @class GsePacket
 * @brief GSE packet
 */
class GsePacket: public NetPacket
{
 protected:

	/// The Quality of Service for the packet
	int _qos;
	/// The MAC identifier of the communication channel used by the packet
	unsigned long _macId;
	/// The identifier for the ST which emited this packet
	long _talId;
	/// The PID of this packet
	uint16_t _pid;

 public:

	/**
	 * Build a GSE packet
	 *
	 * @param data    raw data from which a GSE packet can be created
	 * @param length  length of raw data
	 */
	GsePacket(unsigned char *data, unsigned int length);

	/**
	 * Build a GSE packet
	 *
	 * @param data  raw data from which a GSE packet can be created
	 */
	GsePacket(Data data);

	/**
	 * Build an empty GSE packet
	 */
	GsePacket();

	/**
	 * Destroy the GSE packet
	 */
	~GsePacket();

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
	 * Create a GSE packet
	 *
	 * @param data  raw data from which a GSE packet can be created
	 * @return      the created GSE packet
	 */
	static NetPacket * create(Data data);

	/**
	 * Get the length of a given GSE packet
	 *
	 * @param data    raw data which contains at least one GSE packet
	 * @param offset  the offset in data where the GSE packet starts
	 * @return        the length of the GSE packet
	 */
	static uint16_t length(unsigned char *data, unsigned int offset);

	/**
	 * Get Start Indicator field of a given GSE packet
	 *
	 * @return  the Start Indicator field of the GSE packet
	 */
	uint8_t start_indicator();

	/**
	 * Get End Indicator field of a given GSE packet
	 *
	 * @return  the End Indicator field of the GSE packet
	 */
	uint8_t end_indicator();

	/**
	 * Get Frag ID field of a given GSE packet
	 *
	 * @return  the Frag ID field of the GSE packet
	 */
	uint8_t fragId();
};

#endif

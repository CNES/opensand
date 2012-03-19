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
 * @file Aal5Packet.h
 * @brief AAL5 packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef AAL5_PACKET_H
#define AAL5_PACKET_H

#include <NetPacket.h>
#include <AtmCell.h>


/**
 * @class Aal5Packet
 * @brief AAL5 packet
 */
class Aal5Packet: public NetPacket
{
 private:

	/// The Quality of Service for the IP packet
	int _qos;
	/// The MAC identifier of the communication channel used by the IP packet
	unsigned long _macId;
	/// The identifier for the ST which emited this IP packet
	long _talId;

	/// Is the validity of the AAL5 packet already checked?
	bool validityChecked;
	/// If AAL5 packet validity is checked, what is the result?
	bool validityResult;

 public:

	/**
	 * Build an AAL5 packet from raw data
	 * @param data raw data from which an AAL5 packet can be created
	 * @param length length of raw data
	 */
	Aal5Packet(unsigned char *data, unsigned int length);

	/**
	 * Build an AAL5 packet from raw data
	 * @param data raw data to create an AAL5 packet from
	 */
	Aal5Packet(Data data);

	/**
	 * Build an empty AAL5 packet
	 */
	Aal5Packet();

	/**
	 * Destroy the AAL5 packet
	 */
	~Aal5Packet();

	// implementation of virtual functions
	// implementation of virtual functions
	int qos();
	void setQos(int qos);
	unsigned long macId();
	void setMacId(unsigned long macId);
	long talId();
	void setTalId(long talId);
	bool isValid();
	uint16_t totalLength();
	uint16_t payloadLength();
	Data payload();

	/**
	 * Create an AAL5 packet from its payload
	 * @param payload the payload of the AAL5 packet to be created
	 * @return a newly created AAL5 packet
	 */
	static Aal5Packet * createFromPayload(Data payload);

	/**
	 * Get the number of ATM cells needed to encapsulate payload data into an
	 * AAL5 packet and fragment this packet into one or several ATM cells
	 * @return number of ATM cells
	 */
	unsigned int nbAtmCells();

	/**
	 * Get the ATM cell at the given position in AAL5 packet
	 * @param index index of the wanted ATM cell (index starts at 0)
	 * @return the raw data of the ATM cell at the position given by parameter
	 *         index
	 */
	Data atmCell(unsigned int index);

	/**
	 * Create an AAL5 packet
	 *
	 * @param data  raw data from which an AAL5 packet can be created
	 * @return      the created AAL5 packet
	 */
	static NetPacket * create(Data data);

 protected:

	/**
	 * Calculate some data CRC
	 * @param data data to calculate the CRC from
	 * @return the checksum
	 */
	static uint32_t calcCrc(Data data);

	/**
	 * Retrieve the CRC from the AAL5 trailer
	 * @return the CRC from the AAL5 trailer
	 */
	uint32_t crc();
};

#endif

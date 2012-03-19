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
 * @file IpPacket.h
 * @brief Generic IP packet, either IPv4 or IPv6
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IP_PACKET_H
#define IP_PACKET_H

#include <string>

#include <Data.h>
#include <NetPacket.h>
#include <IpAddress.h>


/**
 * @class IpPacket
 * @brief Generic IP packet, either IPv4 or IPv6
 */
class IpPacket: public NetPacket
{
 protected:

	/// The Quality of Service for the IP packet
	int _qos;
	/// The MAC identifier of the communication channel used by the IP packet
	unsigned long _macId;
	/// The identifier for the ST which emited this IP packet
	long _talId;

	/// Internal cache for IP source address
	IpAddress *_srcAddr;
	/// Internal cache for IP destination address
	IpAddress *_destAddr;

 public:

	/**
	 * Build an IP packet
	 * @param data raw data from which an IP packet can be created
	 * @param length length of raw data
	 */
	IpPacket(unsigned char *data, unsigned int length);

	/**
	 * Build an IP packet
	 * @param data raw data from which an IP packet can be created
	 */
	IpPacket(Data data);

	/**
	 * Build an empty IP packet
	 */
	IpPacket();

	/**
	 * Destroy the IP packet
	 */
	virtual ~IpPacket();

	// implementation of virtual functions
	int qos();
	void setQos(int qos);
	unsigned long macId();
	void setMacId(unsigned long macId);
	long talId();
	void setTalId(long talId);
	Data payload();

	/**
	 * Retrieve the version from an IP packet
	 * @param data IP data
	 * @return the IP version
	 */
	static int version(Data data);

	/**
	 * Retrieve the version from an IP packet
	 * @param data IP data
	 * @param length IP data length
	 * @return the IP version
	 */
	static int version(unsigned char *data, unsigned int length);

	/**
	 * Retrieve the version of the IP packet
	 * @return the version of the IP packet
	 */
	int version();

	/**
	 * Retrieve the source address of the IP packet
	 * @return the IP source address
	 */
	virtual IpAddress *srcAddr() = 0;

	/**
	 * Retrieve the destination address of the IP packet
	 * @return the IP destination address
	 */
	virtual IpAddress *destAddr() = 0;

	/**
	 * Retrieve the Type Of Service (TOS) or Traffic Class (TC) of the IP packet
	 * @return the Type Of Service (TOS) or Traffic Class (TC) of the IP packet
	 */
	virtual uint8_t trafficClass() = 0;
};

#endif

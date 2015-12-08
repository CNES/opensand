/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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

	/// Internal cache for IP source address
	IpAddress *src_addr;
	/// Internal cache for IP destination address
	IpAddress *dst_addr;

 public:

	/**
	 * Build an IP packet
	 * @param data raw data from which an IP packet can be created
	 * @param length length of raw data
	 */
	IpPacket(const unsigned char *data, size_t length);

	/**
	 * Build an IP packet
	 * @param data raw data from which an IP packet can be created
	 */
	IpPacket(const Data &data);

	/**
	 * Build an IP packet
	 * @param data raw data from which an IP packet can be created
	 * @param length length of raw data
	 */
	IpPacket(const Data &data, size_t length);

	/**
	 * Build an empty IP packet
	 */
	IpPacket();

	/**
	 * Destroy the IP packet
	 */
	virtual ~IpPacket();

	// implementation of virtual functions
	Data getPayload() const;

	/**
	 * Retrieve the version from an IP packet
	 * @param data IP data
	 * @return the IP version
	 */
	static int version(Data data);

	/**
	 * Retrieve the version of the IP packet
	 * @return the version of the IP packet
	 */
	int version() const;

	/**
	 * Retrieve the source address of the IP packet
	 * @return the IP source address
	 */
	virtual IpAddress *srcAddr() = 0;

	/**
	 * Retrieve the destination address of the IP packet
	 * @return the IP destination address
	 */
	virtual IpAddress *dstAddr() = 0;

	/**
	 * Retrieve the DiffServField of the IP packet, that is the Type Of Service
	 * (TOS) (IPv4) or Traffic Class (TC) (IPv6) of the IP packet
	 * @return the DiffServField, that is the Type Of Service (TOS) 
	 * or Traffic Class (TC) of the IP packet
	 */
	virtual uint8_t diffServField() const = 0;

	/** 
	 * Retrieve the diffServCodePoint (DSCP) value of the IP packet, 
	 * that is the 6 leftmost bits of the DiffServField.
	 * @return the diffServCodePoint (DSCP)
	 */
	virtual uint8_t diffServCodePoint() const = 0;

	/**
	 * Retrieve the explicitCongestionNoficiation (ECN) value of the IP packet,
	 * that is the 2 rightmost bits of the DiffServField. 
	 * @return the explicitCongestionNotification (ECN)
	 */
	virtual uint8_t explicitCongestionNotification() const = 0;

	/**
	 * Is the network-layer packet a valid one?
	 * @return true if network-layer packet is valid, false otherwise
	 */
	virtual bool isValid() const = 0;

	/// The IP packet log
	static OutputLog *ip_log;
};

#endif

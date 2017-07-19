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
 * @file Ipv6Packet.h
 * @brief IPv6 packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IPV6_PACKET_H
#define IPV6_PACKET_H

#include <Data.h>
#include <IpPacket.h>
#include <Ipv6Address.h>


/**
 * @class Ipv6Packet
 * @brief IPv6 Packet
 */
class Ipv6Packet: public IpPacket
{
 public:

	/**
	 * Build an IPv6 packet
	 * @param data raw data from which an IPv6 packet can be created
	 * @param length length of raw data
	 */
	Ipv6Packet(const unsigned char *data, size_t length);

	/**
	 * Build an IPv6 packet
	 * @param data raw data from which an IPv6 packet can be created
	 */
	Ipv6Packet(const Data &data);

	/**
	 * Build an IPv6 packet
	 * @param data raw data from which an IPv6 packet can be created
	 * @param length length of raw data
	 */
	Ipv6Packet(const Data &data, size_t length);

	/**
	 * Build an empty IPv6 packet
	 */
	Ipv6Packet();

	/**
	 * Destroy the IPv6 packet
	 */
	~Ipv6Packet();

	// implementation of virtual functions
	bool isValid() const;
	size_t getTotalLength() const;
	size_t getPayloadLength() const;
	IpAddress *srcAddr();
	IpAddress *dstAddr();
	uint8_t diffServField() const;
	uint8_t diffServCodePoint() const;
	uint8_t explicitCongestionNotification() const;
};

#endif

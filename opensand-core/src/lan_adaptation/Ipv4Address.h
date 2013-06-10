/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file Ipv4Address.h
 * @brief IPv4 address
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IPV4_ADDRESS_H
#define IPV4_ADDRESS_H

#include <stdint.h>
#include <sstream>
#include <IpAddress.h>
#include <string>


/**
 * @class Ipv4Address
 * @brief IPv4 address
 */
class Ipv4Address: public IpAddress
{
 public:

	/// Internal representation of IPv4 address
	uint8_t _ip[4];

	/**
	 * Get a numerical representation of the IPv4 address
	 * @return a numerical representation of the IPv4 address
	 */
	uint32_t ip() const;

 public:

	/**
	 * Build an IPv4 address
	 * @param ip1 first byte of address
	 * @param ip2 second byte of address
	 * @param ip3 third byte of address
	 * @param ip4 fourth byte of address
	 */
	Ipv4Address(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4);

	/**
	 * Build an IPv4 address from a string
	 * @author THALES ALENIA SPACE
	 * @param s human representation of a ipv4 address
	 */
	Ipv4Address(std::string s);

	/**
	 * Destroy the IPv4 address
	 */
	~Ipv4Address();

	/**
	 * Get the length (in bytes) of an IPv4 address
	 * @return the length of an IPv4 address
	 */
	static unsigned int length();

	std::string str();
	bool matchAddressWithMask(const IpAddress *addr, unsigned int mask) const;
	int version() const;
};

#endif

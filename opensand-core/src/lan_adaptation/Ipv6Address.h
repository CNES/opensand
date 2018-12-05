/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
 * @file Ipv6Address.h
 * @brief IPv6 address
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IPV6_ADDRESS_H
#define IPV6_ADDRESS_H

#include <stdint.h>
#include <netinet/in.h>

#include <sstream>
#include <iomanip>
#include <string>

#include <IpAddress.h>


/**
 * @class Ipv6Address
 * @brief IPv6 address
 */
class Ipv6Address: public IpAddress
{
 protected:

	/// Internal representation of IPv6 address
	struct in6_addr _ip;

	/**
	 * Get a numerical representation of the IPv6 address
	 * @return a numerical representation of the IPv6 address
	 */
	struct in6_addr ip() const;

 public:

	/**
	 * Build an IPv6 address
	 * @param ip1 first byte of address
	 * @param ip2 second byte of address
	 * @param ip3 third byte of address
	 * @param ip4 fourth byte of address
	 * @param ip5 fifth byte of address
	 * @param ip6 sixth byte of address
	 * @param ip7 seventh byte of address
	 * @param ip8 eighth byte of address
	 * @param ip9 ninth byte of address
	 * @param ip10 tenth byte of address
	 * @param ip11 eleventh byte of address
	 * @param ip12 twelfth byte of address
	 * @param ip13 thirteenth byte of address
	 * @param ip14 fourteenth byte of address
	 * @param ip15 fifteenth byte of address
	 * @param ip16 sixteenth byte of address
	 */
	Ipv6Address(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4,
	            uint8_t ip5, uint8_t ip6, uint8_t ip7, uint8_t ip8,
	            uint8_t ip9, uint8_t ip10, uint8_t ip11, uint8_t ip12,
	            uint8_t ip13, uint8_t ip14, uint8_t ip15, uint8_t ip16);

	/**
	 * Build an IPv6 address from a string
	 * @author THALES ALENIA SPACE
	 * @param s human representation of a ipv6 address
	 */
	Ipv6Address(std::string s);

	/**
	 * Destroy the IPv6 address
	 */
	~Ipv6Address();

	/**
	 * Get the length (in bytes) of an IPv6 address
	 * @return the length of an IPv6 address
	 */
	static unsigned int length();

	std::string str();
	bool matchAddressWithMask(const IpAddress *addr, unsigned int mask) const;
	int version() const;
};

#endif

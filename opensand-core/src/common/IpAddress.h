/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file IpAddress.h
 * @brief Generic IP address, either IPv4 or IPv6
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IP_ADDRESS_H
#define IP_ADDRESS_H

#include <string>

/**
 * @class IpAddress
 * @brief Generic IP address, either IPv4 or IPv6
 */
class IpAddress
{
 public:

	/**
	 * Default constructor for IP address
	 */
	IpAddress();

	/**
	 * Destroy the IP address
	 */
	virtual ~IpAddress();

	/**
	 * Build a string representing the IP address
	 * @return the string representing the IP address
	 */
	virtual std::string str() = 0;

	/**
	 * Check whether IP address match another IP address
	 * @param addr the IP address to compare with
	 * @param mask the mask length to use for comparison
	 * @return true if IP addresses match, false otherwise
	 */
	virtual bool matchAddressWithMask(const IpAddress *addr,
	                                  unsigned int mask) const = 0;

	/**
	 * Get the IP address version, that is 4 or 6
	 * @return version of the IP address
	 */
	virtual int version() const = 0;
};

#endif

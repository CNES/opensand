/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file MacAddress.h
 * @brief Generic MAC address
 * @author Remy Pienne <Remy.Pienne@b2i-toulouse.com>
 */

#ifndef MAC_ADDRESS_H
#define MAC_ADDRESS_H

#include <string>
#include <stdint.h>

/**
 * @class MacAddress
 * @brief Generic MAC address
 */
class MacAddress
{
 private:

	/// The MAC address bytes
	uint8_t mac[6];

	/// The bytes that matches all occurences
	bool generic_bytes[6];

 public:
	/**
	 * Default constructor for MAC address
	 */
	MacAddress();

	/**
	 * Destroy the MAC address
	 */
	virtual ~MacAddress();

	/**
	 * @brief Build a MAC address from 6 bytes
	 *
     * @param b0 first byte of address
     * @param b1 second byte of address
     * @param b2 third byte of address
     * @param b3 fourth byte of address
     * @param b4 fifth byte of address
     * @param b5 sixth byte of address
	 */
	MacAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);

	/**
	 * @brief Build a MAC address from string
	 *
	 * @param mac_address The MAC address string representation
	 */
	MacAddress(std::string mac_address);

	/**
	 * @brief Build a string representing the MAC address
	 *
	 * @return the string representing the MAC address
	 */
	std::string str() const;

	/**
	 * @brief Access a byte ine the MAC address
	 *
	 * @param i  The byte we need to access
	 * @return the value of this byte
	 */
	unsigned char at(unsigned int i) const;

	/**
	 * @brief Check whether MAC address matches another MAC address
	 *
	 * @param addr the MAC address to compare with
	 * @return true if MAC addresses matches, false otherwise
	 */
	bool matches(const MacAddress *addr) const;
};

#endif

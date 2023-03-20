/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file MacAddress.cpp
 * @brief Generic MAC address
 * @author Remy Pienne <remy.pienne@toulouse.viveris.com>
 */


#include "MacAddress.h"

#include <cstdlib>
#include <sstream>
#include <iomanip>


MacAddress::MacAddress()
{
}

MacAddress::~MacAddress()
{
}

MacAddress::MacAddress(uint8_t b0,
                       uint8_t b1,
                       uint8_t b2,
                       uint8_t b3,
                       uint8_t b4,
                       uint8_t b5):
	mac{b0, b1, b2, b3, b4, b5},
	generic_bytes{false, false, false, false, false, false}
{
}


MacAddress::MacAddress(std::string mac_address)
{
	std::size_t index = 0;
	std::istringstream addr{mac_address};

	for (std::string token;
	     index < MacAddress::bytes_count && std::getline(addr, token, ':');
	     ++index)
	{
		this->generic_bytes[index] = token == "**";
		this->mac[index] = std::strtoul(token.c_str(), nullptr, 16);
	}
}


std::string MacAddress::str() const
{
	std::stringstream mac_addr;
	mac_addr << std::hex;

	for(std::size_t i = 0; i < MacAddress::bytes_count; ++i)
	{
		if (i) mac_addr << ":";
		mac_addr << std::setw(2)
		         << std::internal
		         << std::setfill('0')
		         << (unsigned int) this->mac[i];
	}

	return mac_addr.str();
}


unsigned char MacAddress::at(unsigned int i) const
{
	if(i < MacAddress::bytes_count)
	{
		return this->mac[i];
	}
	return 0;
}


bool MacAddress::matches(const MacAddress &addr) const
{
	for(std::size_t i = 0; i < MacAddress::bytes_count; ++i)
	{
		if(!this->generic_bytes[i] && this->mac[i] != addr.mac[i])
		{
			return false;
		}
	}
	return true;
}

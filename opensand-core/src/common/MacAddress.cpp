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

#include <cstdio>
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
	mac{b0, b1, b2, b3, b4, b5}
{
}


MacAddress::MacAddress(std::string mac_address)
{
	// TODO do the same for Ipv6Addr (i.e. use pure C++ functions)
	std::stringstream addr{mac_address};

	for(std::size_t index = 0; index < 6; ++index)
	{
		std::stringbuf token{""};
		std::stringstream ss;

		addr.get(token, ':');
		ss << token.str();

		if(ss.str() == "**")
		{
			this->generic_bytes[index] = true;
			this->mac[index] = 0;
		}
		else
		{
			this->generic_bytes[index] = false;
			uint8_t tmp;
			ss >> std::hex >> tmp;
			this->mac[index] = tmp;
		}

		addr.ignore();
		if(addr.fail()) break;
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


bool MacAddress::matches(const MacAddress *addr) const
{
	for(std::size_t i = 0; i < MacAddress::bytes_count; ++i)
	{
		if(this->generic_bytes[i])
		{
			continue;
		}
		if(this->mac[i] != addr->mac[i])
		{
			return false;
		}
	}
	return true;
}

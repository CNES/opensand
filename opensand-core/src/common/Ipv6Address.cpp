/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file Ipv6Address.cpp
 * @brief IPv6 address
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "Ipv6Address.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"
#include <cstdio>


Ipv6Address::Ipv6Address(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4,
                         uint8_t ip5, uint8_t ip6, uint8_t ip7, uint8_t ip8,
                         uint8_t ip9, uint8_t ip10, uint8_t ip11, uint8_t ip12,
                         uint8_t ip13, uint8_t ip14, uint8_t ip15, uint8_t ip16):
	IpAddress()
{
	this->_ip.s6_addr[0] = ip1;
	this->_ip.s6_addr[1] = ip2;
	this->_ip.s6_addr[2] = ip3;
	this->_ip.s6_addr[3] = ip4;
	this->_ip.s6_addr[4] = ip5;
	this->_ip.s6_addr[5] = ip6;
	this->_ip.s6_addr[6] = ip7;
	this->_ip.s6_addr[7] = ip8;
	this->_ip.s6_addr[8] = ip9;
	this->_ip.s6_addr[9] = ip10;
	this->_ip.s6_addr[10] = ip11;
	this->_ip.s6_addr[11] = ip12;
	this->_ip.s6_addr[12] = ip13;
	this->_ip.s6_addr[13] = ip14;
	this->_ip.s6_addr[14] = ip15;
	this->_ip.s6_addr[15] = ip16;
}

Ipv6Address::Ipv6Address(std::string s): IpAddress()
{
	int tmp;                     // read buffer
	int separatorPosition = -1;  // position of ':'
	std::string separator = ":"; // separator caracter
	int index;                   // index for loops
	int concatenationPosition;   // position of '::'
	std::string secondPart;      // sub-string after the '::'
	int size;                    // size of the sub-string 'secondPart'

	// Initialise all bytes
	for(index = 0; index < 16; index++)
		this->_ip.s6_addr[index] = 0;

	index = 0;

	// find bytes from the begining of the string
	while(s.compare(separatorPosition + 1, 1, separator) != 0 && index < 16)
	{
		// scan the part of the string
		sscanf(s.substr(separatorPosition + 1).c_str(), "%x:", &tmp);
		this->_ip.s6_addr[index] = (tmp >> 8) & 0xff;
		index++;
		this->_ip.s6_addr[index]= tmp & 0xff;
		index++;

		// find the next ':'
		separatorPosition = s.find(":", separatorPosition + 1);
	}

	// if there is a concatanation, find bytes from the end of the string
	if(separatorPosition != -1 &&
	   ((unsigned int) separatorPosition) + 2 != s.length())
	{
		index = 16;

		// create the second part sub-string
		concatenationPosition = separatorPosition;
		secondPart = s.substr(concatenationPosition + 2);
		size = secondPart.length();

		while(secondPart.compare(size, 1, separator) != 0 && size > 4)
		{
			// scan the part of the string
			sscanf(secondPart.substr(secondPart.find_last_of(":") + 1).c_str(),
			       "%x:", &tmp);
			this->_ip.s6_addr[index - 1] = tmp & 0xff;
			index--;
			this->_ip.s6_addr[index - 1]= (tmp >> 8) & 0xff;
			index--;

			// find the next ':'
			size = secondPart.find_last_of(":");
			secondPart = secondPart.substr(0, size);
		}

		// scan bytes just after '::'
		sscanf(secondPart.c_str(), "%x", &tmp);
		this->_ip.s6_addr[index - 1] = tmp & 0xff;
		index--;
		this->_ip.s6_addr[index - 1] = (tmp >> 8) & 0xff;
	}
}

Ipv6Address::~Ipv6Address()
{
}

struct in6_addr Ipv6Address::ip()
{
	return this->_ip;
}

// static
unsigned int Ipv6Address::length()
{
	return 16;
}

std::string Ipv6Address::str()
{
	std::basic_ostringstream < char >ip_addr;
	int fill_width;
	int i;

	ip_addr.fill('0');

	for(i = 0; i < 16; i += 2)
	{
		fill_width = 0;

		if(this->_ip.s6_addr[i] != 0)
		{
			ip_addr << std::hex << (int) this->_ip.s6_addr[i];
			fill_width = 2;
		}

		ip_addr << std::setw(fill_width) << std::hex
			<< (int) this->_ip.s6_addr[i + 1];

		if(i != 14)
			ip_addr << ":";
	}

	return ip_addr.str();
}

bool Ipv6Address::matchAddressWithMask(IpAddress *addr, unsigned int mask)
{
	struct in6_addr addr1, addr2;
	unsigned int index;
	uint8_t bitmask;
	unsigned int i;

	if(addr->version() != 6 || mask > (Ipv6Address::length() * 8))
		return false;

	addr1 = this->ip();
	addr2 = ((Ipv6Address *) addr)->ip();

	for(i = mask; i < (Ipv6Address::length() * 8); i++)
	{
		index = (int) (i / 8);
		bitmask = ~(1 << (8 - 1 - (i % 8)));

		addr1.s6_addr[index] &= bitmask;
		addr2.s6_addr[index] &= bitmask;
	}

	return (addr1.s6_addr32[0] == addr2.s6_addr32[0] &&
	        addr1.s6_addr32[1] == addr2.s6_addr32[1] &&
	        addr1.s6_addr32[2] == addr2.s6_addr32[2] &&
	        addr1.s6_addr32[3] == addr2.s6_addr32[3]);
}

int Ipv6Address::version()
{
	return 6;
}

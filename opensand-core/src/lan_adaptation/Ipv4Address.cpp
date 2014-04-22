/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file Ipv4Address.cpp
 * @brief IPv4 address
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "Ipv4Address.h"

#include <cstdio>


Ipv4Address::Ipv4Address(uint8_t ip1,
                         uint8_t ip2,
                         uint8_t ip3,
                         uint8_t ip4):
	IpAddress()
{
	this->_ip[0] = ip1;
	this->_ip[1] = ip2;
	this->_ip[2] = ip3;
	this->_ip[3] = ip4;
}

Ipv4Address::Ipv4Address(std::string s): IpAddress()
{
	int tmp1, tmp2, tmp3, tmp4;
	int ret;

	ret = sscanf(s.c_str(), "%u.%u.%u.%u",
	             &tmp1, &tmp2, &tmp3, &tmp4);
	if(ret != 4)
	{
		tmp1 = 0;
		tmp2 = 0;
		tmp3 = 0;
		tmp4 = 0;
	}

	if((tmp1 & 0xff) != tmp1)
		tmp1 = 0;
	if((tmp2 & 0xff) != tmp2)
		tmp2 = 0;
	if((tmp3 & 0xff) != tmp3)
		tmp3 = 0;
	if((tmp4 & 0xff) != tmp4)
		tmp4 = 0;

	this->_ip[0] = tmp1;
	this->_ip[1] = tmp2;
	this->_ip[2] = tmp3;
	this->_ip[3] = tmp4;
}

Ipv4Address::~Ipv4Address()
{
}

uint32_t Ipv4Address::ip() const
{
	return (uint32_t) ((this->_ip[0] << 24) + (this->_ip[1] << 16) +
	                   (this->_ip[2] <<  8) + (this->_ip[3] <<  0));
}

// static
unsigned int Ipv4Address::length()
{
	return 4;
}

std::string Ipv4Address::str()
{
	std::basic_ostringstream < char >ip_addr;

	ip_addr << (int) this->_ip[0] << "." << (int) this->_ip[1] << "."
	        << (int) this->_ip[2] << "." << (int) this->_ip[3];

	return ip_addr.str();
}

bool Ipv4Address::matchAddressWithMask(const IpAddress *addr,
                                       unsigned int mask) const
{
	uint32_t addr1, addr2;
	unsigned int i;
	uint32_t bitmask;

	if(addr->version() != 4 || mask > (Ipv4Address::length() * 8))
	{
		return false;
	}

	addr1 = this->ip();
	addr2 = ((Ipv4Address *) addr)->ip();

	bitmask = 0;
	for(i = 1; i <= mask; i++)
		bitmask |= 1 << (Ipv4Address::length() * 8 - i);

	addr1 &= bitmask;
	addr2 &= bitmask;

	return (addr1 == addr2);
}

int Ipv4Address::version() const
{
	return 4;
}

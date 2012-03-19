/**
 * @file Ipv4Address.cpp
 * @brief IPv4 address
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "Ipv4Address.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"
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

uint32_t Ipv4Address::ip()
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

bool Ipv4Address::matchAddressWithMask(IpAddress *addr,
                                       unsigned int mask)
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

int Ipv4Address::version()
{
	return 4;
}


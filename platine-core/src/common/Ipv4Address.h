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
	uint32_t ip();

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
	bool matchAddressWithMask(IpAddress *addr, unsigned int mask);
	int version();
};

#endif

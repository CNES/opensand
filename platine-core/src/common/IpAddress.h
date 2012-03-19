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
	virtual bool matchAddressWithMask(IpAddress *addr, unsigned int mask) = 0;

	/**
	 * Get the IP address version, that is 4 or 6
	 * @return version of the IP address
	 */
	virtual int version() = 0;
};

#endif

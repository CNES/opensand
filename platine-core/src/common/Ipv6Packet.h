/**
 * @file Ipv6Packet.h
 * @brief IPv6 packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IPV6_PACKET_H
#define IPV6_PACKET_H

#include <Data.h>
#include <IpPacket.h>
#include <Ipv6Address.h>


/**
 * @class Ipv6Packet
 * @brief IPv6 Packet
 */
class Ipv6Packet: public IpPacket
{
 public:

	/**
	 * Build an IPv6 packet
	 * @param data raw data from which an IPv6 packet can be created
	 * @param length length of raw data
	 */
	Ipv6Packet(unsigned char *data, unsigned int length);

	/**
	 * Build an IPv6 packet
	 * @param data raw data from which an IPv6 packet can be created
	 */
	Ipv6Packet(Data data);

	/**
	 * Build an empty IPv6 packet
	 */
	Ipv6Packet();

	/**
	 * Destroy the IPv6 packet
	 */
	~Ipv6Packet();

	// implementation of virtual functions
	bool isValid();
	uint16_t totalLength();
	uint16_t payloadLength();
	IpAddress *srcAddr();
	IpAddress *destAddr();
	uint8_t trafficClass();

	/**
	 * Create an IPv6 packet
	 *
	 * @param data  raw data from which an IPv6 packet can be created
	 * @return      the created IPv6 packet
	 */
	static NetPacket * create(Data data);

 protected:

	/**
	 * Retrieve the header length of the IPv6 packet
	 * @return the header length of the IPv6 packet
	 */
	static unsigned int headerLength();
};

#endif


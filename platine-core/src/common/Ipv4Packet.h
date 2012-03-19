/**
 * @file Ipv4Packet.h
 * @brief IPv4 packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef IPV4_PACKET_H
#define IPV4_PACKET_H

#include <Data.h>
#include <IpPacket.h>
#include <Ipv4Address.h>


/**
 * @class Ipv4Packet
 * @brief IPv4 Packet
 */
class Ipv4Packet: public IpPacket
{
 protected:

	/// Is the validity of the IPv4 packet already checked?
	bool validityChecked;
	/// If IPv4 packet validity is checked, what is the result?
	bool validityResult;

	/**
	 * Calculate the CRC of the IPv4 packet
	 * @return the CRC of the IPv4 packet
	 */
	uint16_t calcCrc();

 public:

	/**
	 * Build an IPv4 packet
	 * @param data raw data from which an IPv4 packet can be created
	 * @param length length of raw data
	 */
	Ipv4Packet(unsigned char *data, unsigned int length);

	/**
	 * Build an IPv4 packet
	 * @param data raw data from which an IPv4 packet can be created
	 */
	Ipv4Packet(Data data);

	/**
	 * Build an empty IPv4 packet
	 */
	Ipv4Packet();

	/**
	 * Destroy the IPv4 packet
	 */
	~Ipv4Packet();

	// implementation of virtual functions
	bool isValid();
	uint16_t totalLength();
	uint16_t payloadLength();
	IpAddress *destAddr();
	IpAddress *srcAddr();
	uint8_t trafficClass();

	/**
	 * Create an IPv4 packet
	 *
	 * @param data  raw data from which an IPv4 packet can be created
	 * @return      the created IPv4 packet
	 */
	static NetPacket * create(Data data);

 protected:

	/**
	 * Retrieve the CRC field of the IPv4 header
	 * @return the CRC field of the IPv4 header
	 */
	uint16_t crc();

	/**
	 * Retrieve the header length of the IPv4 packet
	 * @return the header length of the IPv4 packet
	 */
	uint8_t ihl();
};

#endif


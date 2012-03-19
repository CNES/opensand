/**
 * @file NetBurst.h
 * @brief Generic network burst
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef NET_BURST_H
#define NET_BURST_H

#include <list>
#include <string>

#include <Data.h>
#include <NetPacket.h>


/**
 * @class NetBurst
 * @brief Generic network burst
 */
class NetBurst: public std::list<NetPacket *>
{
 protected:

	/// The maximum number of network packets in the burst
	/// (0 for unlimited length)
	unsigned int max_packets;

 public:

	/**
	 * Build a network burst
	 *
	 * @param max_packets the maximum number of packets in the burst
	 *                    0 is the default value and means unlimited length
	 */
	NetBurst(unsigned int max_packets = 0);

	/**
	 * Destroy the network burst
	 */
	~NetBurst();

	/**
	 * Get the maximum number of network packets in the burst
	 *
	 * @return the maximum number of network packets in the burst
	 */
	int getMaxPackets();

	/**
	 * Set the maximum number of network packets in the burst
	 *
	 * @param max_packets the maximum number of packets in the burst
	 *                    0 means unlimited length
	 */
	void setMaxPackets(unsigned int max_packets);

	/**
	 * Add a packet in the network burst
	 *
	 * @param packet  the network packet to add
	 * @return        true if packet was successfully added (the burst was not
	 *                full), false otherwise
	 */
	bool add(NetPacket *packet);

	/**
	 * Is the network burst full?
	 *
	 * @return true if the burst is full (no space left for additional packets),
	 *         false otherwise
	 */
	bool isFull();

	/**
	 * Get the number of packets in the burst
	 *
	 * @return the number of packets in the burst
	 */
	unsigned int length();

	/**
	 * Get the raw content of the network burst, ie. the concatenation of
	 * all of the packets within the burst
	 *
	 * @return a string with the raw content of the burst
	 */
	Data data();

	/**
	 * Get the amount of data (in bytes) stored in the burst
	 *
	 * @return the amount of data in the burst
	 */
	long bytes();

	/**
	 * Get the type of packets stored in the burst (ATM, MPEG...)
	 * Packets in the burst must all have the same type.
	 *
	 * @return the type of packets in the burst
	 */
	uint16_t type();

	/**
	 * Get the name of packets stored in the burst (ATM, MPEG...)
	 * Packets in the burst must all have the same type.
	 *
	 * @return the name of packets in the burst
	 */
	std::string name();
};

#endif


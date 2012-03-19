/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file NetPacket.h
 * @brief Network-layer packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef NET_PACKET_H
#define NET_PACKET_H

#include <platine_margouilla/mgl_memorypool.h>
#include <Data.h>

#include <string>
#include <stdint.h>
#include <syslog.h>

// These values are greater than 1535 to avoid error
// with GSE in which a protocol type < 1536 indicates
// header extensions
// If protocol does not have an EtherType value, use
// values in interval [0x0601, 0x0659] which are
// unused EtherTypes values

/// Network protocol ID that indicates an error
#define NET_PROTO_ERROR 0x0000
/// Network protocol ID for ATM
#define NET_PROTO_ATM   0x0601
/// Network protocol ID for AAL5
#define NET_PROTO_AAL5  0x0602
/// Network protocol ID for MPEG-2 TS
// TODO when GSE library supports extensions,
// use the MPEG-2 TS-Concat Extension value
// as defined in RFC 5163 (§ 3.1)
#define NET_PROTO_MPEG  0x0603
/// Network protocol ID for ULE
#define NET_PROTO_ULE   0x0604
/// Network protocol ID for ROHC
#define NET_PROTO_ROHC  0x0605
/// Network protocol ID for DVB frame
#define NET_PROTO_DVB_FRAME  0x0606
/// Network protocol ID for GSE
#define NET_PROTO_GSE   0x0607
/// Network protocol ID for IPv4
#define NET_PROTO_IPV4  0x0800
/// Network protocol ID for IPv6
#define NET_PROTO_IPV6  0x86dd


/**
 * @class NetPacket
 * @brief Network-layer packet
 */
class NetPacket
{
 protected:

	/// Internal buffer for packet data
	Data data;

	/// The name of the network protocol
	std::string name;
	/// The type of network protocol
	uint16_t type;
	/// The packet QoS
	uint8_t qos;
	/// The packet source TalId
	uint8_t src_tal_id;
	/// The packet destination TalID
	uint8_t dst_tal_id;
	/// The destination spot ID
	uint8_t dst_spot;
	/// The packet header length
	size_t header_length;

	/// Pool of memory for network packets
	static mgl_memory_pool mempool;

 public:

	void *operator new(size_t size) throw();
	void *operator new[](size_t size) throw();
	void operator delete(void *p) throw();
	void operator delete[](void *p) throw();

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetPacket(unsigned char *data, size_t length);


	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 */
	NetPacket(Data data);

	/**
	 * Build an empty network-layer packet
	 */
	NetPacket();

 public:
	
	/**
	 * Build a network-layer packet initialized
	 * @param data              raw data from which a network-layer packet can be created
	 * @param length            length of raw data
	 * @param name              the name of the network protocol
	 * @param type              the type of the network protocol
	 * @param qos               the QoS value to associate with the packet
	 * @param src_tal_id        the source terminal ID to associate with the packet
	 * @param dst_tal_id        the destination terminal ID to associate with the packet
	 * @param header_length     the header length of the packet
	 */
	NetPacket(unsigned char *data,
	          size_t length,
	          std::string name,
	          uint16_t type,
	          uint8_t qos,
	          uint8_t src_tal_id,
	          uint8_t dst_tal_id,
	          size_t header_length);

	/**
	 * Destroy the network-layer packet
	 */
	virtual ~NetPacket();

	/**
	 * Add trace in memory pool in order to debug memory allocation
	 */
	// TODO use it in plugins !
	void addTrace(std::string name_function);

	/**
	 * Get the QoS associated with the packet
	 * @return the QoS associated with the packet
	 */
	virtual uint8_t getQos();

	/**
	 * Get the source TAL id associated with the packet
	 * @return the source TAL id associated with the packet
	 */
	virtual uint8_t getSrcTalId();

	/**
	 * Get the destination TAL id associated with the packet
	 * @return the destination TAL id associated with the packet
	 */
	virtual uint8_t getDstTalId();

	/**
	 * Get the name of the network protocol
	 * @return the name of the network protocol
	 */
	std::string getName();

	/**
	 * Get the type of network protocol
	 * @return the type of network protocol
	 */
	uint16_t getType();

	/**
	 * Retrieve the total length of the packet
	 * @return the total length of the packet
	 */
	virtual uint16_t getTotalLength();

	/**
	 * Get a copy of the raw packet data
	 * @return a copy of the raw packet data
	 */
	Data getData();

	/**
	 * Retrieve the length of the packet payload
	 * @return the length of the packet payload
	 */
	virtual uint16_t getPayloadLength();

	/**
	 * Retrieve the payload of the packet
	 * @return the payload of the packet
	 */
	virtual Data getPayload();

	/**
	 * Set the destination spot ID
	 *
	 * @param spot_id  The destination spot id
	 */
	void setDstSpot(uint8_t spot_id);

	/**
	 * Get the destination spot ID
	 *
	 * @return the destinatioj spot ID
	 */
	uint8_t getDstSpot();

};

#endif

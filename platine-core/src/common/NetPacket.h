/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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

#include "config.h"

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
/// Network protocol ID for MPEG2-TS
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
	Data _data;

	/// The name of the network protocol
	std::string _name;
	/// The type of network protocol
	uint16_t _type;

	/// Pool of memory for network packets
	static mgl_memory_pool mempool;

 public:

#if MEMORY_POOL
	inline void *operator new(size_t size) throw()
	{
		if((int) size > NetPacket::mempool._memBlocSize)
		{
			syslog(LOG_ERR, "too much memory asked: %u bytes "
			       "while only %ld is available", size,
			       NetPacket::mempool._memBlocSize);
			return NULL;
		}
		else
		{
			return NetPacket::mempool.get("NetPacket::new", size);
		}
	}

	inline void *operator new[](size_t size) throw()
	{
		if((int) size > NetPacket::mempool._memBlocSize)
		{
			syslog(LOG_ERR, "too much memory asked: %u bytes "
			       "while only %ld is available", size,
			       NetPacket::mempool._memBlocSize);
			return NULL;
		}
		else
		{
			return NetPacket::mempool.get("NetPacket::new[]", size);
		}
	}

	inline void operator delete(void *p) throw()
	{
		mempool.release((char *) p);
	}

	inline void operator delete[](void *p) throw()
	{
		mempool.release((char *) p);
	}
#endif


 public:

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetPacket(unsigned char *data, unsigned int length);

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 */
	NetPacket(Data data);

	/**
	 * Build an empty network-layer packet
	 */
	NetPacket();

	/**
	 * Destroy the network-layer packet
	 */
	virtual ~NetPacket();

	/**
	 * Add trace in memory pool in order to debug memory allocation
	 */
	void addTrace(std::string name_function);

	/**
	 * Is the network-layer packet a valid one?
	 * @return true if network-layer packet is valid, false otherwise
	 */
	virtual bool isValid() = 0;

	/**
	 * Get the QoS associated with the packet
	 * @return the QoS associated with the packet
	 */
	virtual int qos() = 0;

	/**
	 * Associate a QoS with the packet
	 * @param qos the QoS to associate with the packet
	 */
	virtual void setQos(int qos) = 0;

	/**
	 * Get the MAC id associated with the packet
	 * @return the MAC id associated with the packet
	 */
	virtual unsigned long macId() = 0;

	/**
	 * Associate a MAC id with the packet
	 * @param macId the MAC id to associate with packet
	 */
	virtual void setMacId(unsigned long macId) = 0;

	/**
	 * Get the TAL id associated with the packet
	 * @return the TAL id associated with the packet
	 */
	virtual long talId() = 0;

	/**
	 * Associate a TAL id with the packet
	 * @param talId the TAL id to associate with packet
	 */
	virtual void setTalId(long talId) = 0;

	/**
	 * Get the name of the network protocol
	 * @return the name of the network protocol
	 */
	std::string name();

	/**
	 * Get the type of network protocol
	 * @return the type of network protocol
	 */
	uint16_t type();

	/**
	 * Retrieve the total length of the packet
	 * @return the total length of the packet
	 */
	virtual uint16_t totalLength() = 0;

	/**
	 * Get a copy of the raw packet data
	 * @return a copy of the raw packet data
	 */
	Data data();

	/**
	 * Retrieve the length of the packet payload
	 * @return the length of the packet payload
	 */
	virtual uint16_t payloadLength() = 0;

	/**
	 * Retrieve the payload of the packet
	 * @return the payload of the packet
	 */
	virtual Data payload() = 0;

	/**
	 * Set a packet type (only used for GSE with ROHC)
	 */
	virtual void setType(uint16_t type);

};

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file NetPacket.h
 * @brief Network-layer packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef NET_PACKET_H
#define NET_PACKET_H

#include "Data.h"
#include "NetContainer.h"

#include <opensand_output/OutputLog.h>
#include <linux/if_ether.h>

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
#define NET_PROTO_ERROR   0x0000
/// Network protocol ID for ATM
#define NET_PROTO_ATM     0x0601
/// Network protocol ID for AAL5
#define NET_PROTO_AAL5    0x0602
/// Network protocol ID for MPEG-2 TS
// TODO when GSE library supports extensions,
// use the MPEG-2 TS-Concat Extension value
// as defined in RFC 5163 (§ 3.1)
#define NET_PROTO_MPEG    0x0603
/// Network protocol ID for ULE
#define NET_PROTO_ULE     0x0604
/// Network protocol ID for ROHC
#define NET_PROTO_ROHC    0x0605
/// Network protocol ID for DVB frame
//#define NET_PROTO_DVB_FRAME  0x0606
/// Network protocol ID for GSE
#define NET_PROTO_GSE     0x0607
/// Network protocol ID for both IP v4 or v6
#define NET_PROTO_IP      0x0608
/// Network protocol ID for Ethernet
#define NET_PROTO_ETH     0x0609
/// Network protocol ID for PHS
#define NET_PROTO_PHS     0x060A
/// Network protocol ID for IPv4
#define NET_PROTO_IPV4    ETH_P_IP
/// Network protocol ID for IPv6
#define NET_PROTO_IPV6    ETH_P_IPV6
/// Network protocol ID for 802.1Q (VLAN)
#define NET_PROTO_802_1Q  ETH_P_8021Q
/// Network protocol ID for 802.1ad (Q in Q)
#define NET_PROTO_802_1AD 0x9100
// ARP ethertype
#define NET_PROTO_ARP     ETH_P_ARP


// TODO we may add an option to enable jumbo frames

// Size of a IEEE 802.3 Ethernet frame
// dmac(6) + smac(6) + etype(2) + max_payload(1500) = 1514 bytes
#define ETHERNET_2_SIZE			ETH_FRAME_LEN
#define ETHERNET_2_HEADSIZE		ETH_HLEN
// Size of a IEEE 802.1q Ethernet frame
// dmac(6) + smac(6) + 8100(2) + vlan/Qos(2) + 
// etype(2) + max_payload(1500) = 1518 bytes
#define ETHERNET_802_1Q_SIZE 1518
#define ETHERNET_802_1Q_HEADSIZE 18
// Size of a IEEE 802.1ad Ethernet frame
// dmac(6) + smac(6) + 9100(2) + outer vlan/Qos(2) + 8100(2) +
// inner vlan/Qos(2) + etype(2) + max_payload(1500) = 1522 bytes
#define ETHERNET_802_1AD_SIZE 1522
#define ETHERNET_802_1AD_HEADSIZE 22

#define MAX_ETHERNET_SIZE ETHERNET_802_1AD_SIZE


/**
 * @class NetPacket
 * @brief Network-layer packet
 */
class NetPacket: public NetContainer
{
 protected:

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

 public:
#if 0

// call new in Rt
	void *operator new(size_t size) throw();
	void *operator new[](size_t size) throw();
	void operator delete(void *p) throw();
	void operator delete[](void *p) throw();
#endif

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetPacket(const unsigned char *data, size_t length);

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 */
	NetPacket(const Data &data);

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetPacket(const Data &data, size_t length);

	/**
	 * Build an empty network-layer packet
	 */
	NetPacket();

	/**
	 * Build a network-layer packet initialized
	 *
	 * @param data              raw data from which a network-layer packet can be created
	 * @param length            length of raw data
	 * @param name              the name of the network protocol
	 * @param type              the type of the network protocol
	 * @param qos               the QoS value to associate with the packet
	 * @param src_tal_id        the source terminal ID to associate with the packet
	 * @param dst_tal_id        the destination terminal ID to associate with the packet
	 * @param header_length     the header length of the packet
	 */
	NetPacket(const Data &data,
	          size_t length,
	          string name,
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
	 * Set the QoS associated with the packet
	 *
	 * @param qos  the QoS associated with the packet
	 */
	virtual void setQos(uint8_t qos);

	/**
	 * Get the QoS associated with the packet
	 *
	 * @return the QoS associated with the packet
	 */
	virtual uint8_t getQos() const;

	/**
	 * Set the source TAL id associated with the packet
	 *
	 * @param tal_id  the source TAL id associated with the packet
	 */
	virtual void setSrcTalId(uint8_t tal_id);

	/**
	 * Get the source TAL id associated with the packet
	 *
	 * @return the source TAL id associated with the packet
	 */
	virtual uint8_t getSrcTalId() const;

	/**
	 * Set the destination TAL id associated with the packet
	 *
	 * @param tal_id  the destination TAL id associated with the packet
	 */
	virtual void setDstTalId(uint8_t tal_id);

	/**
	 * Get the destination TAL id associated with the packet
	 *
	 * @return the destination TAL id associated with the packet
	 */
	virtual uint8_t getDstTalId() const;

	/**
	 * Get the type of network protocol
	 *
	 * @return the type of network protocol
	 */
	uint16_t getType() const;

	/**
	 * Set the destination spot ID
	 *
	 * @param spot_id  The destination spot id
	 */
	void setDstSpot(uint8_t spot_id);

	/**
	 * Get the destination spot ID
	 *
	 * @return the destination spot ID
	 */
	uint8_t getDstSpot() const;

};

#endif

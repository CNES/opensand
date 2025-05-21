/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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

#include <linux/if_ether.h>
#include <map>
#include "NetContainer.h"

// These values are greater than 1535 to avoid error
// with GSE in which a protocol type < 1536 indicates
// header extensions
// If protocol does not have an EtherType value, use
// values in interval [0x0601, 0x0659] which are
// unused EtherTypes values

enum class NET_PROTO : uint16_t
{
	/// Network protocol ID that indicates an error
	ERROR = 0x0000,
	// CNI extension for GSE
	GSE_EXTENSION_CNI = 0x00FF,
	/// Network protocol ID for ATM
	ATM = 0x0601,
	/// Network protocol ID for AAL5
	AAL5 = 0x0602,
	/// Network protocol ID for MPEG-2 TS
	// TODO when GSE library supports extensions,
	// use the MPEG-2 TS-Concat Extension value
	// as defined in RFC 5163 (§ 3.1)
	MPEG = 0x0603,
	/// Network protocol ID for ULE
	ULE = 0x0604,
	/// Network protocol ID for ROHC
	ROHC = 0x0605,
	/// Network protocol ID for DVB frame
	// DVB_FRAME = 0x0606,
	/// Network protocol ID for GSE
	GSE = 0x0607,
	/// Network protocol ID for both IP v4 or v6
	IP = 0x0608,
	/// Network protocol ID for Ethernet
	ETH = 0x0609,
	/// Network protocol ID for PHS
	PHS = 0x060A,
	/// Network protocol ID for RLE
	RLE = 0x060B,
	/// Network protocol ID for IPv4
	IPV4 = ETH_P_IP,
	// ARP ethertype
	ARP = ETH_P_ARP,
	/// Network protocol ID for 802.1Q (VLAN)
	IEEE_802_1Q = ETH_P_8021Q,
	/// Network protocol ID for IPv6
	IPV6 = ETH_P_IPV6,
	/// Network protocol ID for 802.1ad (Q in Q)
	IEEE_802_1AD = 0x9100,
};

// TODO we may add an option to enable jumbo frames

// Size of a IEEE 802.3 Ethernet frame
// dmac(6) + smac(6) + etype(2) + max_payload(1500) = 1514 bytes
#define ETHERNET_2_SIZE ETH_FRAME_LEN
#define ETHERNET_2_HEADSIZE ETH_HLEN
// Size of a IEEE 802.1q Ethernet frame
// dmac(6) + smac(6) + 8100(2) + vlan/Qos(2) +
// etype(2) + max_payload(1500) = 1518 bytes
// #define ETHERNET_802_1Q_SIZE 1518
#define ETHERNET_802_1Q_SIZE 5018
#define ETHERNET_802_1Q_HEADSIZE 18
// Size of a IEEE 802.1ad Ethernet frame
// dmac(6) + smac(6) + 9100(2) + outer vlan/Qos(2) + 8100(2) +
// inner vlan/Qos(2) + etype(2) + max_payload(1500) = 1522 bytes

// #define ETHERNET_802_1AD_SIZE 1522
//  changer pour tester la désencap
#define ETHERNET_802_1AD_SIZE 5022
#define ETHERNET_802_1AD_HEADSIZE 22

#define MAX_ETHERNET_SIZE ETHERNET_802_1AD_SIZE

/**
 * @class NetPacket
 * @brief Network-layer packet
 */
class NetPacket : public NetContainer
{
protected:
	/// The type of network protocol
	NET_PROTO type;
	/// The packet QoS
	uint8_t qos;
	/// The packet source TalId
	uint8_t src_tal_id;
	/// The packet destination TalID
	uint8_t dst_tal_id;

	// the packet extension header if required
	// used by GSE protocol (Rust)
	std::map<uint16_t, Rt::Data> header_extensions;

public:
	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetPacket(const unsigned char *data, std::size_t length);

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 */
	NetPacket(const Rt::Data &data);

	/**
	 * Build a network-layer packet
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetPacket(const Rt::Data &data, std::size_t length);

	/**
	 * Build a network-layer packet
	 * @param pkt
	 */
	NetPacket(const NetPacket &pkt);

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
	NetPacket(const Rt::Data &data,
			  std::size_t length,
			  std::string name,
			  NET_PROTO type,
			  uint8_t qos,
			  uint8_t src_tal_id,
			  uint8_t dst_tal_id,
			  std::size_t header_length);

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
	NetPacket(const uint8_t *data,
			  std::size_t length,
			  std::string name,
			  NET_PROTO type,
			  uint8_t qos,
			  uint8_t src_tal_id,
			  uint8_t dst_tal_id,
			  std::size_t header_length);

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
	NET_PROTO getType() const;

	/**
	 * Adds an extension header to the packet with the specified ID and data.
	 *
	 * @param ext_id The identifier for the extension header.
	 * @param ext_data The data associated with the extension header.
	 * @return True if the extension header was successfully added, false if an extension with the same ID already exists.
	 */
	bool addExtensionHeader(uint16_t ext_id, Rt::Data &ext_data);


	/**
	 * Retrieves all extension header IDs stored in the packet.
	 *
	 * @return A vector containing all the extension header IDs.
	 */
	std::vector<uint16_t> getAllExtensionHeadersId() const;


	/**
	 * Retrieves the data associated with a specific extension header ID.
	 *
	 * @param ext_id The identifier for the extension header.
	 * @return The data associated with the specified extension header ID.
	 *         Throws an exception if the extension header ID does not exist.
	 */
	Rt::Data getExtensionHeaderValueById(uint16_t ext_id);
};

#endif

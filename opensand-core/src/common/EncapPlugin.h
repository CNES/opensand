/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
 * Copyright © 2019 TAS
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
 * @file EncapPlugin.h
 * @brief Generic encapsulation / deencapsulation plugin
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef ENCAP_PLUGIN_H
#define ENCAP_PLUGIN_H

#include <map>
#include <list>
#include <iostream>
#include <map>
#include <vector>

#include <opensand_rt/Ptr.h>
#include <opensand_rt/Data.h>

#include "OpenSandCore.h"

#include "OpenSandPlugin.h"


class NetPacket;
class NetBurst;
class OutputLog;
class NetContainer;
enum class NET_PROTO : uint16_t;

/**
 * @class SimpleSimpleEncapPlugin
 * @brief Generic encapsulation / deencapsulation plugin
 */
class EncapPlugin: public OpenSandPlugin
{
protected:
	uint8_t dst_tal_id; // used to filter packets

public:
	EncapPlugin(NET_PROTO ether_type);
	virtual ~EncapPlugin() = default;

	void setFilterTalId(uint8_t tal_id);

	/**
	 * @brief Get the source terminal ID of a packet
	 *
	 * @param data    The packet content
	 * @param tal_id  OUT: the source terminal ID of the packet
	 * @return true on success, false otherwise
	 */
	virtual bool getSrc(const Rt::Data &data, tal_id_t &tal_id) const = 0;

	/**
	 * @brief Get the QoS of a packet
	 *
	 * @param data   The packet content
	 * @param qos    OUT: the QoS of the packet
	 * @return true on success, false otherwise
	 */
	virtual bool getQos(const Rt::Data &data, qos_t &qos) const = 0; // only for Rle

	/**
	 * @brief Encapsulate the packet and store unencapsulable part
	 *
	 * @param[in]  packet            The packet to encapsulate
	 * @param[in]  remaining_length  The remaining length
	 * @param[in]  new_burst         The new burst status
	 * @param[out] partial_encap     The status about encapsulation
	 *                               (true if data remains after encapsulation,
	 *                               false otherwise)
	 * @param[out] encap_packet      The encapsulated packet (null in error case)
	 *
	 * @return  true if success, false otherwise
	 */
	bool virtual encapNextPacket(Rt::Ptr<NetPacket> packet,
								 std::size_t remaining_length,
								 bool new_burst,
								 Rt::Ptr<NetPacket> &encap_packet,
								 Rt::Ptr<NetPacket> &remaining_data) = 0;

	/**
	 * @brief Decapsulate packets from NetContainer.
	 * @details  Decapsulate @ref decap_packets_count from the @ref packet.
	 * Packet decapsulated are pushed to @ref decap_packets.
	 * @warning @ref decap_packets can be empty if there are only fragments and/or padding packets.
	 * @warning Rle ignore the @ref decap_packets_count
	 * @param[in]  packet             The packet storing payload
	 * @param[out] decap_packets      The list of decapsulated packet
	 * @param[in] decap_packets_count  The packet count to decapsulate (0 if unknown)
	 */
	bool virtual decapAllPackets(Rt::Ptr<NetContainer> encap_packets,
								 std::vector<Rt::Ptr<NetPacket>> &decap_packets,
								 unsigned int decap_packet_count = 0) = 0;

	/**
	 * @brief Create a NetPacket from data with the relevant attributes.
	 *
	 * @param data        The packet data
	 * @param data_length The packet length
	 * @param qos         The QoS value to associate with the packet
	 * @param src_tal_id  The source terminal ID to associate with the packet
	 * @param dst_tal_id  The destination terminal ID to associate with the packet
	 *
	 * @return The packet
	 */

	// only use by SlottedAloha ...
	virtual Rt::Ptr<NetPacket> build(const Rt::Data &data,
									 std::size_t data_length,
									 uint8_t qos,
									 uint8_t src_tal_id,
									 uint8_t dst_tal_id) = 0;

	virtual bool setHeaderExtensions(Rt::Ptr<NetPacket> packet,
									 Rt::Ptr<NetPacket> &new_packet,
									 tal_id_t tal_id_src,
									 tal_id_t tal_id_dst,
									 std::string callback_name,
									 void *opaque) = 0;

	virtual bool getHeaderExtensions(const Rt::Ptr<NetPacket> &packet,
									 std::string callback_name,
									 void *opaque) = 0;

public:
	/**
	 * @brief Get the EtherType associated with the encapsulation protocol
	 *
	 * @return The EtherType
	 */
	NET_PROTO getEtherType() const;

protected:
	NET_PROTO ether_type;
	std::shared_ptr<OutputLog> log;
};

#endif // ENCAP_PLUGIN_H

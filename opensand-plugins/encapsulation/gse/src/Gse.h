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
 * @file Gse.h
 * @brief Gse encapsulation plugin implementation
 * @author Axel Pinel <axel.pinel@viveris.fr>
 */

#ifndef GseRust_CONTEXT_H
#define GseRust_CONTEXT_H

#include <map>
#include <string>
#include <vector>

#include <EncapPlugin.h>
#include "GseIdentifier.h"

#include "Memory.h"
#include "GseApi.h"

class NetPacket;
class NetBurst;

/**
 * @class Gse
 * @brief Gse encapsulation plugin implementation
 */
class Gse : public EncapPlugin
{
private:
	/**
	 * @brief Decapsulate the packet using Gse (Rust) librairy
	 * @param data  IN: the packet to decapsulate
	 * @param size_t  OUT: the length read from the packet
	 * @return the packet decapsulated
	 */
	Rt::Ptr<NetPacket> decapNextPacket(const Rt::Data &data,
									   size_t &length_decap);

	/**
	 * @brief Map of encapsulation context
	 * @details when a packet is fragmented, the state of the fragmentation is saved
	 * in this map. @ref GetChunk()
	 */
	std::map<GseIdentifier, RustContextFrag, ltGseIdentifier> contexts;

	/**
	 * @brief Memory buffer to save fragment during desencapsulation
	 * @details Memory is created by the plugin but used by the crate
	 * (given in parameter during the creation of the rust deencapsulator object).
	 * @ref decap_and_build()
	 */
	c_memory decap_buffer;

	/**
	 * @brief OpaquePointer to the Rust Encapsulator Object.
	 * This attribute should be given as parameter to
	 * the encapsulation function from the DVB GSE crate.
	 */
	OpaquePtrEncap *rust_encapsulator;

	/**
	 * @brief OpaquePointer to the Rust Decapsulator Object.
	 * This attribute should be given as parameter to
	 * the deencapsulation function from the DVB GSE crate.
	 */
	OpaquePtrDecap *rust_decapsulator;

	/**
	 * @brief Force compatibility with the DVB GSE library written in C language, if true
	 * @details Avoid to use the label ReUse and force the usage of SixBytes Label because
	 * the C library doesn't support them
	 */
	bool force_compatibility;

	uint8_t max_reuse;

public:
	/**
	 * @brief Decapsulate all the packet using @ref decapNextPacket and push the packet to the std::vector
	 * @param encap_packets  IN: the packets to decapsulate
	 * @param decap_packet_count IN: the number of packet to decapsulate in the NetContainer
	 * @param decap_packets  OUT: the vector of decapsulated packet
	 */

	bool decapAllPackets(Rt::Ptr<NetContainer> encap_packets,
						 std::vector<Rt::Ptr<NetPacket>> &decap_packets,
						 unsigned int decap_packet_count = 0) override;

	/**
	 * @warning This method should never be called. If called, it will abort.
	 */
	Rt::Ptr<NetPacket> build(const Rt::Data &data,
							 std::size_t data_length,
							 uint8_t qos,
							 uint8_t src_tal_id,
							 uint8_t dst_tal_id) override;

	/**
	 * @warning This method should never be called. If called, it will abort.
	 */
	bool getSrc(const Rt::Data &data, tal_id_t &tal_id) const override;

	/**
	 * @warning This method should never be called. If called, it will abort.
	 */
	bool getQos(const Rt::Data &data, qos_t &qos) const override;

	bool encapNextPacket(Rt::Ptr<NetPacket> packet,
						 std::size_t remaining_length,
						 bool new_burst,
						 Rt::Ptr<NetPacket> &encap_packet,
						 Rt::Ptr<NetPacket> &remaining_data) override;

	bool setHeaderExtensions(Rt::Ptr<NetPacket> packet,
							 Rt::Ptr<NetPacket> &new_packet,
							 tal_id_t tal_id_src,
							 tal_id_t tal_id_dst,
							 std::string callback_name,
							 void *opaque) override;

	bool getHeaderExtensions(const Rt::Ptr<NetPacket> &packet,
							 std::string callback_name,
							 void *opaque) override;

public:
	Gse();
	~Gse();

	/**
	 * @brief Generate the configuration for the plugin
	 */
	static void generateConfiguration(const std::string &parent_path,
									  const std::string &param_id,
									  const std::string &plugin_name);

	// Static methods: getter/setter for label/fragId

	/**
	 * @brief  Set the Gse packet label
	 *
	 * @param   packet  The packet to get label values from.
	 * @param   label   The label to set values of.
	 * @return  true on success, false otherwise.
	 */
	static bool setLabel(const NetPacket &packet, uint8_t label[]);

	/**
	 * @brief   Get the source TAL Id from label.
	 * @param   label  The label to read value from.
	 * @return  the source TAL Id.
	 */
	static uint8_t getSrcTalIdFromLabel(const uint8_t label[]);

	/**
	 * @brief   Get the destination TAL Id from label.
	 * @param   label  The label to read value from.
	 * @return  the destination TAL Id.
	 */
	static uint8_t getDstTalIdFromLabel(const uint8_t label[]);

	/**
	 * @brief   Get the QoS value from label.
	 * @param   label  The label to read value from.
	 * @return  the QoS value.
	 */
	static uint8_t getQosFromLabel(const uint8_t label[]);

	/**
	 * @brief   Create a fragment id from a packet.
	 * @param   packet  The packet to create the frag id from..
	 * @return  the frag id.
	 */
	static uint8_t getFragId(const NetPacket &packet);

	/**
	 * @brief   Get the source TAL Id from a fragment id..
	 * @param   frag_id  The fragment dd to read value from.
	 * @return  the source TAL Id.
	 */
	static uint8_t getSrcTalIdFromFragId(const uint8_t frag_id);

	/**
	 * @brief   Get the QoS value from a fragment id..
	 * @param   frag_id  The fragment dd to read value from.
	 * @return  the QoS value.
	 */
	static uint8_t getQosFromFragId(const uint8_t frag_id);
};

CREATE(Gse, PluginType::Encapsulation, "GSE");

#endif

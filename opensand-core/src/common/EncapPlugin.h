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

#ifndef ENCAP_CONTEXT_H
#define ENCAP_CONTEXT_H


#include <map>
#include <list>

#include <opensand_rt/Ptr.h>

#include "StackPlugin.h"


class NetPacket;
class NetBurst;
class OutputLog;


/**
 * @class EncapPlugin
 * @brief Generic encapsulation / deencapsulation plugin
 */
class EncapPlugin: public StackPlugin
{
public:
	EncapPlugin(NET_PROTO ether_type);

	/**
	 * @class EncapPacketHandler
	 * @brief Functions to handle the encapsulated packets
	 */
	class EncapPacketHandler: public StackPacketHandler
	{
	public:
		/**
		 * @brief EncapPacketHandler constructor
		 */
		/* Allow packets to access EncapPlugin members */
		EncapPacketHandler(EncapPlugin &pl);

		virtual ~EncapPacketHandler();

		/**
		 * @brief Get the source terminal ID of a packet
		 *
		 * @param data    The packet content
		 * @param tal_id  OUT: the source terminal ID of the packet
		 * @return true on success, false otherwise
		 */
		virtual bool getSrc(const Rt::Data &data, tal_id_t &tal_id) const = 0;

		/**
		 * @brief Get the destination terminal ID of a packet
		 *
		 * @param data    The packet content
		 * @param tal_id  OUT: the data terminal ID of the packet
		 * @return true on success, false otherwise
		 */
		virtual bool getDst(const Rt::Data &data, tal_id_t &tal_id) const = 0;

		/**
		 * @brief Get the QoS of a packet
		 *
		 * @param data   The packet content
		 * @param qos    OUT: the QoS of the packet
		 * @return true on success, false otherwise
		 */
		virtual bool getQos(const Rt::Data &data, qos_t &qos) const = 0;

		virtual bool init();

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
		bool encapNextPacket(Rt::Ptr<NetPacket> packet,
		                     std::size_t remaining_length,
		                     bool new_burst,
		                     Rt::Ptr<NetPacket> &encap_packet,
		                     Rt::Ptr<NetPacket> &remaining_data) override;

		/**
		 * @brief Get encapsulated packet from payload
		 *
		 * @param[in]  packet             The packet storing payload
		 * @param[out] partial_decap      The status about decapsulation (true if data
		 *                                is incomplete to decapsulation, false otherwise)
		 * @param[out] decap_packets      The list of decapsulated packet
		 * @param[in decap_packets_count  The packet count to decapsulate (0 if unknown)
		 */
		bool getEncapsulatedPackets(Rt::Ptr<NetContainer> packet,
		                            bool &partial_decap,
		                            std::vector<Rt::Ptr<NetPacket>> &decap_packets,
		                            unsigned int decap_packet_count=0) override;

		virtual bool checkPacketForHeaderExtensions(Rt::Ptr<NetPacket> &packet) = 0;

		virtual bool setHeaderExtensions(Rt::Ptr<NetPacket> packet,
		                                 Rt::Ptr<NetPacket>& new_packet,
		                                 tal_id_t tal_id_src,
		                                 tal_id_t tal_id_dst,
		                                 std::string callback_name,
		                                 void *opaque) = 0;

		virtual bool getHeaderExtensions(const Rt::Ptr<NetPacket>& packet,
		                                 std::string callback_name,
		                                 void *opaque) = 0;

	protected:
		/**
		 * @brief get a NetPacket that can be encapsulated in the frame
		 *
		 * There is 4 use case:
		 *     1. the whole packet can be encapsulated
		 *     2. the packet should be fragmented to be encapsulated
		 *     3. the packet cannot be encapsulated, even fragmented
		 *     4. an error occured
		 *
		 * @param packet            IN: The initial NetPacket, it should be
		 *                              deleted in this function
		 * @param remaining_length  The maximum length that can be encapsulated
		 *                          in the frame
		 * @param data              OUT: The NetPacket that can be encapsulated
		 *                               (case 1, 2)
		 *                               NULL case (3, 4)
		 * @param remaining_data    OUT: The part of the initial packet that
		 *                               cannot be encapsulated in the current
		 *                               frame (case 2, 3)
		 *                               NULL (case 1, 4)
		 * @return true on success (case 1, 2, 3), false otherwise (case 4)
		 */
		virtual bool getChunk(Rt::Ptr<NetPacket> packet,
		                      std::size_t remaining_length,
		                      Rt::Ptr<NetPacket>& data,
		                      Rt::Ptr<NetPacket>& remaining_data) const = 0;

		/// Output Logs
		std::shared_ptr<OutputLog> log;

		/// map call back name
		std::list<std::string> callback_name;
	};

	/**
	 * @class EncapContext
	 * @brief The encapsulation/deencapsulation context
	 */
	class EncapContext: public StackContext
	{
	public:
		/* Allow context to access EncapPlugin members */
		/**
		 * @brief EncapContext constructor
		 */
		EncapContext(EncapPlugin &pl);

		/**
		 * Flush the encapsulation context identified by context_id (after a context
		 * expiration for example).
		 *
		 * @param context_id  the context to flush
		 * @return            a list of encapsulation packets
		 */
		// TODO replace int by uintXX_t
		virtual Rt::Ptr<NetBurst> flush(int context_id) = 0;

		/**
		 * Flush all the encapsulation contexts. It's the caller charge to delete
		 * the returned NetBurst after use.
		 *
		 * @return  a list of encapsulation packets
		 */
		virtual Rt::Ptr<NetBurst> flushAll() = 0;

		/**
		 * @brief Set the filter on destination TAL Id.
		 *
		 * @param tal_id  The destination TAL Id.
		 */
		void setFilterTalId(uint8_t tal_id);

		virtual bool init();

	protected:
		/// The destination TAL Id to filter received packet on
		uint8_t dst_tal_id;

		/// Output Logs
		std::shared_ptr<OutputLog> log;
	};

	virtual bool init();

	/* for the following functions we use "covariant return type" */

	/**
	 * @brief Get the context
	 *
	 * @return the context
	 */
	inline std::shared_ptr<EncapContext> getContext() const
	{
		return std::static_pointer_cast<EncapContext>(this->context);
	};

	/**
	 * @brief Get the packet handler
	 *
	 * @return the packet handler
	 */
	inline std::shared_ptr<EncapPacketHandler> getPacketHandler() const
	{
		return std::static_pointer_cast<EncapPacketHandler>(this->packet_handler);
	};

};


typedef std::vector<std::shared_ptr<EncapPlugin::EncapContext>> encap_contexts_t;


#ifdef CREATE
#undef CREATE
#define CREATE(CLASS, CONTEXT, HANDLER, pl_name) \
	CREATE_STACK(CLASS, CONTEXT, HANDLER, pl_name, PluginType::Encapsulation)
#endif


#endif

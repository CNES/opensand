/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 CNES
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
 */

#ifndef ENCAP_CONTEXT_H
#define ENCAP_CONTEXT_H

#include <map>
#include <string>

#include "NetPacket.h"
#include "NetBurst.h"
#include "OpenSandCore.h"
#include "StackPlugin.h"

#include <opensand_output/Output.h>




/**
 * @class EncapPlugin
 * @brief Generic encapsulation / deencapsulation plugin
 */
class EncapPlugin: public StackPlugin
{

 public:

	EncapPlugin(uint16_t ether_type): StackPlugin(ether_type)
	{
	};

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
		EncapPacketHandler(EncapPlugin &pl):
			StackPacketHandler(pl)
		{
		};

		virtual ~EncapPacketHandler();

		/**
		 * @brief Get the source terminal ID of a packet
		 *
		 * @param data    The packet content
		 * @param tal_id  OUT: the source terminal ID of the packet
		 * @return true on success, false otherwise
		 */
		virtual bool getSrc(const Data &data, tal_id_t &tal_id) const = 0;

		/**
		 * @brief Get the QoS of a packet
		 *
		 * @param data   The packet content
		 * @param qos    OUT: the QoS of the packet
		 * @return true on success, false otherwise
		 */
		virtual bool getQos(const Data &data, qos_t &qos) const = 0;

		virtual void init()
		{
			this->log = Output::registerLog(LEVEL_WARNING,
			                                "Encap.%s",
			                                this->getName().c_str());
		};

		/**
		 * @brief Encapsulate the packet and store unencapsulable part
		 * 
		 * @param[in]   The packet to encapsulate
		 * @param[in]   The remaining length
		 * @param[out]  The status about encapsulation (true if data remains after encapsulation, false otherwise)
		 * @param[out]  The encapsulated packet (null in error case)
		 * 
		 * @return  true if success, false otherwise
		 */
		virtual bool encapNextPacket(NetPacket *packet,
			size_t remaining_length,
			bool &partial_encap,
			NetPacket **encap_packet);

		/**
		 * @brief Reset remaining data of the packet after encapsulation
		 *
		 * @param[in]   The packet to reset remaining data (if NULL, all packet
		 *              will be reset)
		 * 
		 * @return  true if success, false otherwise
		 */
		virtual bool resetPacketToEncap(NetPacket *packet = NULL);

		/**
		 * @brief Decapsulate a packet or store it if data is partial
		 * 
		 * @param[in]   The packet to decapsulate
		 * @parma[in]   The packet count to decapsulate (0 if unknown)
		 * @param[out]  The status about decapsulation (true if data is incomplete to decapsulation, false otherwise)
		 * @param[out]  The list of decapsulated packet
		 */
		virtual bool decapNextPacket(NetContainer *packet,
			bool &partial_decap,
			vector<NetPacket *> &decap_packets,
			unsigned int decap_packets_count = 0);

		/**
		 * @brief Reset partial data of a packet after decapsulation
		 *
		 * @param[in]   The packet to reset remaining data
		 * 
		 * @return  true if success, false otherwise
		 */
		virtual bool resetPacketToDecap();

		virtual NetPacket *getPacketForHeaderExtensions(const std::vector<NetPacket*>&UNUSED(packets))
		{
			assert(0);
		};

		virtual bool setHeaderExtensions(const NetPacket* UNUSED(packet),
		                                 NetPacket** UNUSED(new_packet),
		                                 tal_id_t UNUSED(tal_id_src),
		                                 tal_id_t UNUSED(tal_id_dst),
		                                 string UNUSED(callback_name),
		                                 void *UNUSED(opaque))
		{
			assert(0);
		};


		virtual bool getHeaderExtensions(const NetPacket *UNUSED(packet),
		                                 string UNUSED(callback_name),
		                                 void *UNUSED(opaque))
		{
			assert(0);
		};

		list<string> getCallback()
		{
			return this->callback_name;
		};

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
		virtual bool getChunk(NetPacket *packet,
			size_t remaining_length,
			NetPacket **data,
			NetPacket **remaining_data) const = 0;

		/// Output Logs
		OutputLog *log;

		/// map call back name
		list<string> callback_name;

		/// map packets being encapsulated
		map<NetPacket *, NetPacket *> encap_packets;
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
		EncapContext(EncapPlugin &pl):
			StackContext(pl)
		{
			this->dst_tal_id = BROADCAST_TAL_ID;
		};
		
		/**
		 * Flush the encapsulation context identified by context_id (after a context
		 * expiration for example). It's the caller charge to delete the returned
		 * NetBurst after use.
		 *
		 * @param context_id  the context to flush
		 * @return            a list of encapsulation packets
		 */
		// TODO replace int by uintXX_t
		virtual NetBurst *flush(int context_id) = 0;
		
		/**
		 * Flush all the encapsulation contexts. It's the caller charge to delete
		 * the returned NetBurst after use.
		 *
		 * @return  a list of encapsulation packets
		 */
		virtual NetBurst *flushAll() = 0;

		/**
		 * @brief Set the filter on destination TAL Id.
		 *
		 * @param tal_id  The destination TAL Id.
		 */
		void setFilterTalId(uint8_t tal_id)
		{
			this->dst_tal_id = tal_id;
		}

		virtual void init()
		{
			this->log = Output::registerLog(LEVEL_WARNING,
			                                "Encap.%s",
			                                this->getName().c_str());
		};

	  protected:

		/// The destination TAL Id to filter received packet on
		uint8_t dst_tal_id;

		/// Output Logs
		OutputLog *log;
	};

	virtual void init()
	{
		this->log = Output::registerLog(LEVEL_WARNING,
		                                "Encap.%s",
		                                this->getName().c_str());
	};

	/* for the following functions we use "covariant return type" */

	/**
	 * @brief Get the context
	 *
	 * @return the context
	 */
	EncapContext *getContext() const
	{
		return static_cast<EncapContext *>(this->context);
	};

	/**
	 * @brief Get the packet handler
	 *
	 * @return the packet handler
	 */
	EncapPacketHandler *getPacketHandler() const
	{
		return static_cast<EncapPacketHandler *>(this->packet_handler);
	};

};

typedef std::vector<EncapPlugin::EncapContext *> encap_contexts_t;

#ifdef CREATE
#undef CREATE
#define CREATE(CLASS, CONTEXT, HANDLER, pl_name) \
			CREATE_STACK(CLASS, CONTEXT, HANDLER, pl_name, encapsulation_plugin)
#endif

#endif

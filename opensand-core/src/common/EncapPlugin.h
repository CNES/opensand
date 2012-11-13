/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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
 * @brief Generic encapsulation / deencapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef ENCAP_CONTEXT_H
#define ENCAP_CONTEXT_H

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "NetPacket.h"
#include "NetBurst.h"
#include "OpenSandCore.h"
#include "OpenSandPlugin.h"


typedef enum
{
	REGENERATIVE,
	TRANSPARENT,
} sat_type_t;


/**
 * @class EncapPlugin
 * @brief Generic encapsulation / deencapsulation plugin
 */
class EncapPlugin:public OpenSandPlugin
{

 public:

	/**
	 * @class EncapPacketHandler
	 * @brief Functions to handle the encapsulated packets
	 */
	class EncapPacketHandler
	{

	  public:

		/**
		 * @brief EncapPacketHandler constructor
		 */
		/* Allow packets to access EncapPlugin members */
		EncapPacketHandler(EncapPlugin &pl): plugin(pl) {};

		/**
		 * @brief EncapPacketHandler destructor
		 */
		virtual ~EncapPacketHandler() {};

		/**
		 * @brief get the packet length if constant
		 *
		 * @return the packet length if constant, 0 otherwise
		 */
		virtual size_t getFixedLength() = 0;

		/**
		 * @brief get the minimum packet length
		 *
		 * @return the minimum packet length
		 */
		virtual size_t getMinLength() = 0;

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
		virtual NetPacket *build(unsigned char *data, size_t data_length,
		                         uint8_t qos,
		                         uint8_t src_tal_id, uint8_t dst_tal_id) = 0;

		/**
		 * @brief Get a packet length
		 *
		 * @param data The packet content
		 * @return the packet length
		 */
		virtual size_t getLength(const unsigned char *data) = 0;

		/**
		 * @brief get a NetPacket that can be encapsulated in the frame
		 * 
		 *   There is 4 use case:
		 *    1. the whole packet can be encapsulated
		 *    2. the packet should be fragmented to be encapsulated
		 *    3. the packet cannot be encapsulated, even fragmented
		 *    4. an error occured
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
		virtual bool getChunk(NetPacket *packet, size_t remaining_length,
		                      NetPacket **data, NetPacket **remaining_data) = 0;

		/**
		 * @brief Get the EtherType associated with the encapsulation protocol
		 *
		 * return The EtherType
		 */
		virtual uint16_t getEtherType() {return plugin.ether_type;};

		/**
		 * @brief Get the type of encapsulation / deencapsulation context (ATM, MPEG, etc.)
		 *
		 * @return the name of the encapsulation / deencapsulation context
		 */
		virtual std::string getName() {return plugin.name;};

	  private:

		EncapPlugin &plugin;
	};

	/**
	 * @class EncapContext
	 * @brief The encapsulation/deencapsulation context
	 */
	class EncapContext
	{
	  public:

		/* Allow context to access EncapPlugin members */
		/**
		 * @brief EncapContext constructor
		 */
		EncapContext(EncapPlugin &pl): plugin(pl)
		{
			this->dst_tal_id = BROADCAST_TAL_ID;
		};

		/**
		 * @brief EncapContext destructor
		 */
		virtual ~EncapContext() {};

		/**
		 * Encapsulate some packets into one or several packets.
		 * The function returns a context ID and expiration time vector.
		 * It's the caller charge to arm the timers to manage contextis expiration.
		 * It's also the caller charge to delete the returned NetBurst after use.
		 *
		 * @param packet        the packet to encapsulate
		 * @param time_contexts a map of time and context ID where:
		 *                       - context ID identifies the context in which the
		 *                         IP packet was encapsulated
		 *                       - time is the time before the context identified by
		 *                         the context ID expires
		 * @return              a list of packets
		 */
		virtual NetBurst *encapsulate(NetBurst *burst,
		                              std::map<long, int> &time_contexts) = 0;

		/**
		 * Deencapsulate some packets into one or several packets.
		 * It's the caller charge to delete the returned NetBurst after use.
		 *
		 * @param packet  the encapsulation packet to deencapsulate
		 * @return        a list of packets
		 */
		virtual NetBurst *deencapsulate(NetBurst *burst) = 0;

		/**
		 * Flush the encapsulation context identified by context_id (after a context
		 * expiration for example). It's the caller charge to delete the returned
		 * NetBurst after use.
		 *
		 * @param context_id  the context to flush
		 * @return            a list of encapsulation packets
		 */
		virtual NetBurst *flush(int context_id) = 0;

		/**
		 * Flush all the encapsulation contexts. It's the caller charge to delete
		 * the returned NetBurst after use.
		 *
		 * @return  a list of encapsulation packets
		 */
		virtual NetBurst *flushAll() = 0;

		/** @brief Get the list of protocols that can be encapsulated
		 *
		 *  @param sat_type The satellite payload type (REGENERATIVE or
		 *                                              TRANSPARENT)
		 *  @param return The list of protocols that can be encapsulated
		 */
		std::vector<std::string> getAvailableUpperProto(sat_type_t sat_type)
		{
			return plugin.upper[sat_type];
		};

		/**
		 * @brief Get the EtherType associated with the encapsulation protocol
		 *
		 * @param name     The upper encapsulation name for compatibility check
		 * @param sat_type The satellite type for compatibility check
		 *
		 * return The EtherType
		 */
		uint16_t getEtherType() {return plugin.ether_type;};

		/**
		 * @brief Set the encapsulated packet handler
		 *
		 * @param pkt_hdl  The encapsulated packet handler
		 * @param sat_type The type of satellite payload
		 * @return true if this type of packet can be encapsulated, false otherwise
		 */
		bool setUpperPacketHandler(EncapPlugin::EncapPacketHandler *pkt_hdl,
		                           sat_type_t sat_type)
		{
			if(!pkt_hdl)
			{
				this->current_upper = NULL;
				return false;
			}
			
			std::vector<std::string>::iterator iter;

			iter = find((plugin.upper[sat_type]).begin(),
			            (plugin.upper[sat_type]).end(), pkt_hdl->getName());

			if(iter == (plugin.upper[sat_type]).end())
				return false;

			this->current_upper = pkt_hdl;
			return true;
		};

		/**
		 * @brief Get the type of encapsulation / deencapsulation context (ATM, MPEG, etc.)
		 *
		 * @return the type of encapsulation / deencapsulation context
		 */
		std::string getName() {return plugin.name;};

		/**
		 * @brief Create a NetPacket from data with the relevant attributes
		 * 
		 * @param data        The packet data
		 * @param data_length The length of the packet
		 * @param qos         The QoS value to associate with the packet
		 * @param src_tal_id  The source terminal ID to associate with the packet
		 * @param dst_tal_id  The destination terminal ID to associate with the packet
		 * @return the packet on success, NULL otherwise
		 */
		NetPacket *createPacket(unsigned char *data, size_t data_length,
		                        uint8_t qos,
		                        uint8_t src_tal_id, uint8_t dst_tal_id)
		{
			return plugin.packet_handler->build(data, data_length,
			                                    qos,
			                                    src_tal_id, dst_tal_id);
		}

		/**
		 * @brief Set the filter on destination TAL Id.
		 *
		 * @param tal_id  The destination TAL Id.
		 */
		void setFilterTalId(uint8_t tal_id)
		{
			this->dst_tal_id = tal_id;
		}

	  protected:

		/// the current upper encapsulation protocol EtherType
		EncapPlugin::EncapPacketHandler *current_upper;

		/// The destination TAL Id to filter received packet on
		uint8_t dst_tal_id;

	  private:

		/// The plugin
		EncapPlugin &plugin;

	};
	
	/**
	 * @brief EncapPlugin constructor
	 */
	EncapPlugin(uint16_t ether_type): OpenSandPlugin()
	{
		this->ether_type = ether_type;
	};


	/**
	 * @brief EncapPlugin destructor
	 */
	virtual ~EncapPlugin()
	{
		if(this->context)
		{
			delete this->context;
		}

		if(this->packet_handler)
		{
			delete this->packet_handler;
		}
	};

	/**
	 * @brief Get the encapsulation context
	 *
	 * @return the context
	 */
	EncapContext *getContext() {return this->context;};

	/**
	 * @brief Get the encapsulation packet handler
	 *
	 * @return the packet handler
	 */
	EncapPacketHandler *getPacketHandler() {return this->packet_handler;};

	/**
	 * @brief Get The plugin name
	 *
	 * @return the plugin name
	 */
	string getName() {return this->name;};

	/**
	 * @brief Create the Plugin, this function should be called instead of constructor
	 *
	 * @return The plugin
	 */
	template<class Plugin, class Context, class Handler>
	static OpenSandPlugin *create(std::string name)
	{
		Plugin *plugin = new Plugin();
		Context *context = new Context(*plugin);
		Handler *handler = new Handler(*plugin);
		plugin->context = context;
		plugin->packet_handler = handler;
		plugin->name = name;
		return plugin;
	};

 protected:

	/// The EtherType (or EtherType like) of the associated protocol
	uint16_t ether_type;

	/** The list of protocols that can be encapsulated according to satellite
	 *  payload type */
	std::map<sat_type_t, std::vector<std::string> > upper;

	/// The encapsulation context
	EncapContext *context;

	/// The encapsulation packet
	EncapPacketHandler *packet_handler;

};

/// Define the function that will create the plugin class
#ifdef CREATE
#undef CREATE
#define CREATE(CLASS, CONTEXT, HANDLER, pl_name) \
	extern "C" OpenSandPlugin *create_ptr(){return CLASS::create<CLASS, CONTEXT, HANDLER>(pl_name);}; \
	extern "C" opensand_plugin_t *init() \
	{\
		opensand_plugin_t *pl = (opensand_plugin_t *)calloc(1, sizeof(opensand_plugin_t)); \
		pl->create = create_ptr; \
		pl->type = encapsulation_plugin; \
		strncpy(pl->name, pl_name, PLUGIN_MAX_LEN - 1); \
		pl->name[PLUGIN_MAX_LEN - 1] = '\0'; \
		return pl; \
	};
#endif

#endif

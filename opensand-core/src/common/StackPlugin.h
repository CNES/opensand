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
 * @file StackPlugin.h
 * @brief Generic plugin for stack elements
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef STACK_H
#define STACK_H

#include <map>
#include <vector>

#include <opensand_rt/Ptr.h>
#include <opensand_rt/Data.h>

#include "OpenSandPlugin.h"
#include "OpenSandCore.h"


class NetBurst;
class NetContainer;
class NetPacket;
class OutputLog;
enum class NET_PROTO : uint16_t;


/**
 * @class StackPlugin
 * @brief Generic stack plugin
 */
class StackPlugin: public virtual OpenSandPlugin
{
public:
	/**
	 * @class StackPacketHandler
	 * @brief Functions to handle the encapsulated packets
	 */
	class StackPacketHandler
	{
	public:
		/**
		 * @brief StackPacketHandler constructor
		 */
		/* Allow packets to access StackPlugin members */
		StackPacketHandler(StackPlugin &pl);

		/**
		 * @brief StackPacketHandler destructor
		 */
		virtual ~StackPacketHandler() = default;

		/**
		 * @brief get the packet length if constant
		 *
		 * @return the packet length if constant, 0 otherwise
		 */
		virtual std::size_t getFixedLength() const = 0;

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
		virtual Rt::Ptr<NetPacket> build(const Rt::Data &data,
		                                 std::size_t data_length,
		                                 uint8_t qos,
		                                 uint8_t src_tal_id,
		                                 uint8_t dst_tal_id)  = 0;

		/**
		 * @brief Get a packet length
		 *
		 * @param data The packet content
		 * @return the packet length
		 */
		virtual std::size_t getLength(const unsigned char *data) const = 0;

		/**
		 * @brief Get the EtherType associated with the related protocol
		 *
		 * return The EtherType
		 */
		virtual NET_PROTO getEtherType() const;

		/**
		 * @brief Get the type of stack
		 *
		 * @return the name of the stack
		 */
		virtual std::string getName() const;

		/* The functions below are only used by EncapPlugin but we need them to avoid
		 * casting upper packet handlers for EncapPlugins that does not support
		 * lan adaptation upper packets */

		/**
		 * @brief get the minimum packet length
		 *
		 * @return the minimum packet length
		 */
		virtual std::size_t getMinLength() const = 0;

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
		virtual bool encapNextPacket(Rt::Ptr<NetPacket> packet,
		                             std::size_t remaining_length,
		                             bool new_burst,
		                             Rt::Ptr<NetPacket> &encap_packet,
		                             Rt::Ptr<NetPacket> &remaining_data) = 0;

		/**
		 * @brief Get encapsulated packet from payload
		 *
		 * @param[in]  packet             The packet storing payload
		 * @param[out] partial_decap      The status about decapsulation (true if data
		 *                                is incomplete to decapsulation, false otherwise)
		 * @param[out] decap_packets      The list of decapsulated packet
		 * @param[in decap_packets_count  The packet count to decapsulate (0 if unknown)
		 */
		virtual bool getEncapsulatedPackets(Rt::Ptr<NetContainer> packet,
		                                    bool &partial_decap,
		                                    std::vector<Rt::Ptr<NetPacket>> &decap_packets,
		                                    unsigned int decap_packet_count = 0) = 0;

		/**
		 * @brief perform some plugin initialization
		 */
		virtual bool init() = 0;

	protected:
		/// Output Logs
		std::shared_ptr<OutputLog> log;

	private:
		StackPlugin &plugin;
	};

	/**
	 * @class StackContext
	 * @brief The stack context
	 */
	class StackContext
	{
	public:
		/* Allow context to access StackPlugin members */
		/**
		 * @brief StackContext constructor
		 */
		StackContext(StackPlugin &pl);

		/**
		 * @brief StackContext destructor
		 */
		virtual ~StackContext() = default;

		/**
		 * Encapsulate some packets into one or several packets.
		 * The function returns a context ID and expiration time map.
		 * It's the caller charge to arm the timers to manage contextis expiration.
		 *
		 * @param burst        the packets to encapsulate
		 * @param time_contexts a map of time and context ID where:
		 *                       - context ID identifies the context in which the
		 *                         packet was encapsulated
		 *                       - time is the time before the context identified by
		 *                         the context ID expires
		 * @return              a list of packets
		 */
		virtual Rt::Ptr<NetBurst> encapsulate(Rt::Ptr<NetBurst> burst,
		                                      std::map<long, int> &time_contexts) = 0;

		/**
		 * Encapsulate some packets into one or several packets for contexts with
		 * no timer.
		 *
		 * @param burst  the packets to encapsulate
		 * @return       a list of packets
		 */
		virtual Rt::Ptr<NetBurst> encapsulate(Rt::Ptr<NetBurst> burst);

		/**
		 * Deencapsulate some packets into one or several packets.
		 *
		 * @param burst   the stack packets to deencapsulate
		 * @return        a list of packets
		 */
		virtual Rt::Ptr<NetBurst> deencapsulate(Rt::Ptr<NetBurst> burst) = 0;

		/** @brief Get the list of protocols that can be encapsulated
		 *
		 *  @param return The list of protocols that can be encapsulated
		 */
		std::vector<std::string> getAvailableUpperProto() const;

		/**
		 * @brief Get the EtherType associated with the encapsulation protocol
		 *
		 * return The EtherType
		 */
		NET_PROTO getEtherType() const;

		/**
		 * @brief Set the encapsulated packet handler
		 *
		 * @param pkt_hdl  The encapsulated packet handler
		 * @return true if this type of packet can be encapsulated, false otherwise
		 */
		virtual bool setUpperPacketHandler(std::shared_ptr<StackPlugin::StackPacketHandler> pkt_hdl);

		/**
		 * @brief Update statistics periodically
		 *
		 * @param period  The time interval bewteen two updates
		 */
		virtual void updateStats(const time_ms_t &period);

		/**
		 * @brief Get the name of the plugin
		 *
		 * @return the name of the plugin
		 */
		std::string getName() const;

		/**
		 * @brief Create a NetPacket from data with the relevant attributes
		 *
		 * @param data        The packet data
		 * @param data_length The packet length
		 * @param qos         The QoS value to associate with the packet
		 * @param src_tal_id  The source terminal ID to associate with the packet
		 * @param dst_tal_id  The destination terminal ID to associate with the packet
		 * @return the packet on success, NULL otherwise
		 */
		Rt::Ptr<NetPacket> createPacket(const Rt::Data &data,
		                                std::size_t data_length,
		                                uint8_t qos,
		                                uint8_t src_tal_id,
		                                uint8_t dst_tal_id);

		/**
		 * @brief perform some plugin initialization
		 *
		 * @return True if success, false otherwise
		 */
		virtual bool init() = 0;

	protected:
		/// the current upper encapsulation protocol EtherType
		std::shared_ptr<StackPlugin::StackPacketHandler> current_upper;

		/// Output Logs
		std::shared_ptr<OutputLog> log;

	private:
		/// The plugin
		StackPlugin &plugin;
	};

	/**
	 * @brief StackPlugin constructor
	 */
	StackPlugin(NET_PROTO ether_type);


	/**
	 * @brief StackPlugin destructor
	 */
	virtual ~StackPlugin() = default;

	/**
	 * @brief Get the encapsulation context
	 *
	 * @return the context
	 */
	std::shared_ptr<StackContext> getContext() const;

	/**
	 * @brief Get the encapsulation packet handler
	 *
	 * @return the packet handler
	 */
	std::shared_ptr<StackPacketHandler> getPacketHandler() const;

	/**
	 * @brief Get The plugin name
	 *
	 * @return the plugin name
	 */
	std::string getName() const;

	/**
	 * @brief Create the Plugin, this function should be called instead of constructor
	 *
	 * @return The plugin
	 */
	template<class Plugin, class Context, class Handler>
	static OpenSandPlugin *create(const std::string &name)
	{
		Plugin *plugin = new Plugin();
		auto context = std::make_shared<Context>(*plugin);
		auto handler = std::make_shared<Handler>(*plugin);
		plugin->context = context;
		plugin->packet_handler = handler;
		plugin->name = name;
		if(!plugin->init())
		{
			goto error;
		}
		if(!context->init())
		{
			goto error;
		}
		if(!handler->init())
		{
			goto error;
		}
		return plugin;
		
	error:
		delete plugin;
		return nullptr;
	};

	/**
	 * @brief perform some plugin initialization
	 *
	 * @return True if success, false otherwise
	 */
	virtual bool init() = 0;

protected:
	/// The EtherType (or EtherType like) of the associated protocol
	NET_PROTO ether_type;

	/// The list of protocols that can be "encapsulated"
	std::vector<std::string> upper;

	/// The context
	std::shared_ptr<StackContext> context;

	/// The packet handler
	std::shared_ptr<StackPacketHandler> packet_handler;

	/// Output Logs
	std::shared_ptr<OutputLog> log;
};


/// Define the function that will create the plugin class
#define CREATE_STACK(CLASS, CONTEXT, HANDLER, pl_name, pl_type) \
	extern "C" OpenSandPlugin *create_ptr(void) \
	{ \
		return CLASS::create<CLASS, CONTEXT, HANDLER>(pl_name); \
	}; \
	extern "C" void configure_ptr(const char *parent_path, const char *param_id) \
	{\
		CLASS::configure<CLASS>(parent_path, param_id, pl_name); \
	}; \
	extern "C" OpenSandPluginFactory *init() \
	{ \
		auto pl = new OpenSandPluginFactory{ \
			create_ptr, \
			configure_ptr, \
			pl_type, \
			pl_name \
		}; \
		return pl; \
	};


#endif

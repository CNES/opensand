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
		virtual Rt::Ptr<NetPacket> createPacket(const Rt::Data &data,
										        std::size_t data_length,
										        uint8_t qos,
										        uint8_t src_tal_id,
										        uint8_t dst_tal_id) = 0;

		/**
		 * @brief perform some plugin initialization
		 *
		 * @return True if success, false otherwise
		 */
		virtual bool init() = 0;

	protected:
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
	template<class Plugin, class Context>
	static OpenSandPlugin *create(const std::string &name)
	{
		Plugin *plugin = new Plugin();
		auto context = std::make_shared<Context>(*plugin);
		plugin->context = context;
		plugin->name = name;
		if(!plugin->init())
		{
			goto error;
		}
		if(!context->init())
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

	/// Output Logs
	std::shared_ptr<OutputLog> log;
};


/// Define the function that will create the plugin class
#define CREATE_STACK(CLASS, CONTEXT, pl_name, pl_type) \
	extern "C" OpenSandPlugin *create_ptr(void) \
	{ \
		return CLASS::create<CLASS, CONTEXT>(pl_name); \
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

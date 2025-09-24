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
 * @file LanAdaptationPlugin.h
 * @brief Generic LAN adaptation plugin
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef LAN_ADAPTATION_CONTEXT_H
#define LAN_ADAPTATION_CONTEXT_H


#include <memory>

#include "OpenSandCore.h"
#include "StackPlugin.h"


class NetBurst;
class SarpTable;
class OutputLog;
class PacketSwitch;


/**
 * @class LanAdaptationPlugin
 * @brief Generic Lan adaptation plugin
 */
class LanAdaptationPlugin: public StackPlugin
{
public:
	/**
	 * @class LanAdaptationContext
	 * @brief The encapsulation/deencapsulation context
	 */
	class LanAdaptationContext: public StackContext
	{
	public:
		/* Allow context to access LanAdaptationPlugin members */
		/**
		 * @brief LanAdaptationContext constructor
		 */
		LanAdaptationContext(LanAdaptationPlugin &pl);

		/**
		 * @brief Initialize the plugin with some bloc configuration
		 *
		 * @param tal_id           The terminal ID
		 * @param class_list       A list of service classes
		 * @return true on success, false otherwise
		 */
		virtual bool initLanAdaptationContext(tal_id_t tal_id, std::shared_ptr<PacketSwitch> packet_switch);

		/**
		 * @brief Get the bytes of LAN header for TUN/TAP interface
		 *
		 * @param pos    The bytes position
		 * @param packet The current packet
		 * @return     The byte indicated by pos
		 */
		virtual char getLanHeader(unsigned int pos, const Rt::Ptr<NetPacket>& packet) = 0;

		/**
		 * @brief check if the packet should be read/written on TAP or TUN interface
		 *
		 * @return true for TAP, false for TUN
		 */
		virtual bool handleTap() = 0;

		virtual bool init();

	protected:
		/// Can we handle packet read from TUN or TAP interface
		bool handle_net_packet;

		/// The terminal ID
		tal_id_t tal_id;

		/// The SARP table
		std::shared_ptr<PacketSwitch> packet_switch;
	};

	LanAdaptationPlugin(NET_PROTO ether_type);

	virtual bool init();

	/**
	 * @brief Get the context
	 *
	 * @return the context
	 */
	inline std::shared_ptr<LanAdaptationContext> getContext() const
	{
		return std::static_pointer_cast<LanAdaptationContext>(this->context);
	};
};

typedef std::shared_ptr<LanAdaptationPlugin::LanAdaptationContext> lan_context_t;


#ifdef CREATE
#undef CREATE
#define CREATE(CLASS, CONTEXT, pl_name) \
	CREATE_STACK(CLASS, CONTEXT, pl_name, PluginType::LanAdaptation)
#endif

#endif

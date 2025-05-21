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
	 * @class LanAdaptationPacketHandler
	 * @brief Functions to handle the encapsulated packets
	 * @warning Be really careful, encapsulation and deencapsulation
	 *          are handled in two different thread so shared ressources
	 *          can be accessed concurrently
	 *          If you wish to prevent concurrent access to one ressource
	 *          you can take the lock with the Block function Lock and
	 *          unlock it with the Block function Unlock
	 *          This should be avoided as this will remove process
	 *          efficiency
	 *          All the attributes defines below should be read-only
	 *          once initLanAdaptationContext is called so there is
	 *          no need to protect them
	 */
	class LanAdaptationPacketHandler: public StackPacketHandler
	{
	public:
		/**
		 * @brief LanAdaptationPacketHandler constructor
		 */
		/* Allow packets to access LanAdaptationPlugin members */
		LanAdaptationPacketHandler(LanAdaptationPlugin &pl);

		bool init() override;

		/* the following functions should not be called */
		std::size_t getMinLength() const override;

		bool encapNextPacket(Rt::Ptr<NetPacket> packet,
		                     std::size_t remaining_length,
		                     bool new_burst,
		                     Rt::Ptr<NetPacket> &encap_packet,
		                     Rt::Ptr<NetPacket> &remaining_packet) override;

		bool getEncapsulatedPackets(Rt::Ptr<NetContainer> packet,
		                            bool &partial_decap,
		                            std::vector<Rt::Ptr<NetPacket>> &decap_packets,
		                            unsigned int decap_packets_count) override;
	};

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

		bool setUpperPacketHandler(std::shared_ptr<StackPlugin::StackPacketHandler> pkt_hdl) override;

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

	/**
	 * @brief Get the packet handler
	 *
	 * @return the packet handler
	 */
	inline std::shared_ptr<LanAdaptationPacketHandler> getPacketHandler() const
	{
		return std::static_pointer_cast<LanAdaptationPacketHandler>(this->packet_handler);
	};
};

typedef std::vector<std::shared_ptr<LanAdaptationPlugin::LanAdaptationContext>> lan_contexts_t;


#ifdef CREATE
#undef CREATE
#define CREATE(CLASS, CONTEXT, HANDLER, pl_name) \
	CREATE_STACK(CLASS, CONTEXT, HANDLER, pl_name, PluginType::LanAdaptation)
#endif

#endif

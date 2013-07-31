/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 */

#ifndef LAN_ADAPTATION_CONTEXT_H
#define LAN_ADAPTATION_CONTEXT_H


#include "NetBurst.h"
#include "OpenSandCore.h"
#include "StackPlugin.h"
#include "SarpTable.h"

#include <cassert>

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
	 */
	class LanAdaptationPacketHandler: public StackPacketHandler
	{

	  public:

		/**
		 * @brief LanAdaptationPacketHandler constructor
		 */
		/* Allow packets to access LanAdaptationPlugin members */
		LanAdaptationPacketHandler(LanAdaptationPlugin &pl):
			StackPacketHandler(pl)
		{};

		/* the following functions should not be called */

		size_t getMinLength() const {assert(0);};
		bool getChunk(NetPacket *UNUSED(packet),
		              size_t UNUSED(remaining_length),
		              NetPacket **UNUSED(data),
		              NetPacket **UNUSED(remaining_data))
		{
			assert(0);
		};
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
		LanAdaptationContext(LanAdaptationPlugin &pl):
			StackContext(pl),
			handle_net_packet(false)
		{
		};

		/** 
		 * @brief Initialize the plugin with some bloc configuration
		 *
		 * @param tal_id           The terminal ID
		 * @param satellite_type   The satellite type
		 * @param class_list       A list of service classes
		 * @return true on success, false otherwise
		 */
		virtual bool initLanAdaptationContext(
			tal_id_t tal_id,
			sat_type_t satellite_type,
			const SarpTable *sarp_table)
		{
			this->tal_id = tal_id;
			this->satellite_type = satellite_type;
			this->sarp_table = sarp_table;
			return true;
		};

		/**
		 * @brief Get the bytes of LAN header for TUN/TAP interface
		 *
		 * @param pos    The bytes position
		 * @param packet The current packet
		 * @return     The byte indicated by pos
		 */
		virtual char getLanHeader(unsigned int pos, NetPacket *packet) = 0;

		/**
		 * @brief check if the packet should be read/written on TAP or TUN interface
		 *
		 * @return true for TAP, false for TUN
		 */
		virtual bool handleTap() = 0;

		bool setUpperPacketHandler(StackPlugin::StackPacketHandler *pkt_hdl,
		                           sat_type_t sat_type)
		{
			if(!pkt_hdl && this->handle_net_packet)
			{
				this->current_upper = NULL;
				return true;
			}
			return StackPlugin::StackContext::setUpperPacketHandler(pkt_hdl,
			                                                        sat_type);
		}

	  protected:

		/// Can we handle packet read from TUNor TAP interface
		bool handle_net_packet;

		/// The terminal ID
		tal_id_t tal_id;

		/// The satellite type
		sat_type_t satellite_type;

		/// The SARP table
		const SarpTable *sarp_table;

	};

	LanAdaptationPlugin(uint16_t ether_type): StackPlugin(ether_type) {};

	/**
	 * @brief Get the context
	 *
	 * @return the context
	 */
	LanAdaptationContext *getContext() const
	{
		return static_cast<LanAdaptationContext *>(this->context);
	};

	/**
	 * @brief Get the packet handler
	 *
	 * @return the packet handler
	 */
	LanAdaptationPacketHandler *getPacketHandler() const
	{
		return static_cast<LanAdaptationPacketHandler *>(this->packet_handler);
	};

};


typedef std::vector<LanAdaptationPlugin::LanAdaptationContext *> lan_contexts_t;

#ifdef CREATE
#undef CREATE
#define CREATE(CLASS, CONTEXT, HANDLER, pl_name) \
		CREATE_STACK(CLASS, CONTEXT, HANDLER, pl_name, lan_adaptation_plugin)
#endif

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file PacketSwitch.h
 * @brief Get switch information about packets
 *
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Francklin SIMO <fsimo@toulouse.viveris.com>
 */

#ifndef PACKET_SWITCH_H
#define PACKET_SWITCH_H

#include "OpenSandModelConf.h"
#include "SarpTable.h"
#include "Data.h"
#include <opensand_rt/RtMutex.h>

#include <opensand_output/OutputLog.h>



/**
 * @class PacketSwitch
 * @brief Get switch information about packets
 */
class PacketSwitch
{
 public:
	PacketSwitch(const tal_id_t &tal_id);

	virtual ~PacketSwitch() = default;

	/**
	 * @brief Get the OpenSAND destination of packet from its MAC destination
	 *
	 * @param packet   The packet
	 * @param dst_id  The returned OpenSAND destination
	 *
	 * @return true if destination found, false otherwise
	 */
	virtual bool getPacketDestination(const Data &packet, tal_id_t &src_id, tal_id_t &dst_id) = 0;

	/**
	 * @brief Check a packet is destinated to the current entity
	 *
	 * @param packet   The packet
	 * @param src_id   The OpenSAND source of the packet
	 * @param forward  True if forward is required, false otherwise
	 *
	 * @return true if packet is for the current entity, false otherwise
	 */
	virtual bool isPacketForMe(const Data &packet, const tal_id_t &src_id, bool &forward) = 0;

	/**
	 * @brief Learn the source MAC address of the specified packet
	 *
	 * @param packet  The packet
	 * @param src_id  The ID of the corresponding terminal
	 * 
	 * @return true on success, false otherwise
 	 */
	bool learn(const Data &packet, const tal_id_t &src_id);

	SarpTable *getSarpTable();

 protected:
	/// The mutex
	RtMutex mutex;

	/// The terminal id of the entity
	tal_id_t tal_id;

	/// The SARP table
	SarpTable sarp_table;
};

/**
 * @class TerminalPacketSwitch
 * @brief Get switch information about packets for satellite terminal
 */
class TerminalPacketSwitch: public PacketSwitch
{
 public:
	TerminalPacketSwitch(const tal_id_t &id, const tal_id_t &gw_id):
		 PacketSwitch(id),
		 gw_id(gw_id)
	{};

	/**
	 * @brief Get the OpenSAND destination of packet from its MAC destination
	 *
	 * @param packet   The packet
	 * @param dest_id  The returned OpenSAND destination
	 *
	 * @return true if destination found, false otherwise
	 */
	bool getPacketDestination(const Data &packet, tal_id_t &src_id, tal_id_t &dest_id);

	/**
	 * @brief Check a packet is destinated to the current entity
	 *
	 * @param packet   The packet
	 * @param src_id   The OpenSAND source of the packet
	 * @param forward  True if forwardis required, false otherwise
	 *
	 * @return true if packet is for the current entity, false otherwise
	 */
	bool isPacketForMe(const Data &packet, const tal_id_t &src_id, bool &forward);

 protected:
	/// The gateway id of the terminal entity
	tal_id_t gw_id;
};

/**
 * @class GatewayPacketSwitch
 * @brief Get switch information about packets for gateway
 */
class GatewayPacketSwitch: public PacketSwitch
{
 public:
	GatewayPacketSwitch(const tal_id_t &id):
		 PacketSwitch(id)
	{};

	/**
	 * @brief Get the OpenSAND destination of packet from its MAC destination
	 *
	 * @param packet   The packet
	 * @param dst_id  The returned OpenSAND destination
	 *
	 * @return true if destination found, false otherwise
	 */
	bool getPacketDestination(const Data &packet, tal_id_t &src_id, tal_id_t &dst_id);

	/**
	 * @brief Check a packet is destinated to the current entity
	 *
	 * @param packet   The packet
	 * @param src_id   The OpenSAND source of the packet
	 * @param forward  True if forwardis required, false otherwise
	 *
	 * @return true if packet is for the current entity, false otherwise
	 */
	bool isPacketForMe(const Data &packet, const tal_id_t &src_id, bool &forward);
};

#endif

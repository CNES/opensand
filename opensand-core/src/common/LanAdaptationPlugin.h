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

#include <opensand_rt/Ptr.h>
#include <opensand_rt/Data.h>

#include "OpenSandCore.h"
#include "OpenSandPlugin.h"


class NetBurst;
class NetPacket;
class OutputLog;
class PacketSwitch;
enum class NET_PROTO : uint16_t;


/**
 * @class LanAdaptationPlugin
 * @brief Generic Lan adaptation plugin
 */
class LanAdaptationPlugin: public OpenSandPlugin
{
public:
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

	LanAdaptationPlugin(NET_PROTO ether_type);

	virtual bool init();

protected:
	/// The EtherType (or EtherType like) of the associated protocol
	NET_PROTO ether_type;

	/// Can we handle packet read from TUN or TAP interface
	bool handle_net_packet;

	/// The terminal ID
	tal_id_t tal_id;

	/// The SARP table
	std::shared_ptr<PacketSwitch> packet_switch;

	/// Output Logs
	std::shared_ptr<OutputLog> log;
};


#endif

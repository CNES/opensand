/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file BlockLanAdaptation.h
 * @brief Interface between network interfaces and OpenSAND
 *
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Nicol <julien.nicol@b2i-toulouse.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef BLOCK_LAN_ADAPTATION_H
#define BLOCK_LAN_ADAPTATION_H


#include "SarpTable.h"
#include "PacketSwitch.h"
#include "TrafficCategory.h"
#include "NetPacket.h"
#include "LanAdaptationPlugin.h"
#include "OpenSandCore.h"

#include <opensand_rt/Rt.h>
#include <opensand_rt/RtChannel.h>
#include <opensand_output/Output.h>


class NetSocketEvent;

struct la_specific
{
  std::string tap_iface;
	PacketSwitch *packet_switch;        
};

/**
 * @class BlockLanAdaptation
 * @brief Interface between network interfaces and OpenSAND
 */
class BlockLanAdaptation: public Block
{
 public:

	BlockLanAdaptation(const std::string &name, struct la_specific specific);
	~BlockLanAdaptation();

	static void generateConfiguration();

	// initialization method
	bool onInit(void);

	// The Packet Switch including packet forwarding logic and SARP 
	inline static PacketSwitch *packet_switch;


	class Upward: public RtUpward
	{
	 public:
		Upward(const std::string &name, struct la_specific specific);

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

		/**
		 * @brief Set the lan adaptation contexts for channels
		 *
		 * @param contexts the lan adaptation contexts
		 */
		void setContexts(const lan_contexts_t &contexts);

		/**
		 * @brief Set the network socket file descriptor
		 *
		 * @param fd  The socket file descriptor
		 */
		void setFd(int fd);

	 private:
		/**
		 * @brief Handle a message from lower block
		 *  - build the TAP header with appropriate protocol identifier
		 *  - write TAP header + packet to TAP interface
		 *
		 * @param burst  The burst of packets
		 * @return true on success, false otherwise
		 */
		bool onMsgFromDown(NetBurst *burst);

		/// SARP table
		SarpTable sarp_table;

		/// TAP file descriptor
		int fd;

		/// the contexts list from lower to upper context
		lan_contexts_t contexts;

		/// The MAC layer group id received through msg_link_up
		group_id_t group_id;
		/// The MAC layer MAC id received through msg_link_up
		tal_id_t tal_id;

		/// State of the satellite link
		SatelliteLinkState state;
	};

	class Downward: public RtDownward
	{
	 public:
		Downward(const std::string &name, struct la_specific specific);

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

		/**
		 * @brief Set the lan adaptation contexts for channels
		 *
		 * @param contexts the lan adaptation contexts
		 */
		void setContexts(const lan_contexts_t &contexts);

		/**
		 * @brief Set the network socket file descriptor
		 *
		 * @param fd  The socket file descriptor
		 */
		void setFd(int fd);

	 private:
		/**
		 * @brief Handle a message from upper block
		 *  - read data from TAP interface
		 *  - create a packet with data
		 *
		 * @param event  The event on TAP interface, containing th message
		 * @return true on success, false otherwise
		 */
		bool onMsgFromUp(NetSocketEvent *const event);

		/// statistic timer
		event_id_t stats_timer;

		///  The period for statistics update
		time_ms_t stats_period_ms;

		/// the contexts list from lower to upper context
		lan_contexts_t contexts;

		/// The MAC layer group id received through msg_link_up
		group_id_t group_id;
		/// The MAC layer MAC id received through msg_link_up
		tal_id_t tal_id;

		/// State of the satellite link
		SatelliteLinkState state;
	};

 private:
	/// The TAP interface name
	std::string tap_iface;

	/// Block specific parameters
	la_specific specific;

	/**
	 * Create or connect to an existing TAP interface
	 *
	 * @param fd  OUT: the file descriptor
	 * @return  true on success, false otherwise
	 */
	bool allocTap(int &fd);
};


#endif

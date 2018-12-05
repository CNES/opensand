/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
#include "TrafficCategory.h"
#include "NetPacket.h"
#include "LanAdaptationPlugin.h"
#include "OpenSandCore.h"
#include "OpenSandConf.h"

#include <opensand_rt/Rt.h>
#include <opensand_output/Output.h>

using std::string;


/**
 * @class BlockLanAdaptation
 * @brief Interface between network interfaces and OpenSAND
 */
class BlockLanAdaptation: public Block
{
 public:

	BlockLanAdaptation(const string &name, string lan_iface);
	~BlockLanAdaptation();

	// initialization method
	bool onInit(void);

	class Upward: public RtUpward
	{
	 public:
		Upward(const string &name, string UNUSED(lan_iface)):
			RtUpward(name),
			sarp_table(),
			contexts(),
			state(link_down)
		{};

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
		 * @brief Instantiate the traffic classes
		 * 
		 * @return true on success, false otherwise
		 */
		bool initSarpTables(void);

		/**
		 * @brief Handle a message from lower block
		 *  - build the TUN or TAP header with appropriate protocol identifier
		 *  - write TUN/TAP header + packet to TUN/TAP interface
		 *
		 * @param burst  The burst of packets
		 * @return true on success, false otherwise
		 */
		bool onMsgFromDown(NetBurst *burst);

		/// SARP table
		SarpTable sarp_table;

		/// The satellite type
		sat_type_t satellite_type;

		/// TUN file descriptor
		int fd;

		/// the contexts list from lower to upper context
		lan_contexts_t contexts;

		/// The MAC layer group id received through msg_link_up
		group_id_t group_id;
		/// The MAC layer MAC id received through msg_link_up
		tal_id_t tal_id;

		/// State of the satellite link
		link_state_t state;
	};

	class Downward: public RtDownward
	{
	 public:
		Downward(const string &name, string UNUSED(lan_iface)):
			RtDownward(name),
			stats_period_ms(),
			contexts(),
			state(link_down)
		{};

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
		 *  - read data from TUN or TAP interface
		 *  - create a packet with data
		 *
		 * @param event  The event on TUN/TAP interface, containing th message
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
		link_state_t state;
	};

 private:

	/// The LAN interface name
	string lan_iface;

	/// whether we handle a TAP interface or a TUN interface
	bool is_tap;

	/**
	 * Create or connect to an existing TUN/TAP interface
	 *
	 * @param fd  OUT: the file descriptor
	 * @return  true on success, false otherwise
	 */
	bool allocTunTap(int &fd);

	/**
	 * @brief add LAN interface in bridge
	 *
	 * return true on success, false otherwise
	 */
	bool addInBridge();

	/**
	 * @brief remove LAN interface from bridge
	 *
	 * return true on success, false otherwise
	 */
	bool delFromBridge();

	bool tunConfiguration();
};

#endif

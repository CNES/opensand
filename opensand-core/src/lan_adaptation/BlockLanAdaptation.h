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
 * @file BlockLanAdaptation.h
 * @brief Interface between network interfaces and OpenSAND
 *
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Nicol <julien.nicol@b2i-toulouse.com>
 */

#ifndef BLOCK_LAN_ADAPTATION_H
#define BLOCK_LAN_ADAPTATION_H


#include "SarpTable.h"
#include "TrafficCategory.h"
#include "NetPacket.h"
#include "LanAdaptationPlugin.h"
#include "OpenSandCore.h"

#include <opensand_conf/conf.h>
#include <opensand_rt/Rt.h>
#include <opensand_output/Output.h>

#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <sstream>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/types.h>
#include <arpa/inet.h>
#include <errno.h>

using std::string;

/**
 * @class BlockLanAdaptation
 * @brief Interface between network interfaces and OpenSAND
 */
class BlockLanAdaptation: public Block
{
 public:

	BlockLanAdaptation(const string &name, component_t host,
	                   string lan_iface);
	~BlockLanAdaptation();

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

 private:

	SarpTable sarp_table; ///< SARP table

	/// The LAN interface name
	string lan_iface;

	/// The satellite type
	sat_type_t satellite_type;

	/// the contexts list from lower to upper context
	lan_contexts_t contexts;

	bool getConfig();

	/**
	 * @brief Instantiate the traffic classes
	 * 
	 * @return true on success, false otherwise
	 */
	bool initSarpTables();
	/**
	 * @brief Load the Lan Adaptation plugins according to stack
	 * 
	 * @return true on success, false otherwise
	 */
	bool initLanAdaptationPlugin();
	void terminate();

	// if down
	bool onMsgFromDown(NetBurst *burst);

	/**
	 * Create or connect to an existing TUN/TAP interface
	 *
	 * @return  true on success, false otherwise
	 */
	bool allocTunTap();

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

	bool onMsgFromUp(NetSocketEvent *const event);

	/// Whether the bloc has been initialized or not
	component_t host;

	/**
	 * This map associates directly the category identifier (unique) to a ptr
	 * on the category; it allows fast access when a packet coming from upper
	 * layer needs to be inserted in this category
	 */
	map<qos_t, TrafficCategory *> category_map;

	/// Category to be used when classifier returns no category
	qos_t default_category;

	/// TUN file descriptor
	int fd;

	long _group_id;      ///< it is the MAC layer group id received through msg_link_up
	long _tal_id;        ///< it is the MAC layer MAC id received through msg_link_up

	/// The type of satellite
	string _satellite_type;

	bool tun_configuration();

	/// State of the satellite link
	enum
	{
		link_down,
		link_up
	} state;

	spot_id_t group_id;   ///< it is the MAC layer group id received through msg_link_up
	tal_id_t tal_id;      ///< it is the MAC layer MAC id received through msg_link_up

	/// whether we handle a TAP interface or a TUN interface
	bool is_tap;

	/// the statistics period
	unsigned int stats_period;

	/// statistic timer
	event_id_t stats_timer;
};

class BlockLanAdaptationTal: public BlockLanAdaptation
{
 public:

	BlockLanAdaptationTal(const string &name, string lan_iface):
		BlockLanAdaptation(name, terminal, lan_iface)
	{};
};

class BlockLanAdaptationGw: public BlockLanAdaptation
{
 public:

	BlockLanAdaptationGw(const string &name, string lan_iface):
		BlockLanAdaptation(name, gateway, lan_iface)
	{};
};

#endif

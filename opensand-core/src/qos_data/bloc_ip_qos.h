/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file bloc_ip_qos.h
 * @brief Interface between Traffic Classifier in Linux kernel and OpenSAND
 *
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Nicol <julien.nicol@b2i-toulouse.com>
 */

#ifndef BLOC_IP_QOS_H
#define BLOC_IP_QOS_H


#include "IpPacket.h"
#include "Ipv4Packet.h"
#include "Ipv6Packet.h"
#include "SarpTable.h"
#include "TrafficCategory.h"
#include "ServiceClass.h"
#include "msg_dvb_rcs.h"
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


using namespace std;

/// The debug prefix for the IP QoS block
#define IPQOS_DBG_PREFIX "[IPQOS]"

/// The time between two QoS statistics updates (in ms)
#define STATS_TIMER  1000


/**
 * @class BlocIPQoS
 * @brief Apply IP QoS model to outgoing (UL) traffic; by-pass
 *        incoming (DL) traffic
 */
class BlocIPQoS: public Block
{
 public:

	BlocIPQoS(const string &name, component_t host);
	~BlocIPQoS();


	void writeStats();

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();


 private:

	void getConfig();
	void initSarpTables();
	int terminate();

	// if down
	int onMsgIpFromDn(IpPacket *packet); ///< treatments on reception of pk from lower layer
	int initIpRawSock(ulong family, int *fdRawSock);

	// if up
	int tun_alloc();
	int onMsgIpFromUp(NetSocketEvent *const event);  // treatments on reception of pk from upper layer
	int onMsgIp(IpPacket *packet);  // treatments on reception of pk from upper layer

	SarpTable sarpTable; ///< SARP table

	/// List of service classes ordered by scheduler priority
	vector < ServiceClass > classList;

	/// Whether the bloc has been initialized or not
	component_t host;

	/**
	 * This map associates directly the category identifier (unique) to a ptr
	 * on the category; it allows fast access when a packet coming from upper
	 * layer needs to be inserted in this category
	 */
	map < unsigned short, TrafficCategory * > categoryMap;

	/// Category to be used when classifier returns no category
	long int defaultCategory;

	// TUN file descriptor
	int _tun_fd;

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
	} _state;

	/// statistic timer
	event_id_t stats_timer;
	
	/// output events
	static Event* error_init;
};

class BlocIPQoSTal: public BlocIPQoS
{
 public:

	BlocIPQoSTal(const string &name):
		BlocIPQoS(name, terminal)
	{};
};

class BlocIPQoSGw: public BlocIPQoS
{
 public:

	BlocIPQoSGw(const string &name):
		BlocIPQoS(name, gateway)
	{};
};

#endif

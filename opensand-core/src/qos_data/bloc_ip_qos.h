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

// System includes
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <sstream>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <net/if.h> // for IFNAMSIZ //
#include <linux/if_tun.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/types.h>
#include <arpa/inet.h>
#include <errno.h>

using namespace std;

// Margouilla includes
#include <opensand_margouilla/mgl_bloc.h>
#include <opensand_margouilla/msg_ip.h>

// Project includes
#include <IpPacket.h>
#include <Ipv4Packet.h>
#include <Ipv6Packet.h>
#include <SarpTable.h>
#include <TrafficCategory.h>
#include <ServiceClass.h>
#include <msg_dvb_rcs.h>
#include <opensand_conf/conf.h>

// environment plane
#include "opensand_env_plane/EnvironmentAgent_e.h"
#include "OpenSandCore.h"

/// The debug prefix for the IP QoS block
#define IPQOS_DBG_PREFIX "[IPQOS]"

/// The time between two QoS statistics updates (in ms)
#define STATS_TIMER  1000


/**
 * @class BlocIPQoS
 * @brief Apply IP QoS model to outgoing (UL) traffic; by-pass
 *        incoming (DL) traffic
 */
class BlocIPQoS: public mgl_bloc
{
 public:

	BlocIPQoS(mgl_blocmgr *blocmgr, mgl_id fatherid,
	          const char *name, component_t host);
	~BlocIPQoS();

	// Margouilla event handler
	mgl_status onEvent(mgl_event *event);

	void writeStats();

 private:

	/// Whether the bloc has been initialized or not
	bool _initOk;
	component_t host;

	string _satellite_type;

	void getConfig();
	void initSarpTables();
	int terminate();

	// if down
	int onMsgIpFromDn(IpPacket *packet); ///< treatments on reception of pk from lower layer
	int initIpRawSock(ulong family, int *fdRawSock);

	// if up
	int tun_alloc();
	int onMsgIpFromUp(int fd);  // treatments on reception of pk from upper layer
	int onMsgIp(IpPacket *packet);  // treatments on reception of pk from upper layer

	/// List of service classes ordered by scheduler priority
	vector < ServiceClass > classList;

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

	/// State of the satellite link
	enum
	{
		link_down,
		link_up
	} _state;

	long _group_id;      ///< it is the MAC layer group id received through msg_link_up
	long _tal_id;        ///< it is the MAC layer MAC id received through msg_link_up

	SarpTable sarpTable; ///< SARP table

	bool tun_configuration();

	/// statistic timer
	mgl_timer stats_timer;
};


#endif

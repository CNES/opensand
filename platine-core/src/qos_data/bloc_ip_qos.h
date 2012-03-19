/**
 * @file bloc_ip_qos.h
 * @brief Interface between Traffic Classifier in Linux kernel and Platine
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
#include "platine_margouilla/mgl_bloc.h"
#include "platine_margouilla/msg_ip.h"

// Project includes
#include <IpPacket.h>
#include <Ipv4Packet.h>
#include <Ipv6Packet.h>
#include <SarpTable.h>
#include <TrafficCategory.h>
#include <ServiceClass.h>
#include <msg_dvb_rcs.h>
#include <platine_conf/conf.h>

// environment plane
#include "platine_env_plane/EnvironmentAgent_e.h"

/// The debug prefix for the IP QoS block
#define IPQOS_DBG_PREFIX "[IPQOS]"

/// The time between two QoS statistics updates (in ms)
#define STATS_TIMER  1000

// TC/Netlink includes
extern "C"
{
	#include "gen_stats.h"
	#include "libnetlink.h"
	#include "ll_map.h"
}

/**
 * @class BlocIPQoS
 * @brief Apply IP QoS model to outgoing (UL) traffic; by-pass
 *        incoming (DL) traffic
 */
class BlocIPQoS: public mgl_bloc
{
 public:

	BlocIPQoS(mgl_blocmgr *blocmgr, mgl_id fatherid,
	          const char *name, string host_name);
	~BlocIPQoS();

	// Margouilla event handler
	mgl_status onEvent(mgl_event *event);

	// statistics
	static __u32 *hdlb_pkt_est;  ///< HDLB packets number for each class
	static __u32 *hdlb_drop_est; ///< HDLB dropped packets for each class
	static int print_class(const struct sockaddr_nl *who,
	                       struct nlmsghdr *n,
	                       void *arg);
	static int print_tc_classid(__u32 h);
	static void print_tcstats_attr(struct rtattr *tb[],
	                               const char *prefix,
	                               struct rtattr **xstats,
	                               unsigned int id);
	static void print_tcstats2_attr(struct rtattr *rta,
	                                const char *prefix,
	                                struct rtattr **xstats,
	                                unsigned int id);
	void writeStats();

 private:

	/// Whether the bloc has been initialized or not
	bool _initOk;
	string _host_name;

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
	unsigned short defaultCategory;

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

	// Statistics
	const static double tick_in_usec = 1;
	rtnl_handle rth;     ///< rtnetlink socket to communicate with TC in Linux kernel

	long tc_core_tick2usec(long tick) { return (long) (tick/tick_in_usec); }
	int tc_class_list(struct rtnl_handle *rth, const char *dev);
	static int HexToDec(int hex);
};


#endif

/**
 * @file bloc_ip_qos.cpp
 * @brief Interface between Traffic Classifier in Linux kernel and Platine
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Nicol <julien.nicol@b2i-toulouse.com>
 */

#include "bloc_ip_qos.h"

// debug
#define DBG_PACKAGE PKG_QOS_DATA
#include "platine_conf/uti_debug.h"


extern T_ENV_AGENT EnvAgent;

/// The default LABEL to associate with one IP packet if no MAC ID is found
const int C_DEFAULT_LABEL = 255;

// init HDLB counters
__u32 *BlocIPQoS::hdlb_pkt_est = NULL;
__u32 *BlocIPQoS::hdlb_drop_est = NULL;


/**
 * constructor
 */
BlocIPQoS::BlocIPQoS(mgl_blocmgr *blocmgr, mgl_id fatherid,
                     const char *name, string host_name):
	mgl_bloc(blocmgr, fatherid, name),
	sarpTable()
{
	this->_initOk = false;
	// group & TAL id
	this->_group_id = -1;
	this->_tal_id = -1;
	this->_host_name = host_name;
	this->_satellite_type = "";

	// link state
	this->_state = link_down;
}


/**
 * destructor : Free all resources
 */
BlocIPQoS::~BlocIPQoS()
{
	// close rtnetlink socket
	ll_close_map();
	rtnl_close(&rth);

	// destroy HDLB counters
	if(BlocIPQoS::hdlb_pkt_est != NULL)
	{
		delete[] BlocIPQoS::hdlb_pkt_est;
		BlocIPQoS::hdlb_pkt_est = NULL;
	}
	if(BlocIPQoS::hdlb_drop_est != NULL)
	{
		delete[] BlocIPQoS::hdlb_drop_est;
		BlocIPQoS::hdlb_drop_est = NULL;
	}

	// close TUN file descriptor
	close(this->_tun_fd);

	// free some ressources of IPQoS block
	this->terminate();
}

/**
 * mgl events handler
 *
 * @param event event delivered to the bloc
 */
mgl_status BlocIPQoS::onEvent(mgl_event *event)
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[onEvent]";
	mgl_status status = mgl_ok;
	IpPacket *ip_packet;
	string str;

	if(MGL_EVENT_IS_INIT(event))
	{
		std::basic_ostringstream < char >cmd;
		int ret;

		// retrieve bloc config
		this->getConfig();

		// create TUN virtual interface
		this->_tun_fd = -1;
		this->_tun_fd = tun_alloc();
		if(this->_tun_fd < 0)
		{
			UTI_ERROR("%s error in creating TUN interface\n", FUNCNAME);
			return mgl_ko;
		}

		// add file descriptor for TUN interface
		if(this->addFd(this->_tun_fd) == mgl_ko)
		{
			UTI_ERROR("%s failed to register TUN handle fd\n", FUNCNAME);
			return mgl_ko;
		}

		UTI_INFO("%s TUN handle with fd %d initialized\n",
		         FUNCNAME, this->_tun_fd);

		// initialize some statistics values
		BlocIPQoS::hdlb_pkt_est = new __u32[categoryMap.size()];
		BlocIPQoS::hdlb_drop_est = new __u32[categoryMap.size()];
		if(BlocIPQoS::hdlb_pkt_est == NULL || BlocIPQoS::hdlb_drop_est == NULL)
		{
			UTI_ERROR("%s memory allocation for HDLB statistics failed\n",
			          FUNCNAME);
			return mgl_ko;
		}
		bzero(BlocIPQoS::hdlb_pkt_est, sizeof(int) * categoryMap.size());
		bzero(BlocIPQoS::hdlb_drop_est, sizeof(int) * categoryMap.size());

		// set statistics timer
		this->setTimer(this->stats_timer, STATS_TIMER);

		// open rtnetlink socket for statistics
		if(rtnl_open(&rth, 0) < 0)
		{
			UTI_ERROR("%s cannot open rtnetlink socket for stats\n", FUNCNAME);
			return mgl_ko;
		}

		// initialise ll_map and get device index for statistics
		ll_init_map(&rth);

		// list all HDLB classes and display their stats
		ret = this->tc_class_list(&rth, "platine");
		if(ret != 0)
		{
			UTI_ERROR("%s stats output failed with code %d", FUNCNAME, ret);
			return mgl_ko;
		}
		this->_initOk = true;
	}
	else if(!this->_initOk)
	{
		UTI_ERROR("%s encapsulation bloc not initialized, ignore "
		          "non-init event\n", FUNCNAME);
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		int ret;

		// is it the timer for the statistics ?
		if((mgl_timer) event->event.timer.id == this->stats_timer)
		{
			// list all HDLB classes and display their stats
			ret = this->tc_class_list(&rth, "platine");
			if(ret != 0)
				UTI_ERROR("%s stats output failed with code %d", FUNCNAME, ret);

			// rearm the timer
			this->setTimer(this->stats_timer, STATS_TIMER);
		}
		else
		{
			UTI_ERROR("%s unknown timer expired\n", FUNCNAME);
			status = mgl_ko;
		}
	}
	else if(MGL_EVENT_IS_FD(event))
	{
		// input data available on TUN handle

		if(MGL_EVENT_FD_GET_FD(event) == this->_tun_fd)
		{
			this->onMsgIpFromUp(this->_tun_fd);
		}
		else
		{
			UTI_ERROR("%s data received on unknown socket %ld\n",
			          FUNCNAME, MGL_EVENT_FD_GET_FD(event));
			status = mgl_ko;
		}
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		if(MGL_EVENT_MSG_IS_TYPE(event, msg_link_up))
		{
			T_LINK_UP *link_up_msg;

			// 'link is up' message advertised

			link_up_msg = (T_LINK_UP *) MGL_EVENT_MSG_GET_BODY(event);
			UTI_DEBUG("%s link up message received (group = %ld, tal = %ld)\n",
			          FUNCNAME, link_up_msg->group_id, link_up_msg->tal_id);

			if(this->_state == link_up)
			{
				UTI_INFO("%s duplicate link up msg\n", FUNCNAME);
			}
			else
			{
				// save group id and TAL id sent by MAC layer
				this->_group_id = link_up_msg->group_id;
				this->_tal_id = link_up_msg->tal_id;
				this->_state = link_up;
			}

			delete link_up_msg;
		}
		else if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer() &&
		        MGL_EVENT_MSG_IS_TYPE(event, msg_ip))
		{
			UTI_DEBUG("%s IP packet received from lower layer\n", FUNCNAME);

			ip_packet = (IpPacket *) MGL_EVENT_MSG_GET_BODY(event);

			if(this->_state != link_up)
			{
				UTI_INFO("%s IP packets received from lower layer, but link is down "
				         "=> drop packets\n", FUNCNAME);
				delete ip_packet;
			}
			else
			{
				if(this->onMsgIpFromDn(ip_packet) < 0)
					status = mgl_ko;
			}
		}
		else
		{
			UTI_ERROR("%s unknown message received from bloc %ld\n",
			          FUNCNAME, MGL_EVENT_MSG_GET_SRCBLOC(event));
			status = mgl_ko;
		}
	}
	else
	{
		UTI_ERROR("%s unknown event (type %ld) received\n",
		          FUNCNAME, event->type);
		status = mgl_ko;
	}

	return status;
}

/**
 * Manage an IP packet received from lower layer:
 *  - build the TUN header with appropriate protocol identifier
 *  - write TUN header + IP packet to TUN interface
 *
 * @param packet  IP packet received from lower layer
 * @return        0 ok, -1 failed
 */
int BlocIPQoS::onMsgIpFromDn(IpPacket *packet)
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[onMsgIpFromDn]";
	int status = 0;
	int ip_length;
	unsigned char *pkt;
	unsigned char *flags;
	static unsigned char flags4[4] = { 0, 0, 8, 0 };
	static unsigned char flags6[4] = { 0, 0, 134, 221 };

	// check IP packet validity
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s IP packet is not valid\n", FUNCNAME);
		status = -1;
		goto drop;
	}
	packet->addTrace(HERE());

	// check if packet should be forwarded
	if(this->_host_name == "GW" &&
	   this->_satellite_type == TRANSPARENT_SATELLITE)
	{
		// check if destination is GW
		if(packet->talId() != 0)
		{
			UTI_DEBUG_L3("%s IP packet is not for GW, forward it\n", FUNCNAME);
			status = this->onMsgIp(packet);
			goto quit;
		}
		UTI_DEBUG_L3("%s IP packet is for GW\n", FUNCNAME);
	}

	// allocate memory for IP data
	ip_length = packet->totalLength();

	pkt = (unsigned char *) calloc(ip_length + 4, sizeof(unsigned char));

	if(pkt == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for sending IP data on TUN\n",
		          FUNCNAME);
		status = -1;
		goto drop;
	}

	bzero(pkt, ip_length + 4);
	memcpy(pkt + 4, packet->data().c_str(), ip_length);

	// find the protocol flag according to IP version
	switch(packet->version())
	{
		case 4:
			flags = flags4;
			break;
		case 6:
			flags = flags6;
			break;
		default:
			UTI_ERROR("IP packet (version %d) received from lower bloc and "
			          "dropped\n", packet->version());
			status = -1;
			goto clean_and_drop;
	}

	// add the protocol flag in the TUN header
	pkt[0] = flags[0];
	pkt[1] = flags[1];
	pkt[2] = flags[2];
	pkt[3] = flags[3];

	// write data on TUN device
	if(write(this->_tun_fd, pkt, ip_length + 4) < 0)
	{
		UTI_ERROR("%s: Unable to write data on tun interface (errno: %d)\n",
		          FUNCNAME, errno);
		status = -1;
		goto clean_and_drop;
	}

	UTI_DEBUG("%s: Packet IPv%d received from lower layer & forwarded to "
	          "network\n", FUNCNAME, packet->version());

clean_and_drop:
	free(pkt);
drop:
	delete packet;
quit:
	return status;
}

/**
 * Manage an IP packet received from upper layer:
 *  - read data from TUN interface
 *  - create an IP packet with data
 *
 * @param fd  file descriptor for the TUN device
 * @return    0 ok, -1 failed, -2 if packet dropped
 */
int BlocIPQoS::onMsgIpFromUp(int fd)
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[onMsgIpFromUp]";
	int status = 0;

	unsigned char buf[TUNTAP_BUFSIZE + 4];
	unsigned char *data;
	unsigned int length;

	IpPacket *ip_packet;

	// read IP data received on tun interface
	length = read(fd, buf, TUNTAP_BUFSIZE);
	data = buf + 4;
	length -= 4;

	if(length > TUNTAP_BUFSIZE)
	{
		UTI_ERROR("%s Received length from tun: %d greater than %d", FUNCNAME,
		          length, TUNTAP_BUFSIZE);
		status = -1;
		goto drop;
	}
	else if(length == 0)
	{
		UTI_ERROR("%s 0 size packet", FUNCNAME);
		status = -1;
		goto drop;
	}
	else if(length < 0)
	{
		UTI_ERROR("%s Error in receiving data from TUN", FUNCNAME);
		status = -1;
		goto drop;
	}

	if(this->_state != link_up)
	{
		UTI_INFO("%s IP packets received from tun, but link is down "
		         "=> drop packets\n", FUNCNAME);
		goto drop;
	}

	// create IP packet from data
	switch(IpPacket::version(data, length))
	{
		case 4:
			ip_packet = new Ipv4Packet(data, length);
			break;
		case 6:
			ip_packet = new Ipv6Packet(data, length);
			break;
		default:
			ip_packet = NULL;
	}

	if(ip_packet == NULL || !ip_packet->isValid())
	{
		UTI_ERROR("%s cannot create IP packet, drop %d bytes of data\n",
		          FUNCNAME, length);

		if(ip_packet != NULL)
			delete ip_packet;

		status = -1;
		goto drop;
	}

	ip_packet->addTrace(HERE());

	status = this->onMsgIp(ip_packet);

drop:
	return status;
}

/**
 * Manage an IP packet handled by this bloc before sending it to lower layers:
 *  - find assigned QoS, TAL ID and MAC ID
 *  - send IP packet to lower layer
 *
 * @param ip_packet the IP packet
 * @return          0 ok, -1 failed, -2 if packet dropped
 */

int BlocIPQoS::onMsgIp(IpPacket *ip_packet)
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[onMsgIp]";
	int status = 0;

	int traffic_category;
	map < unsigned short, TrafficCategory * >::iterator foundCategory;

	IpAddress *ip_addr;
	long mac_id; // MAC id found in the SARP table
	int tal_id; // tal is found in the SARP table

	mgl_msg *lp_msg;

	// set QoS:
	//  - retrieve the QoS set by TC using DCSP
	//  - if unknown category, put packet in the default category
	//  - assign QoS to the IP packet
	traffic_category = (int) ip_packet->trafficClass();

	foundCategory = categoryMap.find(traffic_category);
	if(foundCategory == categoryMap.end())
	{
		UTI_DEBUG("%s: category %d unknown; IP packet goes to default "
		          "category %d\n", FUNCNAME, traffic_category, defaultCategory);

		foundCategory = categoryMap.find(defaultCategory);
		if(foundCategory == categoryMap.end())
		{
			UTI_ERROR("%s: default category not defined\n", FUNCNAME);
			goto drop;
		}
	}
	else
	{
		UTI_DEBUG("%s: IP packet goes to category %d\n", FUNCNAME,
		          traffic_category);
	}

	ip_packet->setQos(foundCategory->second->svcClass->macQueueId);

	// set the TAL id (MAC is locally solved thanks to a mapping table)
	ip_addr = ip_packet->destAddr();
	UTI_DEBUG_L3("%s IPv%d destination address = %s\n",
	             FUNCNAME, ip_packet->version(), ip_addr->str().c_str());

	tal_id = this->sarpTable.getTalByIp(ip_addr);
	if(tal_id < 0)
	{
		// tal id not found, fall back to default
		UTI_ERROR("%s IP dest addr not found in SARP table \n", FUNCNAME);
		status = -1;
		goto drop;
	}

	UTI_DEBUG_L3("%s talID in SARP Table: %d \n", FUNCNAME, tal_id);
	ip_packet->setTalId(tal_id);
	UTI_DEBUG_L3("%s talID: %ld \n", FUNCNAME, ip_packet->talId());

	// set the MAC id (MAC is locally solved thanks to a mapping table)
	ip_addr = ip_packet->destAddr();
	UTI_DEBUG_L3("%s IPv%d destination address = %s\n",
	             FUNCNAME, ip_packet->version(), ip_addr->str().c_str());

	mac_id = this->sarpTable.getMacByIp(ip_addr);
	if(mac_id < 0)
	{
		// MAC id not found, fall back to default
		UTI_DEBUG("%s IP dest addr not found in SARP table => sending to "
		          "default label\n", FUNCNAME);
		mac_id = C_DEFAULT_LABEL;
	}

	ip_packet->setMacId(mac_id);

	// create the Margouilla message with IP packet as data
	lp_msg = this->newMsgWithBodyPtr(msg_ip, ip_packet, sizeof(ip_packet));

	if(!lp_msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		delete ip_packet; // delete the IP packet
		status = -1;
		goto drop;
	}

	// send the message to the lower layer
	this->sendMsgTo(getLowerLayer(), lp_msg);

drop:
	return status;
}

/**
 * Create TUN interface
 *
 * @return  TUN file descriptor
 */
int BlocIPQoS::tun_alloc()
{
	struct ifreq ifr;
	int fd, err;

	fd = open("/dev/net/tun", O_RDWR);
	if(fd < 0)
		return fd;

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */

	/* create TUN interface */
	snprintf(ifr.ifr_name, IFNAMSIZ, "platine");
	ifr.ifr_flags = IFF_TUN;

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if(err < 0)
	{
		close(fd);
		return err;
	}

	return fd;
}


/**
 * Functions below have statistic purposes
 */

/**
 * Get class id from a handle
 *
 * @return  The class id, -1 if an error occurs
 */
int BlocIPQoS::print_tc_classid(__u32 h)
{
	if(h == TC_H_ROOT)
		return 0;
	else if(h == TC_H_UNSPEC)
		return -1;
	else if(TC_H_MIN(h) == 0)
		return -1;

	return TC_H_MIN(h);
}

/**
 * Record HDLB class statistics (new method)
 */
void BlocIPQoS::print_tcstats2_attr(struct rtattr *rta,
                                    const char *prefix,
                                    struct rtattr **xstats,
                                    unsigned int id)
{
	struct rtattr *tbs[TCA_STATS_MAX + 1];

	parse_rtattr(tbs, TCA_STATS_MAX, (rtattr *) RTA_DATA(rta), RTA_PAYLOAD(rta));


	// Class rate (rate is computed from the transmitted bytes to have
	// better granularity than the rate computed in kernel)
	if(tbs[TCA_STATS_BASIC] && BlocIPQoS::hdlb_drop_est != NULL)
	{
		struct gnet_stats_basic bs = { 0 };
		memcpy(&bs, RTA_DATA(tbs[TCA_STATS_BASIC]),
		       MIN(RTA_PAYLOAD(tbs[TCA_STATS_BASIC]), sizeof(bs)));
		ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_OUTPUT_HDLB_RATE, id,
		                       (int)((bs.bytes - BlocIPQoS::hdlb_pkt_est[id - 1])
		                       / STATS_TIMER * 8000));
		BlocIPQoS::hdlb_pkt_est[id - 1] = bs.bytes;
	}

	// number of dropped packets
	if(tbs[TCA_STATS_QUEUE] && BlocIPQoS::hdlb_drop_est != NULL)
	{
		struct gnet_stats_queue q = { 0 };
		memcpy(&q, RTA_DATA(tbs[TCA_STATS_QUEUE]),
		       MIN(RTA_PAYLOAD(tbs[TCA_STATS_QUEUE]), sizeof(q)));
		ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_OUTPUT_HDLB_DROPS, id,
		                       1000 * (q.drops - BlocIPQoS::hdlb_drop_est[id - 1])
		                       / STATS_TIMER);
		BlocIPQoS::hdlb_drop_est[id - 1] = q.drops;
	}

	// number of packets in the queue
	if(tbs[TCA_STATS_QUEUE])
	{
		struct gnet_stats_queue q = { 0 };
		memcpy(&q, RTA_DATA(tbs[TCA_STATS_QUEUE]),
		       MIN(RTA_PAYLOAD(tbs[TCA_STATS_QUEUE]), sizeof(q)));
		ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_OUTPUT_HDLB_BACKLOG,
		                       id, q.qlen);
	}
}

/**
 * Record HDLB class statistics (backward compatible version)
 *
 * @see print_tcstats2_attr
 */
void BlocIPQoS::print_tcstats_attr(struct rtattr *tb[],
                                   const char *prefix,
                                   struct rtattr **xstats,
                                   unsigned int id)
{
	if(tb[TCA_STATS2])
	{
		BlocIPQoS::print_tcstats2_attr(tb[TCA_STATS2], prefix, xstats, id);
	}
	else if(tb[TCA_STATS] && BlocIPQoS::hdlb_drop_est != NULL) // backward compatibility
	{
		struct tc_stats st;

		/* handle case where kernel returns more/less than we know about */
		memset(&st, 0, sizeof(st));
		memcpy(&st, RTA_DATA(tb[TCA_STATS]), MIN(RTA_PAYLOAD(tb[TCA_STATS]), sizeof(st)));
		ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_OUTPUT_HDLB_RATE, id,
		                       (int)((st.bytes - BlocIPQoS::hdlb_pkt_est[id - 1])
		                       / STATS_TIMER * 8000));
		BlocIPQoS::hdlb_pkt_est[id - 1] = st.bytes;

		if(st.qlen || st.backlog)
		{
			if(st.backlog)
				ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_OUTPUT_HDLB_BACKLOG,
				                       id, st.backlog);
			if(st.qlen)
				ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_OUTPUT_HDLB_BACKLOG,
				                       id, st.qlen);
		}
	}
}

/**
 * Translate a number from an hexadecimal representation to a decimal value
 *
 * @return  The decimal value corresponding to the hexadecimal representation
 */
int BlocIPQoS::HexToDec(int hex)
{
	int res = 0;
	int weight = 1;

	while(hex > 15)
	{
		res += hex % 16 * weight;
		hex = hex / 16;
		weight = weight * 10;
	}

	res = hex * weight;

	return res;
}

/**
 * Print HDLB class statistics
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocIPQoS::print_class(const struct sockaddr_nl *who,
                           struct nlmsghdr *n,
                           void *arg)
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX " [print_class]";
	struct tcmsg *t = (tcmsg *) (NLMSG_DATA(n)); // Netlink message data
	int len = n->nlmsg_len;         // Netlink message data length
	struct rtattr * tb[TCA_MAX+1];  // Netlink message's routing attributes
	struct rtattr *xstats = NULL;   // Extended hdlb statistics
	unsigned int id = 0;            // HDLB class id

	// Check message type
	if(n->nlmsg_type != RTM_NEWTCLASS && n->nlmsg_type != RTM_DELTCLASS)
	{
		UTI_ERROR("%s Not a class\n", FUNCNAME);
		return 0;
	}

	// Check message length
	len -= NLMSG_LENGTH(sizeof(*t));
	if(len < 0)
	{
		UTI_ERROR("%s Wrong len %d\n", FUNCNAME, len);
		return -1;
	}

	// Get class attributes from netlink message
	memset(tb, 0, sizeof(tb));
	parse_rtattr(tb, TCA_MAX, TCA_RTA(t), len);

	// Check class type
	if(tb[TCA_KIND] == NULL)
	{
		UTI_ERROR("%s NULL kind\n", FUNCNAME);
		return -1;
	}
	if(strcmp((char *) (RTA_DATA(tb[TCA_KIND])), "hdlb") != 0)
		return 0;

	// Get class id and translate it for the display manager
	if(t->tcm_handle)
	{
		int id_tc = 0;
		if((id_tc = BlocIPQoS::print_tc_classid(t->tcm_handle)) == -1)
		{
			UTI_INFO("%s class h%u has no id, cannot record its stats\n",
			         FUNCNAME, t->tcm_handle);
			return 0;
		}

		if(id_tc < 100)
			return 0;

		id = (unsigned int) BlocIPQoS::HexToDec(id_tc) / 100;
	}

	// Record class general statistics
	BlocIPQoS::print_tcstats_attr(tb, " ", &xstats, id);

	return 0;
}

/**
 * List all HDLB classes and send rtnetlink requests
 *
 * @return  0 in case of success, 1 otherwise
 */
int BlocIPQoS::tc_class_list(struct rtnl_handle *rth, const char *dev)
{
	struct tcmsg t; // TC message

	// Set up the message
	memset(&t, 0, sizeof(t));
	t.tcm_family = AF_UNSPEC;

	if(dev)
	{
		if((t.tcm_ifindex = ll_name_to_index(dev)) == 0)
		{
			UTI_ERROR("Cannot find device \"%s\"\n", dev);
			return 1;
		}
	}

	// Send an rtnetlink dump request
	if(rtnl_dump_request(rth, RTM_GETTCLASS, &t, sizeof(t)) < 0)
	{
		UTI_ERROR("Cannot send dump request\n");
		return 1;
	}

	// Filter the request to get wanted statistics
	if(rtnl_dump_filter(rth, BlocIPQoS::print_class, (char*)dev, NULL, NULL) < 0)
	{
		UTI_ERROR("Dump terminated\n");
		return 1;
	}

	return 0;
}


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 * Copyright © 2012 CNES
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
 * @file bloc_ip_qos.cpp
 * @brief Interface between Traffic Classifier in Linux kernel and OpenSAND
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "bloc_ip_qos.h"

// debug
#define DBG_PACKAGE PKG_QOS_DATA
#include "opensand_conf/uti_debug.h"


extern T_ENV_AGENT EnvAgent;

/// The default LABEL to associate with one IP packet if no MAC ID is found
const int C_DEFAULT_LABEL = 255;


/**
 * constructor
 */
BlocIPQoS::BlocIPQoS(mgl_blocmgr *blocmgr, mgl_id fatherid,
                     const char *name, t_component host):
	mgl_bloc(blocmgr, fatherid, name),
	sarpTable()
{
	this->_initOk = false;
	// group & TAL id
	this->_group_id = -1;
	this->_tal_id = -1;
	this->host = host;
	this->_satellite_type = "";

	// link state
	this->_state = link_down;
}


/**
 * destructor : Free all resources
 */
BlocIPQoS::~BlocIPQoS()
{
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

		// retrieve bloc config
		this->getConfig();

		// create TUN virtual interface
		this->_tun_fd = -1;
		this->_tun_fd = tun_alloc();
		if(this->_tun_fd < 0)
		{
			UTI_ERROR("%s error in creating TUN interface\n", FUNCNAME);
			ENV_AGENT_Error_Send(&EnvAgent, C_ERROR_CRITICAL, 0, 0,
			                     C_ERROR_INIT_COMPO);
			return mgl_ko;
		}

		// add file descriptor for TUN interface
		if(this->addFd(this->_tun_fd) == mgl_ko)
		{
			UTI_ERROR("%s failed to register TUN handle fd\n", FUNCNAME);
			ENV_AGENT_Error_Send(&EnvAgent, C_ERROR_CRITICAL, 0, 0,
			                     C_ERROR_INIT_COMPO);
			return mgl_ko;
		}

		UTI_INFO("%s TUN handle with fd %d initialized\n",
		         FUNCNAME, this->_tun_fd);
		this->_initOk = true;
	}
	else if(!this->_initOk)
	{
		UTI_ERROR("%s IP-QOS bloc not initialized, ignore "
		          "non-init event\n", FUNCNAME);
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
	if(this->host == gateway &&
	   this->_satellite_type == TRANSPARENT_SATELLITE)
	{
		IpAddress *ip_addr;
		uint8_t tal_id;

		// get destination Tal ID from IP information because
		// packet tal_id could be wrong
		ip_addr = packet->dstAddr();
		tal_id = this->sarpTable.getTalByIp(ip_addr);

		// check if destination is GW
		if(tal_id != DVB_GW_MAC_ID)
		{
			UTI_DEBUG_L3("%s IP packet is not for GW, forward it\n", FUNCNAME);
			status = this->onMsgIp(packet);
			goto quit;
		}
		UTI_DEBUG_L3("%s IP packet is for GW\n", FUNCNAME);
	}

	// allocate memory for IP data
	ip_length = packet->getTotalLength();

	pkt = (unsigned char *) calloc(ip_length + 4, sizeof(unsigned char));

	if(pkt == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for sending IP data on TUN\n",
		          FUNCNAME);
		status = -1;
		goto drop;
	}

	bzero(pkt, ip_length + 4);
	memcpy(pkt + 4, packet->getData().c_str(), ip_length);

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

	// set the source terminal ID out of onMsgIp to avoid overwriting it by the
	// GW when this function is called for forwarding
	ip_packet->setSrcTalId(this->_tal_id);
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
	uint8_t tal_id; // tal is found in the SARP table

	mgl_msg *lp_msg;

	// set QoS:
	//  - retrieve the QoS set by TC using DSCP
	//  - if unknown category, put packet in the default category
	//  - assign QoS to the IP packet
	traffic_category = (int) ip_packet->diffServCodePoint();

	foundCategory = categoryMap.find(traffic_category);
	if(foundCategory == categoryMap.end())
	{
		UTI_DEBUG("%s: category %d unknown; IP packet goes to default "
		          "category %ld\n", FUNCNAME, traffic_category, defaultCategory);

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

	if(this->host != gateway && this->_satellite_type == TRANSPARENT_SATELLITE)
	{
		// ST in transparent mode:
		// DST Tal Id = GW
		// SRC Tal Id = ST Tal Id
		ip_packet->setDstTalId(DVB_GW_MAC_ID);
	}
	else
	{
		// Other modes
		// DST Tal Id = Tal Id(ip_dst)
		// SRC Tal Id = Host Tal Id
		ip_addr = ip_packet->dstAddr();
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
		ip_packet->setDstTalId(tal_id);
	}

	UTI_DEBUG_L3("%s Src TAL ID: %u \n", FUNCNAME, ip_packet->getSrcTalId());
	UTI_DEBUG_L3("%s Dst TAL ID: %u \n", FUNCNAME, ip_packet->getDstTalId());


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
	snprintf(ifr.ifr_name, IFNAMSIZ, "opensand");
	ifr.ifr_flags = IFF_TUN;

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if(err < 0)
	{
		close(fd);
		return err;
	}

	return fd;
}



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

// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_QOS_DATA
#include "opensand_conf/uti_debug.h"

#include "bloc_ip_qos.h"

#include <cstdio>

// output events
Event* BlocIPQoS::error_init = NULL;

/// The default LABEL to associate with one IP packet if no MAC ID is found
const int C_DEFAULT_LABEL = 255;

/**
 * constructor
 */
BlocIPQoS::BlocIPQoS(const string &name, component_t host):
	Block(name),
	sarpTable(),
	host(host),
	_group_id(-1),
	_tal_id(-1),
	_satellite_type(),
	_state(link_down)
{
	// TODO we need a mutex here because some parameters may be used in upward and downward
	this->enableChannelMutex();
	if(error_init == NULL)
	{
		error_init = Output::registerEvent("bloc_ip_qos:init", LEVEL_ERROR);
	}
}


/**
 * destructor : Free all resources
 */
BlocIPQoS::~BlocIPQoS()
{
	// close TUN file descriptor TODO this should be done in event
//	close(this->_tun_fd);

	// free some ressources of IPQoS block
	this->terminate();
}

bool BlocIPQoS::onInit()
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[onInit]";
	std::basic_ostringstream < char >cmd;

	// retrieve bloc config
	this->getConfig();

	// create TUN virtual interface
	this->_tun_fd = -1;
	this->_tun_fd = tun_alloc();
	if(this->_tun_fd < 0)
	{
		UTI_ERROR("%s error in creating TUN interface\n", FUNCNAME);
		Output::sendEvent(error_init, "%s error in creating TUN interface\n",
		                  FUNCNAME);
		return false;
	}

	// add file descriptor for TUN interface
	this->downward->addFileEvent("tun", this->_tun_fd, TUNTAP_BUFSIZE + 4);

	UTI_INFO("%s TUN handle with fd %d initialized\n",
			 FUNCNAME, this->_tun_fd);

return true;
}

bool BlocIPQoS::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_file:
			// input data available on TUN handle
			this->onMsgIpFromUp((NetSocketEvent *)event);
			break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlocIPQoS::onUpwardEvent(const RtEvent *const event)
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[onEvent]";
	IpPacket *ip_packet;
	string str;

	switch(event->getType())
	{
		case evt_message:
			if(((MessageEvent *)event)->getMessageType() == msg_link_up)
			{
				T_LINK_UP *link_up_msg;

				// 'link is up' message advertised

				link_up_msg = (T_LINK_UP *)((MessageEvent *)event)->getData();
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
				break;
			}
			UTI_DEBUG("%s IP packet received from lower layer\n", FUNCNAME);

			ip_packet = (IpPacket *)((MessageEvent *)event)->getData();

			if(this->_state != link_up)
			{
				UTI_INFO("%s IP packets received from lower layer, but link is down "
				         "=> drop packets\n", FUNCNAME);
				delete ip_packet;
				return false;
			}
			if(this->onMsgIpFromDn(ip_packet) < 0)
			{
				return false;
			}
			break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
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

	IpAddress *ip_addr;
	uint8_t tal_id;
	// get destination Tal ID from IP information because
	// packet tal_id could be wrong
	ip_addr = packet->dstAddr();
	tal_id = this->sarpTable.getTalByIp(ip_addr);
	
	// check if the packet should be read
	if(tal_id == BROADCAST_TAL_ID || tal_id == this->_tal_id)
	{
		UTI_DEBUG("%s: Packet IPv%d received from lower layer & shloud be read\n", 
				FUNCNAME, packet->version());
		
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
				free(pkt);
				goto drop;
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
			free(pkt);
			goto drop;
		}

		UTI_DEBUG("%s: Packet IPv%d received from lower layer & forwarded to "
				  "network layer\n", FUNCNAME, packet->version());
		free(pkt);
	}

	// check if packet should be forwarded
	if(this->host == gateway &&
	   this->_satellite_type == TRANSPARENT_SATELLITE &&
	   tal_id != DVB_GW_MAC_ID)
	{
		UTI_DEBUG("%s: Packet should be forwarded (multicast/broadcast or"
		          " unicast not for GW)", FUNCNAME);
		status = this->onMsgIp(packet);
		goto quit;
	}

drop:
	delete packet;
quit:
	return status;
}

/**
 * Manage an IP packet received from upper layer:
 *  - get data from event
 *  - create an IP packet with data
 *
 * @param event  The event on TUN interface
 * @return    0 ok, -1 failed, -2 if packet dropped
 */
int BlocIPQoS::onMsgIpFromUp(NetSocketEvent *const event)
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[onMsgIpFromUp]";
	int status = 0;

	unsigned char data[TUNTAP_BUFSIZE];
	unsigned int length;

	IpPacket *ip_packet;

	// read IP data received on tun interface
	// we need to memcpy as start pointer is not the same
	length = event->getSize() - 4;
	memcpy(data, event->getData() + 4, length);

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


	if(!this->sendDown((void **)&ip_packet))
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		delete ip_packet; // delete the IP packet
		status = -1;
		goto drop;
	}

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



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
 * @file Ip.cpp
 * @brief IP lan adaptation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Ip.h"
#include "SarpTable.h"
#include "TrafficCategory.h"


#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_QOS_DATA
#include <opensand_conf/uti_debug.h>
#include <opensand_conf/ConfigurationFile.h>

#include <vector>
#include <map>


#define SECTION_MAPPING		"ip_qos"
#define MAPPING_LIST		"categories"
#define MAPPING_IP_DSCP		"dscp"
#define MAPPING_MAC_PRIO	"mac_prio"
#define MAPPING_MAC_NAME	"mac_name"
#define KEY_DEF_CATEGORY	"default_dscp"
#define CONF_IP_FILE "/etc/opensand/plugins/ip.conf"


Ip::Ip():
	LanAdaptationPlugin(NET_PROTO_IP)
{
}

Ip::Context::Context(LanAdaptationPlugin &plugin):
	LanAdaptationContext(plugin)
{
	ConfigurationFile config;

	if(config.loadConfig(CONF_IP_FILE) < 0)
	{
		UTI_ERROR("failed to load config file '%s'", CONF_IP_FILE);
		return;
	}

	this->handle_net_packet = true;
	if(!this->initTrafficCategories(config))
	{
		UTI_ERROR("cannot Initialize traffic categories\n");
	}

	config.unloadConfig();
}

Ip::Context::~Context()
{
}

NetBurst *Ip::Context::encapsulate(NetBurst *burst,
                                   std::map<long, int> &UNUSED(time_contexts))
{
	NetBurst *ip_packets = NULL;
	NetBurst::iterator packet;

	// create an empty burst of IP packets
	ip_packets = new NetBurst();
	if(ip_packets == NULL)
	{
		UTI_ERROR("cannot allocate memory for burst of IP packets\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		IpPacket *ip_packet;

		// create IP packet from data
		switch(IpPacket::version((*packet)->getData()))
		{
			case 4:
				ip_packet = new Ipv4Packet((*packet)->getData());
				break;
			case 6:
				ip_packet = new Ipv6Packet((*packet)->getData());
				break;
			default:
				UTI_ERROR("unknown IP packet version");
				continue;
		}
		UTI_DEBUG("got an IPv%u packet\n",
		          IpPacket::version((*packet)->getData()));
		// check IP packet validity
		if(!ip_packet->isValid())
		{
			UTI_ERROR("IP packet is not valid\n");
			delete ip_packet;
			continue;
		}

		ip_packet->setSrcTalId(this->tal_id);
		if(!this->onMsgIp(ip_packet))
		{
			UTI_ERROR("IP handling failed, drop packet\n");
			continue;
		}
		ip_packets->add(ip_packet);
	}

	// delete the burst and all packets in it
	delete burst;
	return ip_packets;
}


NetBurst *Ip::Context::deencapsulate(NetBurst *burst)
{
	NetBurst *net_packets;
	NetBurst::iterator packet;

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("cannot allocate memory for burst of network packets\n");
		delete burst;
		return false;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		IpPacket *ip_packet;
		IpAddress *ip_addr;
		tal_id_t pkt_tal_id;

		// create IP packet from data
		switch(IpPacket::version((*packet)->getData()))
		{
			case 4:
				ip_packet = new Ipv4Packet((*packet)->getData());
				break;
			case 6:
				ip_packet = new Ipv6Packet((*packet)->getData());
				break;
			default:
				UTI_ERROR("unknown IP packet version");
				continue;
		}
		UTI_DEBUG("got an IPv%u packet\n",
		          IpPacket::version((*packet)->getData()));
		// check IP packet validity
		if(!ip_packet->isValid())
		{
			UTI_ERROR("IP packet is not valid\n");
			delete ip_packet;
			continue;
		}

		// get destination Tal ID from IP information because
		// packet tal_id could be wrong
		ip_addr = ip_packet->dstAddr();
		if(!this->sarp_table->getTalByIp(ip_addr, pkt_tal_id))
		{
			// check default tal_id
			if(pkt_tal_id > BROADCAST_TAL_ID)
			{
				UTI_ERROR("cannot get destination tal ID in SARP table\n");
				delete ip_packet;
				continue;
			}
			else
			{
				// TODO use info or notice once it will not be printed by default
				UTI_DEBUG("cannot find destination tal ID, use default (%u)\n",
				          pkt_tal_id);
			}
		}
		ip_packet->setDstTalId(pkt_tal_id);

		net_packets->add(ip_packet);
	}

	// delete the burst and all packets in it
	delete burst;
	return net_packets;
}


bool Ip::Context::onMsgIp(IpPacket *ip_packet)
{
	int traffic_category;
	map<qos_t, TrafficCategory *>::const_iterator found_category;

	IpAddress *ip_addr;
	tal_id_t pkt_tal_id; // tal is found in the SARP table

	// set QoS:
	//  - retrieve the QoS set by TC using DSCP
	//  - if unknown category/priority, put packet in the default category/priority
	//  - assign QoS/priority to the IP packet
	traffic_category = (int) ip_packet->diffServCodePoint();

	found_category = this->category_map.find(traffic_category);
	if(found_category == this->category_map.end())
	{
		UTI_DEBUG("DSCP %d unknown; IP packet goes to default "
		          "MAC category %d\n", traffic_category, this->default_category);

		found_category = this->category_map.find(this->default_category);
		if(found_category == this->category_map.end())
		{
			UTI_ERROR("default MAC category not defined\n");
			return false;
		}
	}
	else
	{
		UTI_DEBUG("IP packet with DSCP %d goes to MAC category %s with id %u\n", 
		          traffic_category, found_category->second->getName().c_str(),
		          found_category->second->getId());
	}
	ip_packet->setQos(found_category->second->getId());

	if(this->tal_id != GW_TAL_ID && this->satellite_type == TRANSPARENT)
	{
		// ST in transparent mode:
		// DST Tal Id = GW
		// SRC Tal Id = ST Tal Id
		ip_packet->setDstTalId(GW_TAL_ID);
	}
	else
	{
		// Other modes
		// DST Tal Id = Tal Id(ip_dst)
		// SRC Tal Id = Host Tal Id
		ip_addr = ip_packet->dstAddr();
		if(!ip_addr)
		{
			UTI_ERROR("cannot get IP packet address\n");
			return false;
		}
		UTI_DEBUG_L3("IPv%d destination address = %s\n",
					 ip_packet->version(), ip_addr->str().c_str());

		if(!this->sarp_table->getTalByIp(ip_addr, pkt_tal_id))
		{
			// check default tal_id
			if(pkt_tal_id > BROADCAST_TAL_ID)
			{
				// tal id not found
				UTI_ERROR("IP dest addr not found in SARP table\n");
				return false;

			}
			else
			{
				// TODO use info or notice once it will not be printed by default
				UTI_DEBUG("cannot find destination tal ID, use default (%u)\n",
				          pkt_tal_id);
			}
		}

		UTI_DEBUG_L3("talID in SARP Table: %d \n", pkt_tal_id);
		ip_packet->setDstTalId(pkt_tal_id);
	}

	UTI_DEBUG_L3("Src TAL ID: %u \n", ip_packet->getSrcTalId());
	UTI_DEBUG_L3("Dst TAL ID: %u \n", ip_packet->getDstTalId());

	return true;
}

char Ip::Context::getLanHeader(unsigned int pos, NetPacket *packet)
{
	unsigned char ether_type[4] = {0, 0, 0, 0};
	// create IP packet from data
	switch(IpPacket::version(packet->getData()))
	{
		case 4:
			UTI_DEBUG("add IPv4 flags for TUN interface");
			ether_type[2] = (NET_PROTO_IPV4 >> 8) & 0xFF;
			ether_type[3] = (NET_PROTO_IPV4) & 0xFF;
			break;
		case 6:
			UTI_DEBUG("add IPv6 flags for TUN interface");
			ether_type[2] = (NET_PROTO_IPV6 >> 8) & 0xFF;
			ether_type[3] = (NET_PROTO_IPV6) & 0xFF;
			break;
		default:
			return 0;
	}
	if(pos > 3)
	{
		return 0;
	}

	return ether_type[pos];
}


NetPacket *Ip::PacketHandler::build(unsigned char *data, size_t data_length,
                                    uint8_t qos,
                                    uint8_t src_tal_id,
                                    uint8_t dst_tal_id) const
{
	if(IpPacket::version(data, data_length) == 4)
	{
		Ipv4Packet *packet;
		packet = new Ipv4Packet(data, data_length);
		packet->setQos(qos);
		packet->setSrcTalId(src_tal_id);
		packet->setDstTalId(dst_tal_id);
		return packet;
	}
	else if(IpPacket::version(data, data_length) == 6)
	{
		Ipv6Packet *packet;
		packet = new Ipv6Packet(data, data_length);
		packet->setQos(qos);
		packet->setSrcTalId(src_tal_id);
		packet->setDstTalId(dst_tal_id);
		return packet;
	}
	else
	{
		UTI_ERROR("cannot get IP version\n");
		return NULL;
	}
}

bool Ip::Context::initTrafficCategories(ConfigurationFile &config)
{
	int i = 0;
	TrafficCategory *category;
	vector<TrafficCategory *>::iterator cat_iter;
	ConfigurationList category_list;
	ConfigurationList::iterator iter;

	// Traffic flow categories
	if(!config.getListItems(SECTION_MAPPING, MAPPING_LIST,
	                        category_list))
	{
		UTI_ERROR("missing or empty section [%s, %s]\n",
		          SECTION_MAPPING, MAPPING_LIST);
		return false;
	}

	for(iter = category_list.begin(); iter != category_list.end(); iter++)
	{
		long int dscp_value;
		long int mac_queue_prio;
		string mac_queue_name;

		i++;
		// get category id
		if(!config.getAttributeValue(iter, MAPPING_IP_DSCP, dscp_value))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SECTION_MAPPING, MAPPING_LIST,
			          MAPPING_IP_DSCP, i);
			return false;
		}
		// get category name
		if(!config.getAttributeValue(iter, MAPPING_MAC_NAME, mac_queue_name))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SECTION_MAPPING, MAPPING_LIST,
			          MAPPING_MAC_NAME, i);
			return false;
		}
		// get service class
		if(!config.getAttributeValue(iter, MAPPING_MAC_PRIO,
		                             mac_queue_prio))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SECTION_MAPPING, MAPPING_LIST,
			          MAPPING_MAC_PRIO, i);
			return false;
		}

		if(this->category_map.count(dscp_value))
		{
			UTI_ERROR("Traffic category %ld - [%s] rejected: identifier "
			          "already exists for [%s]\n", dscp_value,
			           mac_queue_name.c_str(),
			           this->category_map[dscp_value]->getName().c_str());
			return false;
		}

		category = new TrafficCategory();

		category->setId(mac_queue_prio);
		category->setName(mac_queue_name);
		this->category_map[dscp_value] = category;
	}
	// Get default category
	if(!config.getValue(SECTION_MAPPING, KEY_DEF_CATEGORY,
	                    this->default_category))
	{
		this->default_category = (this->category_map.begin())->first;
		UTI_ERROR("cannot find default MAC traffic category\n");
		return false;
	}

	return true;
}


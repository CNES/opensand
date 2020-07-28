/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "Ip.h"
#include "SarpTable.h"
#include "TrafficCategory.h"


#include <opensand_output/Output.h>
#include <opensand_conf/ConfigurationFile.h>

#include <vector>
#include <map>


#define SECTION_MAPPING     "ip_qos"
#define MAPPING_LIST        "categories"
#define MAPPING_IP_DSCP     "dscp"
#define MAPPING_MAC_PRIO    "mac_prio"
#define MAPPING_NAME        "name"
#define KEY_DEF_CATEGORY    "default_dscp"
#define CONF_IP_FILENAME    "ip.conf"


Ip::Ip():
	LanAdaptationPlugin(NET_PROTO_IP)
{
}

Ip::Context::Context(LanAdaptationPlugin &plugin):
	LanAdaptationContext(plugin)
{
	// register the static IP packet log
	IpPacket::ip_log = Output::Get()->registerLog(LEVEL_WARNING, "LanAdaptation.Net.IP");
}

bool Ip::Context::init()
{
	if(!LanAdaptationPlugin::LanAdaptationContext::init())
	{
		return false;
	}
	ConfigurationFile config;
	vector<string> conf_files;
	string conf_ip_path;
	conf_ip_path = this->getConfPath() + string(CONF_IP_FILENAME);
	conf_files.push_back(conf_ip_path.c_str());

	if(config.loadConfig(conf_files) < 0)
	{
		LOG(this->log, LEVEL_ERROR,
		    "failed to load config file '%s'", conf_ip_path.c_str());
		return false;
	}

	this->handle_net_packet = true;
	if(!this->initTrafficCategories(config))
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot Initialize traffic categories\n");
		return false;
	}
	return true;
}

Ip::Context::~Context()
{
	map<qos_t, TrafficCategory *>::const_iterator cat_it;
	for(cat_it = this->category_map.begin();
	    cat_it != this->category_map.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
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
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of IP packets\n");
		delete burst;
		return NULL;
	}

	// TODO for here and deencap functions try to dynamic cast packets
	//      instead of allocating (do not forget to erase from source burst
	//      because when releasing burst content is also released
	for(packet = burst->begin(); packet != burst->end(); ++packet)
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
				LOG(this->log, LEVEL_ERROR,
				    "encap:unknown IP packet version");
				continue;
		}
		LOG(this->log, LEVEL_INFO,
		    "encap:got an IPv%u packet\n",
		    IpPacket::version((*packet)->getData()));
		// check IP packet validity
		if(!ip_packet->isValid())
		{
			LOG(this->log, LEVEL_ERROR,
			    "IP packet is not valid\n");
			delete ip_packet;
			continue;
		}

		ip_packet->setSrcTalId(this->tal_id);
		if(!this->onMsgIp(ip_packet))
		{
			LOG(this->log, LEVEL_ERROR,
			    "IP handling failed, drop packet\n");
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
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of network "
		    "packets\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); ++packet)
	{
		IpPacket *ip_packet;
		IpAddress *ip_addr;
		tal_id_t dst_tal_id;
		tal_id_t src_tal_id = (*packet)->getSrcTalId();

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
				LOG(this->log, LEVEL_ERROR,
				    "deencap:unknown IP packet version");
				continue;
		}
		LOG(this->log, LEVEL_INFO,
		    "deencap:got an IPv%u packet\n",
		    IpPacket::version((*packet)->getData()));

		// check IP packet validity
		if(!ip_packet->isValid())
		{
			LOG(this->log, LEVEL_ERROR,
			    "IP packet is not valid\n");
			delete ip_packet;
			continue;
		}

		// get destination Tal ID from IP information as on GW
		// in transparent mode, the destination is always the GW
		// itself
		ip_addr = ip_packet->dstAddr();
		if(!this->sarp_table->getTalByIp(ip_addr, dst_tal_id))
		{
			// check default tal_id
			if(dst_tal_id > BROADCAST_TAL_ID)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot get destination tal ID in SARP table\n");
				delete ip_packet;
				continue;
			}
			else
			{
				LOG(this->log, LEVEL_INFO,
				    "cannot find destination tal ID, use default "
				    "(%u)\n", dst_tal_id);
			}
		}
		ip_packet->setDstTalId(dst_tal_id);
		ip_packet->setSrcTalId(src_tal_id);

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
		LOG(this->log, LEVEL_INFO,
		    "DSCP %d unknown		    ; IP packet goes to default "
		    "MAC category %d\n", traffic_category, 
		    this->default_category);

		found_category = this->category_map.find(this->default_category);
		if(found_category == this->category_map.end())
		{
			LOG(this->log, LEVEL_ERROR,
			    "default MAC category not defined\n");
			return false;
		}
	}
	else
	{
		LOG(this->log, LEVEL_INFO,
		    "IP packet with DSCP %d goes to MAC category %s with "
		    "id %u\n", traffic_category, 
		    found_category->second->getName().c_str(),
		    found_category->second->getId());
	}
	ip_packet->setQos(found_category->second->getId());

	if(this->tal_id != this->gw_id)
	{
		ip_packet->setDstTalId(this->gw_id);
	}
	else
	{
		// Other modes
		// DST Tal Id = Tal Id(ip_dst)
		// SRC Tal Id = Host Tal Id
		ip_addr = ip_packet->dstAddr();
		if(!ip_addr)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot get IP packet address\n");
			return false;
		}
		LOG(this->log, LEVEL_DEBUG,
		    "IPv%d destination address = %s\n",
		    ip_packet->version(), ip_addr->str().c_str());

		if(!this->sarp_table->getTalByIp(ip_addr, pkt_tal_id))
		{
			// check default tal_id
			if(pkt_tal_id > BROADCAST_TAL_ID)
			{
				// tal id not found
				LOG(this->log, LEVEL_ERROR,
				    "IP dest addr not found in SARP table\n");
				return false;

			}
			else
			{
				LOG(this->log, LEVEL_INFO,
				    "cannot find sValiddestination tal ID, use "
				    "default (%u)\n", pkt_tal_id);
			}
		}

		LOG(this->log, LEVEL_DEBUG,
		    "talID in SARP Table: %d \n", pkt_tal_id);
		                ip_packet->setDstTalId(pkt_tal_id);
	}

	LOG(this->log, LEVEL_DEBUG,
	    "Src TAL ID: %u \n", ip_packet->getSrcTalId());
	LOG(this->log, LEVEL_DEBUG,
	    "Dst TAL ID: %u \n", ip_packet->getDstTalId());

	return true;
}

char Ip::Context::getLanHeader(unsigned int pos, NetPacket *packet)
{
	unsigned char ether_type[4] = {0, 0, 0, 0};
	// create IP packet from data
	switch(IpPacket::version(packet->getData()))
	{
		case 4:
			LOG(this->log, LEVEL_INFO,
			    "add IPv4 flags for TUN interface");
			ether_type[2] = (NET_PROTO_IPV4 >> 8) & 0xFF;
			ether_type[3] = (NET_PROTO_IPV4) & 0xFF;
			break;
		case 6:
			LOG(this->log, LEVEL_INFO,
			    "add IPv6 flags for TUN interface");
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


NetPacket *Ip::PacketHandler::build(const Data &data,
                                    size_t data_length,
                                    uint8_t qos,
                                    uint8_t src_tal_id,
                                    uint8_t dst_tal_id) const
{
	if(IpPacket::version(data) == 4)
	{
		Ipv4Packet *packet;
		packet = new Ipv4Packet(data, data_length);
		packet->setQos(qos);
		packet->setSrcTalId(src_tal_id);
		packet->setDstTalId(dst_tal_id);
		return packet;
	}
	else if(IpPacket::version(data) == 6)
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
		LOG(this->log, LEVEL_ERROR,
		    "cannot get IP version\n");
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

	map<string, ConfigurationList> config_section_map;
	config.loadSectionMap(config_section_map);

	// Traffic flow categories
	if(!config.getListItems(config_section_map[SECTION_MAPPING], 
		                    MAPPING_LIST,
	                        category_list))
	{
		LOG(this->log, LEVEL_ERROR,
		    "missing or empty section [%s, %s]\n",
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
			LOG(this->log, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SECTION_MAPPING, MAPPING_LIST,
			    MAPPING_IP_DSCP, i);
			return false;
		}
		// get category name
		if(!config.getAttributeValue(iter, MAPPING_NAME, mac_queue_name))
		{
			LOG(this->log, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SECTION_MAPPING, MAPPING_LIST,
			    MAPPING_NAME, i);
			return false;
		}
		// get service class
		if(!config.getAttributeValue(iter, MAPPING_MAC_PRIO,
		                             mac_queue_prio))
		{
			LOG(this->log, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SECTION_MAPPING, MAPPING_LIST,
			    MAPPING_MAC_PRIO, i);
			return false;
		}

		if(this->category_map.count(dscp_value))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Traffic category %ld - [%s] rejected: identifier "
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
	if(!config.getValue(config_section_map[SECTION_MAPPING], 
		                KEY_DEF_CATEGORY,
	                    this->default_category))
	{
		this->default_category = (this->category_map.begin())->first;
		LOG(this->log, LEVEL_ERROR,
		    "cannot find default MAC traffic category\n");
		return false;
	}

	return true;
}


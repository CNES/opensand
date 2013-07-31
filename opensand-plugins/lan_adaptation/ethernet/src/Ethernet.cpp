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
 * @file Ethernet.cpp
 * @brief Ethernet LAN adaptation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Remy PIENNE <rpienne@toulouse.viveris.com>
 */

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_QOS_DATA
#include <opensand_conf/uti_debug.h>

#include "Ethernet.h"
#include "TrafficCategory.h"


#include <vector>
#include <map>
#include <arpa/inet.h>

#define CONF_ETH_FILE "/etc/opensand/plugins/ethernet.conf"

#define CONF_ETH_SECTION "ethernet"
#define CONF_SAT_FRAME_TYPE "sat_frame_type"
#define CONF_LAN_FRAME_TYPE "lan_frame_type"
#define CONNECTION_LIST  "virtual_connections"
#define EVC_ID           "id"
#define MAC_SRC          "mac_src"
#define MAC_DST          "mac_dst"
#define IP_SRC           "ip_src"
#define IP_DST           "ip_dst"
#define Q_TAG            "tag_802_1q"
#define AD_TAG           "tag_802_1ad"
#define PROTOCOL_TYPE    "protocol"

// TODO remove following elements
#define SECTION_MAPPING		"ip_qos"
#define MAPPING_LIST		"categories"
#define MAPPING_IP_DSCP		"dscp"
#define MAPPING_MAC_PRIO	"mac_prio"
#define MAPPING_MAC_NAME	"mac_name"
#define KEY_DEF_CATEGORY	"default_dscp"
#define CONF_IP_FILE "/etc/opensand/plugins/ip.conf"

Ethernet::Ethernet():
	LanAdaptationPlugin(NET_PROTO_ETH)
{
	ConfigurationFile config;
	string sat_eth;

	this->upper[TRANSPARENT].push_back("IP");
	this->upper[TRANSPARENT].push_back("ROHC");

	this->upper[REGENERATIVE].push_back("IP");
	this->upper[REGENERATIVE].push_back("ROHC");

	// here we need frame type on satellite for lower layers
	if(config.loadConfig(CONF_ETH_FILE) < 0)
	{
		UTI_ERROR("failed to load config file '%s'", CONF_ETH_FILE);
		return;
	}
	if(!config.getValue(CONF_ETH_SECTION, CONF_SAT_FRAME_TYPE, sat_eth))
	{
		UTI_ERROR("missing %s parameter\n", CONF_SAT_FRAME_TYPE);
	}

	config.unloadConfig();

	if(sat_eth == "Ethernet")
	{
		this->ether_type = NET_PROTO_ETH;
	}
	else if(sat_eth == "802.1Q")
	{
		this->ether_type = NET_PROTO_802_1Q;
	}
	else if(sat_eth == "802.1ad")
	{
		this->ether_type = NET_PROTO_802_1AD;
	}
	else
	{
		UTI_ERROR("unknown type of Ethernet frames\n");
		this->ether_type = NET_PROTO_ERROR;
	}
}

Ethernet::Context::Context(LanAdaptationPlugin &plugin):
	LanAdaptationContext(plugin)
{
	ConfigurationFile config;
	string lan_eth;
	string sat_eth;

	this->handle_net_packet = true;

	if(config.loadConfig(CONF_ETH_FILE) < 0)
	{
		UTI_ERROR("failed to load config file '%s'", CONF_ETH_FILE);
		return;
	}

	if(!config.getValue(CONF_ETH_SECTION, CONF_LAN_FRAME_TYPE, lan_eth))
	{
		UTI_ERROR("missing %s parameter\n", CONF_LAN_FRAME_TYPE);
	}
	if(!config.getValue(CONF_ETH_SECTION, CONF_SAT_FRAME_TYPE, sat_eth))
	{
		UTI_ERROR("missing %s parameter\n", CONF_SAT_FRAME_TYPE);
	}

	if(!this->initEvc(config))
	{
		UTI_ERROR("failed to Initialize EVC\n");
	}
	config.unloadConfig();

	if(lan_eth == "Ethernet")
	{
		UTI_DEBUG("Ethernet layer without extension on network\n");
		this->lan_frame_type = NET_PROTO_ETH;
	}
	else if(lan_eth == "802.1Q")
	{
		UTI_DEBUG("Ethernet layer support 802.1Q extension on network\n");
		this->lan_frame_type = NET_PROTO_802_1Q;
	}
	else if(lan_eth == "802.1ad")
	{
		UTI_DEBUG("Ethernet layer support 802.1ad extension on network\n");
		this->lan_frame_type = NET_PROTO_802_1AD;
	}
	else
	{
		UTI_ERROR("unknown type of Ethernet layer on network\n");
		this->lan_frame_type = NET_PROTO_ERROR;
	}

	if(sat_eth == "Ethernet")
	{
		UTI_DEBUG("Ethernet layer without extension on satellite\n");
		this->sat_frame_type = NET_PROTO_ETH;
	}
	else if(sat_eth == "802.1Q")
	{
		UTI_DEBUG("Ethernet layer support 802.1Q extension on satellite\n");
		this->sat_frame_type = NET_PROTO_802_1Q;
	}
	else if(sat_eth == "802.1ad")
	{
		UTI_DEBUG("Ethernet layer support 802.1ad extension on satellite\n");
		this->sat_frame_type = NET_PROTO_802_1AD;
	}
	else
	{
		UTI_ERROR("unknown type of Ethernet layer on satellite\n");
		this->sat_frame_type = NET_PROTO_ERROR;
	}

	// TODO remove, use something else
	if(config.loadConfig(CONF_IP_FILE) < 0)
	{
		UTI_ERROR("failed to load config file '%s'", CONF_IP_FILE);
		return;
	}

	if(!this->initTrafficCategories(config))
	{
		UTI_ERROR("cannot Initialize traffic categories\n");
	}

	config.unloadConfig();
}

Ethernet::Context::~Context()
{
	map<uint8_t, Evc *>::iterator it;
	for(it = this->evc_map.begin(); it != this->evc_map.end(); ++it)
	{
		delete (*it).second;
	}
	this->evc_map.clear();

	this->probe_evc_throughput.clear();
	this->probe_evc_size.clear();
}

bool Ethernet::Context::initEvc(ConfigurationFile &config)
{
	int i = 0;
	ConfigurationList evc_list;
	ConfigurationList::iterator iter;

	if(!config.getListItems(CONF_ETH_SECTION, CONNECTION_LIST, evc_list))
	{
		UTI_ERROR("missing or empty section [%s, %s]\n",
		          CONF_ETH_SECTION, CONNECTION_LIST);
		return false;
	}

	for(iter = evc_list.begin(); iter != evc_list.end(); iter++)
	{
		Evc *evc;
		string src;
		string dst;
		uint8_t id = 0;
		MacAddress *mac_src;
		MacAddress *mac_dst;
		uint16_t q_tag = 0;
		uint16_t ad_tag = 0;
		stringstream proto;
		uint16_t pt;

		i++;

		// get ID
		if(!config.getAttributeValue(iter, EVC_ID, id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", CONF_ETH_SECTION, CONNECTION_LIST,
			          EVC_ID,  i);
			return false;
		}

		// get source MAC address
		if(!config.getAttributeValue(iter, MAC_SRC, src))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", CONF_ETH_SECTION, CONNECTION_LIST,
			          MAC_SRC, i);
			return false;
		}
		mac_src = new MacAddress(src);
		// get destination MAC address
		if(!config.getAttributeValue(iter, MAC_DST, dst))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", CONF_ETH_SECTION, CONNECTION_LIST,
			          MAC_DST, i);
			return false;
		}
		mac_dst = new MacAddress(dst);

		// get 802.1Q tag
		if(!config.getAttributeValue(iter, Q_TAG, q_tag))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", CONF_ETH_SECTION, CONNECTION_LIST,
			          Q_TAG,  i);
			return false;
		}
		// get 802.1ad tag
		if(!config.getAttributeValue(iter, AD_TAG, ad_tag))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", CONF_ETH_SECTION, CONNECTION_LIST,
			          AD_TAG, i);
			return false;
		}
		// get source protocol type
		if(!config.getAttributeValue(iter, PROTOCOL_TYPE, proto))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", CONF_ETH_SECTION, CONNECTION_LIST,
			          PROTOCOL_TYPE, i);
			return false;
		}
		proto >> std::hex >> pt;

		UTI_DEBUG("New EVC: MAC source = %s, MAC destination = %s, "
		          "tag Q = %u, tag AD = %u, payload_type = %#2X\n",
		          mac_src->str().c_str(), mac_dst->str().c_str(),
		          q_tag, ad_tag, pt);

		evc = new Evc(mac_src, mac_dst, q_tag, ad_tag, pt);

		if(this->evc_map.find(id) != this->evc_map.end())
		{
			UTI_ERROR("Duplicated ID %u in Ethernet Virtual Connections\n", id);
			return false;
		}
		this->evc_map[id] = evc;
	}

	return true;
}

bool Ethernet::Context::initTrafficCategories(ConfigurationFile &config)
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


bool Ethernet::Context::initLanAdaptationContext(
	tal_id_t tal_id,
	sat_type_t satellite_type,
	const SarpTable *sarp_table)
{
	if(!LanAdaptationPlugin::LanAdaptationContext::initLanAdaptationContext(
										tal_id, satellite_type, sarp_table))
	{
		return false;
	}

	this->initStats();
	return true;
}


NetBurst *Ethernet::Context::encapsulate(NetBurst *burst,
                                         map<long, int> &UNUSED(time_contexts))
{
	NetBurst *eth_frames = NULL;
	NetBurst::iterator packet;

	if(this->current_upper)
	{
		UTI_DEBUG("got a burst of %s packets to encapsulate\n",
		          this->current_upper->getName().c_str());
	}
	else
	{
		UTI_DEBUG("got a network packet to encapsulate\n");
	}

	// create an empty burst of ETH frames
	eth_frames = new NetBurst();
	if(eth_frames == NULL)
	{
		UTI_ERROR("cannot allocate memory for burst of ETH frames\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		NetPacket *eth_frame;
		uint8_t evc_id = 0;

		if(this->current_upper)
		{
			eth_frame = this->createEthFrameData(*packet, evc_id);
			if(!eth_frame)
			{
				continue;
			}
		}
		else
		{
			Data payload = (*packet)->getData();
			unsigned char *data =(unsigned char *)payload.data();
			size_t length = (*packet)->getTotalLength();
			size_t header_length;
			uint16_t ether_type = Ethernet::getPayloadEtherType(data, length);
			uint16_t frame_type = Ethernet::getFrameType(data, length);
			MacAddress src_mac = Ethernet::getSrcMac(data, length);
			MacAddress dst_mac = Ethernet::getDstMac(data, length);
			tal_id_t src;
			tal_id_t dst = 0;
			uint16_t q_tag = Ethernet::getQTag(data, length);
			uint16_t ad_tag = Ethernet::getAdTag(data, length);
			qos_t qos = 0;
			Evc *evc;
			map<qos_t, TrafficCategory *>::const_iterator default_category;

			// Do not print errors here because we may want to reject trafic as spanning
			// tree coming from miscellaneous host
			if(!this->sarp_table->getTalByMac(src_mac, src))
			{
				// do not use default here, default is for destination !
				UTI_INFO("cannot find source MAC address %s in sarp table\n",
				         src_mac.str().c_str());
				continue;
			}
			if(this->tal_id != GW_TAL_ID && this->satellite_type == TRANSPARENT)
			{
				// ST in transparent mode:
				// DST Tal Id = GW
				// SRC Tal Id = ST Tal Id
				dst = GW_TAL_ID;
			}
			else if(!this->sarp_table->getTalByMac(dst_mac, dst))
			{
				// check default tal_id
				if(dst > BROADCAST_TAL_ID)
				{
					UTI_INFO("cannot find destination MAC address %s in sarp table\n",
					          dst_mac.str().c_str());
					continue;
				}
				else
				{
					// TODO use info or notice once it will not be printed by default
					UTI_DEBUG("cannot find destination tal ID, use default (%u)\n",
					          dst);
				}
			}
			UTI_DEBUG("build Ethernet frame with source MAC %s corresponding to terminal ID %d "
			          "and destination MAC %s corresponding to terminal ID %d\n",
			          src_mac.str().c_str(), src, dst_mac.str().c_str(), dst);

			switch(frame_type)
			{
				case NET_PROTO_ETH:
					header_length = ETHERNET_2_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, ether_type, evc_id);
					break;
				case NET_PROTO_802_1Q:
					header_length = ETHERNET_802_1Q_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, q_tag, ether_type, evc_id);
					break;
				case NET_PROTO_802_1AD:
					header_length = ETHERNET_802_1AD_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, q_tag, ad_tag, ether_type, evc_id);
					break;
				default:
					UTI_ERROR("wrong Ethernet frame type 0x%.4x\n", frame_type);
					continue;
			}
			if(!evc)
			{
				UTI_INFO("cannot find EVC for this flow, use the default values\n");
			}

			// get default QoS value
			default_category = this->category_map.find(this->default_category);
			if(default_category == this->category_map.end())
			{
				UTI_ERROR("Unable to find default category for QoS");
				continue;
			}
			qos = default_category->second->getId();

			if(frame_type != this->sat_frame_type)
			{
				if(evc)
				{
					map<qos_t, TrafficCategory *>::const_iterator found_category;
					q_tag = (evc->getQTag() & 0xff);
					ad_tag = (evc->getAdTag() & 0xff);
					// here we use the ad_tag to set QoS at DVB layer
					// TODO add other parameters
					found_category = this->category_map.find(ad_tag);
					if(found_category == this->category_map.end())
					{
						found_category = this->category_map.find(this->default_category);
						if(found_category == this->category_map.end())
							continue;
					}
					qos = found_category->second->getId();
					UTI_DEBUG("Use the ad-tag to get the QoS value (%u) for DVB layer\n",
					          qos);
				}

				eth_frame = this->createEthFrameData(data + header_length,
				                                     length - header_length,
				                                     src_mac, dst_mac,
				                                     ether_type,
				                                     q_tag, ad_tag,
				                                     qos, src, dst,
				                                     this->sat_frame_type);
			}
			else
			{
				eth_frame = this->createPacket(data, length, qos, src, dst);
			}
			if(eth_frame == NULL)
			{
				UTI_ERROR("cannot create the Ethernet frame\n");
				continue;
			}
		}

		if(this->evc_data_size.find(evc_id) == this->evc_data_size.end())
		{
			// create the element
			this->evc_data_size[evc_id] = 0;
		}
		this->evc_data_size[evc_id] += eth_frame->getTotalLength();
		eth_frames->add(eth_frame);
	}
	UTI_DEBUG("encapsulate %zu Ethernet frames\n", eth_frames->size());

	// delete the burst and all frames in it
	delete burst;
	// avoid returning empty bursts
	if(eth_frames->size() > 0)
	{
		return eth_frames;
	}
	return NULL;
}


NetBurst *Ethernet::Context::deencapsulate(NetBurst *burst)
{
	NetBurst *net_packets;
	NetBurst::iterator packet;

	if(burst == NULL || burst->front() == NULL)
	{
		UTI_ERROR("empty burst received\n");
		if(burst)
		{
			delete burst;
		}
		return NULL;
	}
	UTI_DEBUG("got a burst of %s packets to deencapsulate\n",
	          (burst->front())->getName().c_str());

	// create an empty burst of network frames
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("cannot allocate memory for burst of network frames\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		NetPacket *deenc_packet = NULL;
		size_t data_length = (*packet)->getTotalLength();
		Data payload = (*packet)->getData();
		unsigned char *data =(unsigned char *)payload.data();
		MacAddress dst_mac = Ethernet::getDstMac(data, data_length);
		MacAddress src_mac = Ethernet::getSrcMac(data, data_length);
		uint16_t q_tag = Ethernet::getQTag(data, data_length);
		uint16_t ad_tag = Ethernet::getAdTag(data, data_length);
		uint16_t ether_type = Ethernet::getPayloadEtherType(data, data_length);
		uint16_t frame_type = Ethernet::getFrameType(data, data_length);
		Evc *evc;
		size_t header_length;
		uint8_t evc_id = 0;

		switch(frame_type)
		{
			case NET_PROTO_ETH:
				header_length = ETHERNET_2_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, ether_type, evc_id);
				break;
			case NET_PROTO_802_1Q:
				header_length = ETHERNET_802_1Q_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, q_tag, ether_type, evc_id);
				break;
			case NET_PROTO_802_1AD:
				header_length = ETHERNET_802_1AD_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, q_tag, ad_tag, ether_type, evc_id);
				break;
			default:
				UTI_ERROR("wrong Ethernet frame type 0x%.4x\n", frame_type);
				evc_id = 0;
				continue;
		}

		if(this->evc_data_size.find(evc_id) == this->evc_data_size.end())
		{
			this->evc_data_size[evc_id] = 0;
		}
		this->evc_data_size[evc_id] += data_length;

		UTI_DEBUG("Ethernet frame received: src: %s, dst %s, Q-tag: %u, "
		          "ad-tag: %u, EtherType: 0x%.4x\n",
		          src_mac.str().c_str(), dst_mac.str().c_str(),
		          q_tag, ad_tag, ether_type);

		if(this->current_upper)
		{
			if(ether_type == NET_PROTO_ARP && this->current_upper->getName() == "IP")
			{
				UTI_INFO("ARP is not supported on IP layer at the moment, drop it\n");
				continue;
			}

			// we need to instantiate payload, else if we directly do the cast
			// on getPayload the data can be lost
			Data payload = (*packet)->getPayload();
			// strip eth header to get to IP
			data_length = (*packet)->getPayloadLength();
			data = (unsigned char *)payload.data();
			deenc_packet = this->current_upper->build(data, data_length,
			                                          (*packet)->getQos(),
			                                          (*packet)->getSrcTalId(),
			                                          (*packet)->getDstTalId());
		}
		else
		{
			// TODO factorize
			tal_id_t dst;

			// Here we have errors because if we received this packets
			// the information should be in sarp table
			if(!this->sarp_table->getTalByMac(dst_mac, dst))
			{
				// check default tal_id
				if(dst > BROADCAST_TAL_ID)
				{
					UTI_ERROR("cannot find destination MAC address %s in sarp table\n",
					          dst_mac.str().c_str());
					delete deenc_packet;
					continue;
				}
				else
				{
					// TODO use info or notice once it will not be printed by default
					UTI_DEBUG("cannot find destination tal ID, use default (%u)\n",
					          dst);
				}
			}

			if(frame_type != this->lan_frame_type)
			{
				if(evc)
				{
					q_tag = (evc->getQTag() & 0xff);
					ad_tag = (evc->getAdTag() & 0xff);
				}
				deenc_packet = this->createEthFrameData(data + header_length,
				                                        data_length - header_length,
				                                        src_mac, dst_mac,
				                                        ether_type,
				                                        q_tag, ad_tag,
				                                        (*packet)->getQos(),
				                                        (*packet)->getSrcTalId(),
				                                        dst,
				                                        this->lan_frame_type);
			}
			else
			{
				// create ETH packet
				deenc_packet = this->createPacket(data, data_length,
				                                  (*packet)->getQos(),
				                                  (*packet)->getSrcTalId(),
				                                  dst);
			}
		}
		if(!deenc_packet)
		{
			UTI_ERROR("failed to deencapsulated Ethernet frame\n");
			continue;
		}

		net_packets->add(deenc_packet);
	}
	UTI_DEBUG("deencapsulate %zu ethernet frames\n", net_packets->size());

	// delete the burst and all frames in it
	delete burst;
	return net_packets;
}


NetPacket *Ethernet::Context::createEthFrameData(NetPacket *packet, uint8_t &evc_id)
{
	Data payload = packet->getData();
	unsigned char *data =(unsigned char *)payload.data();
	size_t data_length = packet->getTotalLength();
	vector<MacAddress> src_macs;
	vector<MacAddress> dst_macs;
	MacAddress src_mac;
	MacAddress dst_mac;
	tal_id_t src_tal = packet->getSrcTalId();
	tal_id_t dst_tal = packet->getDstTalId();
	qos_t qos = packet->getQos();
	uint16_t q_tag = 0;
	uint16_t ad_tag = 0;
	uint16_t ether_type = packet->getType();
	Evc *evc = NULL;

	// search traffic category associated with QoS value
	// TODO we should filter on IP addresses instead of QoS
	for(map<qos_t, TrafficCategory *>::const_iterator it = this->category_map.begin();
	    it != this->category_map.end(); ++it)
	{
		if((*it).second->getId() == qos)
		{
			ad_tag = (*it).first;
		}
	}

	// TODO get ad_tag with IP instead of taking the qos value, in order that
	//      the qos at layer 3 is independant
	if(!this->sarp_table->getMacByTal(src_tal, src_macs))
	{
		UTI_ERROR("unable to find MAC address associated with terminal with ID %u\n",
		          src_tal);
		return NULL;
	}
	if(!this->sarp_table->getMacByTal(dst_tal, dst_macs))
	{
		UTI_ERROR("unable to find MAC address associated with terminal with ID %u\n",
		          dst_tal);
		return NULL;
	}
	for(vector<MacAddress>::iterator it1 = src_macs.begin();
	    it1 != src_macs.end(); ++it1)
	{
		for(vector<MacAddress>::iterator it2 = dst_macs.begin();
		    it2 != dst_macs.end(); ++it2)
		{
			// TODO remove tags from here and search with IP addresses
			evc = this->getEvc(*it1, *it2, q_tag, ad_tag, ether_type, evc_id);
			if(evc)
			{
				break;
			}
		}
		if(evc)
		{
			break;
		}
	}
	if(!evc)
	{
		map<qos_t, TrafficCategory *>::const_iterator default_category;
		UTI_INFO("no EVC for this flow, use default values");
		src_mac = src_macs.front();
		dst_mac = dst_macs.front();
		// get default QoS value
		default_category = this->category_map.find(this->default_category);
		if(default_category != this->category_map.end())
		{
			ad_tag = default_category->first;
		}
		evc_id = 0;
	}
	else
	{
		q_tag = (evc->getQTag() & 0xff);
		ad_tag = (evc->getAdTag() & 0xff);
		src_mac = *(evc->getMacSrc());
		dst_mac = *(evc->getMacDst());
	}
	return this->createEthFrameData(data, data_length, src_mac, dst_mac, ether_type,
	                                q_tag, ad_tag, qos, src_tal, dst_tal, this->sat_frame_type);
}

NetPacket *Ethernet::Context::createEthFrameData(unsigned char *data, size_t length,
                                                 MacAddress src_mac, MacAddress dst_mac,
                                                 uint16_t ether_type,
                                                 uint16_t q_tag, uint16_t ad_tag,
                                                 qos_t qos,
                                                 tal_id_t src_tal_id, tal_id_t dst_tal_id,
                                                 uint16_t desired_frame_type)
{
	eth_2_header_t *eth_2_hdr;
	eth_1q_header_t *eth_1q_hdr;
	eth_1ad_header_t *eth_1ad_hdr;

	unsigned char frame_data[MAX_ETHERNET_SIZE];
	size_t frame_data_length = 0;

	memset(frame_data, '\0', MAX_ETHERNET_SIZE);
	// common part for all header
	eth_2_hdr = (eth_2_header_t *) frame_data;
	for(unsigned int i = 0; i < 6; i++)
	{
		eth_2_hdr->ether_dhost[i] = dst_mac.at(i);
		eth_2_hdr->ether_shost[i] = src_mac.at(i);
	}
	// build eth frame : header + whole IP packet
	switch(desired_frame_type)
	{
		case NET_PROTO_ETH:
			eth_2_hdr->ether_type = htons(ether_type);
			memcpy(frame_data + ETHERNET_2_HEADSIZE, data, length);
			frame_data_length = length + ETHERNET_2_HEADSIZE;
			UTI_DEBUG("create an Ethernet frame with src = %s, "
			          "dst = %s\n", src_mac.str().c_str(), dst_mac.str().c_str());
			break;
		case NET_PROTO_802_1Q:
			eth_1q_hdr = (eth_1q_header_t *) frame_data;
			eth_1q_hdr->TPID = htons(NET_PROTO_802_1Q);
			eth_1q_hdr->TCI = htons(q_tag);
			eth_1q_hdr->ether_type = htons(ether_type);
			memcpy(frame_data + ETHERNET_802_1Q_HEADSIZE, data, length);
			frame_data_length = length + ETHERNET_802_1Q_HEADSIZE;
			UTI_DEBUG("create a 802.1Q frame with src = %s, "
			          "dst = %s, VLAN ID = %d\n", src_mac.str().c_str(),
			          dst_mac.str().c_str(), q_tag);
			break;
		case NET_PROTO_802_1AD:
			eth_1ad_hdr = (eth_1ad_header_t *) frame_data;
			// TODO use NET_PROTO_802_1AD once kernel will support it
			eth_1ad_hdr->outer_TPID = htons(NET_PROTO_802_1Q);
			//eth_1ad_hdr->outer_TPID = htons(NET_PROTO_802_1AD);
			eth_1ad_hdr->outer_TCI = htons(ad_tag);
			eth_1ad_hdr->inner_TPID = htons(NET_PROTO_802_1Q);
			eth_1ad_hdr->inner_TCI = htons(q_tag);
			eth_1ad_hdr->ether_type = htons(ether_type);
			memcpy(frame_data + ETHERNET_802_1AD_HEADSIZE, data, length);
			frame_data_length = length + ETHERNET_802_1AD_HEADSIZE;
			UTI_DEBUG("create a 802.1AD frame with src = %s, "
			          "dst = %s, q-tag = %u, ad-tag = %u\n",
			          src_mac.str().c_str(), dst_mac.str().c_str(), q_tag, ad_tag);
			break;
		default:
			UTI_ERROR("Bad protocol value (0x%.4x) for Ethernet plugin\n", desired_frame_type);
			return NULL;
	}
	return this->createPacket(frame_data, frame_data_length,
	                          qos,
	                          src_tal_id, dst_tal_id);

}

char Ethernet::Context::getLanHeader(unsigned int UNUSED(pos),
                                     NetPacket *UNUSED(frame))
{
	return 0;
}

bool Ethernet::Context::handleTap()
{
	// if no upper protocol, then we are using a TAP device
	return !(this->current_upper);
}


void Ethernet::Context::initStats()
{
	stringstream name("");
	uint8_t id;

	// create default probe with EVC=0 if it does no exist
	id = 0;
	// TODO try to do default in and default out
	name << "EVC throughput.default";
	// we can receive any type of frames
	this->probe_evc_throughput[id] =
		Output::registerProbe<float>(name.str().c_str(), "kbits/s", true, SAMPLE_AVG);
	name.str("");
	name << "EVC frame size.default";
	this->probe_evc_size[id] =
		Output::registerProbe<float>(name.str().c_str(), "Bytes", true, SAMPLE_SUM);

	for(map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
	    it != this->evc_map.end(); ++it)
	{
		id = (*it).first;
		if(this->probe_evc_throughput.find(id) != this->probe_evc_throughput.end())
		{
			continue;
		}

		name.str("");
		name << "EVC throughput." << (unsigned int) id;
		this->probe_evc_throughput[id] =
			Output::registerProbe<float>(name.str().c_str(), "kbits/s", true, SAMPLE_AVG);
		name.str("");
		name << "EVC frame size." << (unsigned int) id;
		this->probe_evc_size[id] =
			Output::registerProbe<float>(name.str().c_str(), "Bytes", true, SAMPLE_SUM);
	}
}

void Ethernet::Context::updateStats(unsigned int period)
{
	map<uint8_t, size_t>::iterator it;
	map<uint8_t, Probe<float> *>::iterator found;
	uint8_t id;
	stringstream name;

	for(it = this->evc_data_size.begin(); it != this->evc_data_size.end(); ++it)
	{
		id = (*it).first;
		if(this->probe_evc_throughput.find(id) == this->probe_evc_throughput.end())
		{
			// use the default id
			id = 0;
		}
		this->probe_evc_throughput[id]->put((*it).second * 8 / period);
		this->probe_evc_size[id]->put((*it).second);
		(*it).second = 0;
	}
}

NetPacket *Ethernet::PacketHandler::build(unsigned char *data, size_t data_length,
                                          uint8_t qos,
                                          uint8_t src_tal_id,
                                          uint8_t dst_tal_id)
{
	NetPacket *frame = NULL;
	size_t head_length = 0;
	uint16_t frame_type = Ethernet::getFrameType(data, data_length);
	switch(frame_type)
	{
		case NET_PROTO_802_1Q:
			head_length = ETHERNET_802_1Q_HEADSIZE;
			break;
		case NET_PROTO_802_1AD:
			head_length = ETHERNET_802_1AD_HEADSIZE;
			break;
		// Ethernet packet, this is the ethertype of the payload
		default:
			head_length = ETHERNET_2_HEADSIZE;
			break;
	}

	frame = new NetPacket(data, data_length,
	                      this->getName(),
	                      frame_type,
	                      qos,
	                      src_tal_id,
	                      dst_tal_id,
	                      head_length);
	return frame;
}

Evc *Ethernet::Context::getEvc(const MacAddress src_mac,
                               const MacAddress dst_mac,
                               uint16_t ether_type,
                               uint8_t &evc_id) const
{
	for(map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
	    it != this->evc_map.end(); ++it)
	{
		if((*it).second->matches(&src_mac, &dst_mac, ether_type))
		{
			evc_id = (*it).first;
			return (*it).second;
		}
	}
	return NULL;
}


Evc *Ethernet::Context::getEvc(const MacAddress src_mac,
                               const MacAddress dst_mac,
                               uint16_t q_tag,
                               uint16_t ether_type,
                               uint8_t &evc_id) const
{
	for(map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
	    it != this->evc_map.end(); ++it)
	{
		if((*it).second->matches(&src_mac, &dst_mac, q_tag, ether_type))
		{
			evc_id = (*it).first;
			return (*it).second;
		}
	}
	return NULL;
}


Evc *Ethernet::Context::getEvc(const MacAddress src_mac,
                               const MacAddress dst_mac,
                               uint16_t q_tag,
                               uint16_t ad_tag,
                               uint16_t ether_type,
                               uint8_t &evc_id) const
{
	for(map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
	    it != this->evc_map.end(); ++it)
	{
		if((*it).second->matches(&src_mac, &dst_mac, q_tag, ad_tag, ether_type))
		{
			evc_id = (*it).first;
			return (*it).second;
		}
	}
	return NULL;
}

uint16_t Ethernet::getFrameType(const unsigned char *data,
                                size_t length)
{
	uint16_t ether_type = NET_PROTO_ERROR;
	uint16_t ether_type2 = NET_PROTO_ERROR;
	if(data == NULL || length < 13)
	{
		UTI_ERROR("cannot retrieve EtherType in Ethernet header\n");
		return NET_PROTO_ERROR;
	}
	// read ethertype: 2 bytes at a 12 bytes offset
	ether_type = (data[12] << 8) | data[13];
	ether_type2 = (data[16] << 8) | data[17];
	if(ether_type != NET_PROTO_802_1Q && ether_type != NET_PROTO_802_1AD)
	{
		ether_type = NET_PROTO_ETH;
	}
	// TODO: we need the following part because we use two 802.1Q tags for kernel support
	else if(ether_type == NET_PROTO_802_1Q && ether_type2 == NET_PROTO_802_1Q)
	{
		ether_type = NET_PROTO_802_1AD;
	}
	return ether_type;
}

uint16_t Ethernet::getPayloadEtherType(const unsigned char *data,
                                       size_t length)
{
	uint16_t ether_type = NET_PROTO_ERROR;
	if(data == NULL || length < 13)
	{
		UTI_ERROR("cannot retrieve EtherType in Ethernet header\n");
		return NET_PROTO_ERROR;
	}
	// read ethertype: 2 bytes at a 12 bytes offset
	ether_type = (data[12] << 8) | data[13];
	switch(ether_type)
	{
		case NET_PROTO_802_1Q:
			if(length < 17)
			{
				UTI_ERROR("cannot retrieve EtherType in Ethernet header\n");
				return NET_PROTO_ERROR;
			}
			ether_type = (data[16] << 8) | data[17];

			// TODO: we need the following part because we use two 802.1Q
			//       tags for kernel support
			if(ether_type != NET_PROTO_802_1Q)
			{
				break;
			}
		case NET_PROTO_802_1AD:
			if(length < 21)
			{
				UTI_ERROR("cannot retrieve EtherType in Ethernet header\n");
				return NET_PROTO_ERROR;
			}
			ether_type = (data[20] << 8) | data[21];
			break;
	}

	return ether_type;
}

uint16_t Ethernet::getQTag(const unsigned char *data,
                           size_t length)
{
	uint16_t vlan_id = 0;
	uint16_t ether_type;
	if(data == NULL || length < 17)
	{
		UTI_ERROR("cannot retrieve vlan id in Ethernet header\n");
		return 0;
	}
	ether_type = (data[12] << 8) | data[13];
	switch(ether_type)
	{
		case NET_PROTO_802_1Q:
			vlan_id = (data[14] << 8) | data[15];
			// TODO: we need the following part because we use two 802.1Q
			//       tags for kernel support
			ether_type = (data[16] << 8) | data[17];
			if(ether_type != NET_PROTO_802_1Q)
			{
				break;
			}
		case NET_PROTO_802_1AD:
			vlan_id = (data[18] << 8) | data[19];
			break;
	}

	return vlan_id;
}

uint16_t Ethernet::getAdTag(const unsigned char *data,
                            size_t length)
{
	uint16_t vlan_id = 0;
	uint16_t ether_type;
	uint16_t ether_type2;
	if(data == NULL || length < 17)
	{
		UTI_ERROR("cannot retrieve vlan id in Ethernet header\n");
		return 0;
	}
	ether_type = (data[12] << 8) | data[13];
	ether_type2 = (data[16] << 8) | data[17];
	// TODO: we need the following part because we use two 802.1Q tags for kernel support
	if(ether_type == NET_PROTO_802_1Q && ether_type2 == NET_PROTO_802_1Q)
	{
		ether_type = NET_PROTO_802_1AD;
	}

	switch(ether_type)
	{
		case NET_PROTO_802_1AD:
			vlan_id = (data[14] << 8) | data[15];
			break;
	}

	return vlan_id;
}

MacAddress Ethernet::getDstMac(const unsigned char *data,
                               size_t length)
{
	if(data == NULL || length < 6)
	{
		UTI_ERROR("cannot retrieve destination MAC in Ethernet header\n");
		return MacAddress(0, 0, 0, 0, 0, 0);
	}

	return MacAddress(data[0], data[1], data[2],
	                  data[3], data[4], data[5]);
}

MacAddress Ethernet::getSrcMac(const unsigned char *data,
                               size_t length)
{
	if(data == NULL || length < 12)
	{
		UTI_ERROR("cannot retrieve source MAC in Ethernet header\n");
		return MacAddress(0, 0, 0, 0, 0, 0);
	}
	return MacAddress(data[6], data[7], data[8],
	                  data[9], data[10], data[11]);
}

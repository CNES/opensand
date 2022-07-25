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
 * @file Ethernet.cpp
 * @brief Ethernet LAN adaptation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Remy PIENNE <rpienne@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */


#include "Ethernet.h"
#include "BlockLanAdaptation.h"
#include "SarpTable.h"
#include "PacketSwitch.h"
#include "TrafficCategory.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>

#include <vector>
#include <map>
#include <arpa/inet.h>


Ethernet::Ethernet():
	LanAdaptationPlugin(NET_PROTO_ETH)
{
}

Ethernet::~Ethernet()
{
}

void Ethernet::generateConfiguration()
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	types->addEnumType("frame_type", "Frame Protocol Type", {"Ethernet", "802.1Q", "802.1ad"});

	auto conf = Conf->getOrCreateComponent("network", "Network", "The DVB layer configuration");

	auto categories = conf->addList("qos_classes", "QoS Classes", "qos_class")->getPattern();
	categories->addParameter("pcp", "PCP", types->getType("int"));
	categories->addParameter("name", "Class Name", types->getType("string"));
	categories->addParameter("fifo", "Fifo Name", types->getType("string"));  // <- Try using this instead of this v
	// categories->addParameter("mac_prio", "Fifo Priority", types->getType("int"));

	auto evcs = conf->addList("virtual_connections", "Virtual Connections", "virtual_connection")->getPattern();
	evcs->setAdvanced(true);
	evcs->addParameter("id", "Connection ID", types->getType("int"));
	evcs->addParameter("mac_src", "Source MAC Address", types->getType("string"));
	evcs->addParameter("mac_dst", "Destination MAC Address", types->getType("string"));
	evcs->addParameter("tci_802_1q", "TCI of the 802.1q tag", types->getType("int"));
	evcs->addParameter("tci_802_1ad", "TCI of the 802.1ad tag", types->getType("int"));
	evcs->addParameter("protocol", "Inner Payload Type", types->getType("string"), "2 Bytes Hexadecimal value");

	auto settings = conf->addComponent("qos_settings", "QoS Settings");
	settings->addParameter("lan_frame_type", "Lan Frame Type", types->getType("frame_type"),
	                       "The type of 802.1 Ethernet extension transmitted to network");
	settings->addParameter("sat_frame_type", "Satellite Frame Type", types->getType("frame_type"),
	                       "The type of 802.1 Ethernet extension carried on satellite");
	settings->addParameter("default_pcp", "Default PCP", types->getType("int"));
}


Ethernet *Ethernet::constructPlugin()
{
	static Ethernet *plugin = static_cast<Ethernet *>(Ethernet::create<Ethernet, Ethernet::Context, Ethernet::PacketHandler>("Ethernet"));
	return plugin;
}


bool Ethernet::init()
{
	if(!LanAdaptationPlugin::init())
	{
		return false;
	}

	this->upper.push_back("IP");
	this->upper.push_back("ROHC");

	auto network = OpenSandModelConf::Get()->getProfileData()->getComponent("network");
	auto qos_parameter = network->getComponent("qos_settings")->getParameter("sat_frame_type");

	// here we need frame type on satellite for lower layers
	std::string sat_eth;
	if(!OpenSandModelConf::extractParameterData(qos_parameter, sat_eth))
	{
		LOG(this->log, LEVEL_ERROR,
		    "Section QoS settings, missing parameter satellite frame type\n");
	}

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
		LOG(this->log, LEVEL_ERROR,
		    "unknown type of Ethernet frames\n");
		this->ether_type = NET_PROTO_ERROR;
	}
	return true;
}

Ethernet::Context::Context(LanAdaptationPlugin &plugin):
	LanAdaptationContext(plugin)
{
}

bool Ethernet::Context::init()
{
	if(!LanAdaptationPlugin::LanAdaptationContext::init())
	{
		return false;
	}
	
	this->handle_net_packet = true;

	auto qos = OpenSandModelConf::Get()->getProfileData()->getComponent("network")->getComponent("qos_settings");

	std::string lan_eth;
	auto lan_qos = qos->getParameter("lan_frame_type");
	if(!OpenSandModelConf::extractParameterData(lan_qos, lan_eth))
	{
		LOG(this->log, LEVEL_ERROR,
		    "Section QoS settings, missing parameter LAN frame type\n");
	}

	std::string sat_eth;
	auto sat_qos = qos->getParameter("sat_frame_type");
	if(!OpenSandModelConf::extractParameterData(sat_qos, sat_eth))
	{
		LOG(this->log, LEVEL_ERROR,
		    "Section QoS settings, missing parameter satellite frame type\n");
	}

	if(!this->initEvc())
	{
		LOG(this->log, LEVEL_ERROR,
		    "failed to Initialize EVC\n");
	}

	if(!this->initTrafficCategories())
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot Initialize traffic categories\n");
	}

	if(lan_eth == "Ethernet")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer without extension on network\n");
		this->lan_frame_type = NET_PROTO_ETH;
	}
	else if(lan_eth == "802.1Q")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1Q extension on network\n");
		this->lan_frame_type = NET_PROTO_802_1Q;
	}
	else if(lan_eth == "802.1ad")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1ad extension on network\n");
		this->lan_frame_type = NET_PROTO_802_1AD;
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "unknown type of Ethernet layer on network\n");
		this->lan_frame_type = NET_PROTO_ERROR;
	}

	if(sat_eth == "Ethernet")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer without extension on satellite\n");
		this->sat_frame_type = NET_PROTO_ETH;
	}
	else if(sat_eth == "802.1Q")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1Q extension on satellite\n");
		this->sat_frame_type = NET_PROTO_802_1Q;
	}
	else if(sat_eth == "802.1ad")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1ad extension on satellite\n");
		this->sat_frame_type = NET_PROTO_802_1AD;
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "unknown type of Ethernet layer on satellite\n");
		this->sat_frame_type = NET_PROTO_ERROR;
	}

	return true;
}

Ethernet::Context::~Context()
{
	std::map<uint8_t, Evc *>::iterator evc_it;
	std::map<qos_t, TrafficCategory *>::iterator cat_it;
	for(evc_it = this->evc_map.begin(); evc_it != this->evc_map.end(); ++evc_it)
	{
		delete (*evc_it).second;
	}
	this->evc_map.clear();
	
	for(cat_it = this->category_map.begin(); cat_it != this->category_map.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	this->category_map.clear();

	this->probe_evc_throughput.clear();
	this->probe_evc_size.clear();
}

bool Ethernet::Context::initEvc()
{
	auto network = OpenSandModelConf::Get()->getProfileData()->getComponent("network");
	
	for(auto& item : network->getList("virtual_connections")->getItems())
	{
		auto vconnection = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);

		int id_value;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("id"), id_value))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection ID\n");
			return false;
		}
		uint8_t id = id_value;

		std::string src;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("mac_src"), src))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection MAC source\n");
			return false;
		}
		MacAddress *mac_src = new MacAddress(src);

		std::string dst;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("mac_dst"), dst))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection MAC destination\n");
			return false;
		}
		MacAddress *mac_dst = new MacAddress(dst);

		int q_tci_value;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("tci_802_1q"), q_tci_value))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection TCI for 802.1q tag\n");
			return false;
		}
		uint16_t q_tci = q_tci_value;

		int ad_tci_value;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("tci_802_1ad"), ad_tci_value))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection TCI for 802.1ad tag\n");
			return false;
		}
		uint16_t ad_tci = ad_tci_value;

		std::string protocol;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("protocol"), protocol))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection protocol\n");
			return false;
		}

		std::stringstream proto{protocol};
		uint16_t pt;
		proto >> std::hex >> pt;

		LOG(this->log, LEVEL_INFO,
		    "New EVC: MAC source = %s, MAC destination = %s, "
		    "tag Q = %u, tag AD = %u, payload_type = %#2X\n",
		    mac_src->str().c_str(), mac_dst->str().c_str(),
		    q_tci, ad_tci, pt);

		if(this->evc_map.find(id) != this->evc_map.end())
		{
			LOG(this->log, LEVEL_ERROR,
			    "Duplicated ID %u in Ethernet Virtual Connections\n", id);
			return false;
		}

		Evc *evc = new Evc(mac_src, mac_dst, q_tci, ad_tci, pt);
		this->evc_map[id] = evc;
	}
	// initialize the statistics on EVC
	this->initStats();

	return true;
}

bool Ethernet::Context::initTrafficCategories()
{
	auto network = OpenSandModelConf::Get()->getProfileData()->getComponent("network");

	std::map<std::string, int> fifo_priorities;

	auto conf = OpenSandModelConf::Get();
	auto st_fifos = network->getList("st_fifos");
	auto gw_fifos = network->getList("gw_fifos");
	if(!((st_fifos == nullptr) ^ (gw_fifos == nullptr))) {
		LOG(this->log, LEVEL_ERROR,
		    "Exactly one of {st_fifos, gw_fifos} should be defined in the profile configuration file");
		return false;
	}
	auto fifos = st_fifos ? st_fifos : gw_fifos;
	
	for(auto& item : fifos->getItems())
	{
		auto fifo = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);

		int priority;
		if(!OpenSandModelConf::extractParameterData(fifo->getParameter("priority"), priority))
		{
			continue;
		}

		std::string name;
		if(!OpenSandModelConf::extractParameterData(fifo->getParameter("name"), name))
		{
			continue;
		}

		fifo_priorities[name] = priority;
	}
	
	for(auto& item : network->getList("qos_classes")->getItems())
	{
		auto category = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);

		int pcp;
		if(!OpenSandModelConf::extractParameterData(category->getParameter("pcp"), pcp))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing QoS class PCP parameter\n");
			return false;
		}

		std::string fifo_name;
		if(!OpenSandModelConf::extractParameterData(category->getParameter("fifo"), fifo_name))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing QoS class FIFO name parameter\n");
			return false;
		}

		auto priority = fifo_priorities.find(fifo_name);
		if(priority == fifo_priorities.end())
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing QoS class has unknown FIFO name %s\n",
			    fifo_name.c_str());
			return false;
		}
		int mac_queue_prio = priority->second;

		std::string class_name;
		if(!OpenSandModelConf::extractParameterData(category->getParameter("name"), class_name))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing QoS class FIFO priority parameter\n");
			return false;
		}

		if(this->category_map.count(pcp))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Traffic category %ld - [%s] rejected: identifier "
			    "already exists for [%s]\n", pcp,
			    class_name.c_str(),
			    this->category_map[pcp]->getName().c_str());
			return false;
		}

		TrafficCategory *traffic_category = new TrafficCategory();

		traffic_category->setId(mac_queue_prio);
		traffic_category->setName(class_name);
		this->category_map[pcp] = traffic_category;
	}

	auto default_pcp = network->getComponent("qos_settings")->getParameter("default_pcp");
	int default_category;
	if(!OpenSandModelConf::extractParameterData(default_pcp, default_category))
	{
		this->default_category = (this->category_map.begin())->first;
		LOG(this->log, LEVEL_ERROR,
		    "cannot find default MAC traffic category\n");
		return false;
	}

	this->default_category = default_category;
	return true;
}


bool Ethernet::Context::initLanAdaptationContext(
	tal_id_t tal_id,
	tal_id_t gw_id,
	PacketSwitch *packet_switch)
{
	if(!LanAdaptationPlugin::LanAdaptationContext::initLanAdaptationContext(tal_id, gw_id, 
										packet_switch))
	{
		return false;
	}
	return true;
}


NetBurst *Ethernet::Context::encapsulate(NetBurst *burst,
                                         std::map<long, int> &UNUSED(time_contexts))
{
	NetBurst::iterator packet;

	if(this->current_upper)
	{
		LOG(this->log, LEVEL_INFO,
		    "got a burst of %s packets to encapsulate\n",
		    this->current_upper->getName().c_str());
	}
	else
	{
		LOG(this->log, LEVEL_INFO,
		    "got a network packet to encapsulate\n");
	}

	// create an empty burst of ETH frames
	NetBurst *eth_frames = nullptr;
	try
	{
		eth_frames = new NetBurst();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of ETH frames\n");
		delete burst;
		return nullptr;
	}

	for(auto&& packet : *burst)
	{
		std::unique_ptr<NetPacket> eth_frame;
		uint8_t evc_id = 0;

		if(this->current_upper)
		{
			// we have to create the Ethernet header from scratch,
			// try to find an EVC and create the header with given information
			eth_frame = this->createEthFrameData(packet, evc_id);
			if(!eth_frame)
			{
				continue;
			}
		}
		else
		{
			size_t header_length;
			uint16_t ether_type = Ethernet::getPayloadEtherType(packet->getData());
			uint16_t frame_type = Ethernet::getFrameType(packet->getData());
			MacAddress src_mac = Ethernet::getSrcMac(packet->getData());
			MacAddress dst_mac = Ethernet::getDstMac(packet->getData());
			tal_id_t src = 255 ;
			tal_id_t dst = 255;
			uint16_t q_tci = Ethernet::getQTci(packet->getData());
			uint16_t ad_tci = Ethernet::getAdTci(packet->getData());
			qos_t pcp = (q_tci & 0xe000) >> 13;
			qos_t qos = 0;
			Evc *evc;
			std::map<qos_t, TrafficCategory *>::const_iterator default_category;
			std::map<qos_t, TrafficCategory *>::const_iterator found_category;
			SarpTable *sarp_table = BlockLanAdaptation::packet_switch->getSarpTable();

			// Do not print errors here because we may want to reject trafic as spanning
			// tree coming from miscellaneous host
			if(!BlockLanAdaptation::packet_switch->getPacketDestination(packet->getData(), src, dst))
			{
				// check default tal_id
				if(dst > BROADCAST_TAL_ID)
				{
					LOG(this->log, LEVEL_WARNING,
					    "cannot find destination MAC address %s in sarp table\n",
					    dst_mac.str().c_str());
					continue;
				}
				else
				{
					LOG(this->log, LEVEL_NOTICE,
					    "cannot find destination tal ID, use default (%u)\n",
					    dst);
				}
			}
			LOG(this->log, LEVEL_INFO,
			    "build Ethernet frame with source MAC %s corresponding "
			    " to terminal ID %d and destination MAC %s corresponding "
			    "to terminal ID %d\n",
			    src_mac.str().c_str(), src, dst_mac.str().c_str(), dst);

			// get default QoS value
			default_category = this->category_map.find(this->default_category);
			if(default_category == this->category_map.end())
			{
				LOG(this->log, LEVEL_ERROR,
				    "Unable to find default category for QoS");
				continue;
			}

			switch(frame_type)
			{
				case NET_PROTO_ETH:
					header_length = ETHERNET_2_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, ether_type, evc_id);
					qos = default_category->second->getId();
					break;
				case NET_PROTO_802_1Q:
					header_length = ETHERNET_802_1Q_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, q_tci, ether_type, evc_id);
					LOG(this->log, LEVEL_INFO,
					    "TCI = %u\n", q_tci);
					break;
				case NET_PROTO_802_1AD:
					header_length = ETHERNET_802_1AD_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, q_tci, ad_tci, ether_type, evc_id);
					LOG(this->log, LEVEL_INFO,
					    "Outer TCI = %u, Inner TCI = %u\n", ad_tci, q_tci);
					break;
				default:
					LOG(this->log, LEVEL_ERROR,
					    "wrong Ethernet frame type 0x%.4x\n", frame_type);
					continue;
			}
			if(!evc)
			{
				LOG(this->log, LEVEL_INFO,
				    "cannot find EVC for this flow, use the default values\n");
			}

			if(frame_type != NET_PROTO_ETH)
			{
				// get the QoS from the PCP is there is a PCP
				found_category = this->category_map.find(pcp);
				if(found_category == this->category_map.end())
				{
					found_category = this->category_map.find(this->default_category);
					if(found_category == this->category_map.end())
						continue;
				}
				qos = found_category->second->getId();
				LOG(this->log, LEVEL_INFO,
				    "PCP = %u corresponding to queue %s (%u)\n", pcp,
				    found_category->second->getName().c_str(), qos);
			}

			if(frame_type != this->sat_frame_type)
			{
				if(evc)
				{
					// Retrieve every field, we may already have it but no need to
					// handle every condition if we do that
					q_tci = (evc->getQTci() & 0xffff);
					ad_tci = (evc->getAdTci() & 0xffff);
					qos_t pcp = (evc->getQTci() & 0xe000) > 13;
					found_category = this->category_map.find(pcp);
					if(found_category == this->category_map.end())
					{
						found_category = this->category_map.find(this->default_category);
						if(found_category == this->category_map.end())
							continue;
					}
					qos = found_category->second->getId();
					LOG(this->log, LEVEL_INFO,
					    "PCP in EVC is %u corresponding to QoS %u for DVB layer\n",
					    pcp, qos);
				}
				// TODO we should cast to an EthernetPacket and use getPayload instead
				eth_frame = this->createEthFrameData(packet->getData().substr(header_length),
				                                     src_mac, dst_mac,
				                                     ether_type,
				                                     q_tci, ad_tci,
				                                     qos, src, dst,
				                                     this->sat_frame_type);
			}
			else
			{
				eth_frame = this->createPacket(packet->getData(),
				                               packet->getTotalLength(),
				                               qos, src, dst);
			}

			if(eth_frame == nullptr)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create the Ethernet frame\n");
				continue;
			}
		}

		if(this->evc_data_size.find(evc_id) == this->evc_data_size.end())
		{
			// create the element
			this->evc_data_size[evc_id] = 0;
		}
		this->evc_data_size[evc_id] += eth_frame->getTotalLength();
		eth_frames->add(std::move(eth_frame));
	}
	LOG(this->log, LEVEL_INFO,
	    "encapsulate %zu Ethernet frames\n", eth_frames->size());

	// delete the burst and all frames in it
	delete burst;

	// avoid returning empty bursts
	if(eth_frames->size() > 0)
	{
		return eth_frames;
	}
	delete eth_frames;
	return nullptr;
}


NetBurst *Ethernet::Context::deencapsulate(NetBurst *burst)
{
	if(burst == nullptr || burst->front() == nullptr)
	{
		LOG(this->log, LEVEL_ERROR,
		    "empty burst received\n");
    delete burst;
		return nullptr;
	}

	LOG(this->log, LEVEL_INFO,
	    "got a burst of %s packets to deencapsulate\n",
	    (burst->front())->getName().c_str());

	// create an empty burst of network frames
	NetBurst *net_packets = nullptr;
  try
  {
    net_packets = new NetBurst();
  }
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of network frames\n");
		delete burst;
		return nullptr;
	}

	for(auto&& packet : *burst)
	{
    std::unique_ptr<NetPacket> deenc_packet;
		size_t data_length = packet->getTotalLength();
		MacAddress dst_mac = Ethernet::getDstMac(packet->getData());
		MacAddress src_mac = Ethernet::getSrcMac(packet->getData());
		uint16_t q_tci = Ethernet::getQTci(packet->getData());
		uint16_t ad_tci = Ethernet::getAdTci(packet->getData());
		uint16_t ether_type = Ethernet::getPayloadEtherType(packet->getData());
		uint16_t frame_type = Ethernet::getFrameType(packet->getData());
		Evc *evc;
		size_t header_length;
		uint8_t evc_id = 0;
		SarpTable *sarp_table = BlockLanAdaptation::packet_switch->getSarpTable();

		switch(frame_type)
		{
			case NET_PROTO_ETH:
				header_length = ETHERNET_2_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, ether_type, evc_id);
				break;
			case NET_PROTO_802_1Q:
				header_length = ETHERNET_802_1Q_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, q_tci, ether_type, evc_id);
				break;
			case NET_PROTO_802_1AD:
				header_length = ETHERNET_802_1AD_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, q_tci, ad_tci, ether_type, evc_id);
				break;
			default:
				LOG(this->log, LEVEL_ERROR,
				    "wrong Ethernet frame type 0x%.4x\n", frame_type);
				evc_id = 0;
				continue;
		}

		if(this->evc_data_size.find(evc_id) == this->evc_data_size.end())
		{
			this->evc_data_size[evc_id] = 0;
		}
		this->evc_data_size[evc_id] += data_length;

		LOG(this->log, LEVEL_INFO,
		    "Ethernet frame received: src: %s, dst %s, Q-tag: %u, "
		    "ad-tag: %u, EtherType: 0x%.4x\n",
		    src_mac.str().c_str(), dst_mac.str().c_str(),
		    q_tci, ad_tci, ether_type);

		if(this->current_upper)
		{
			if(ether_type == NET_PROTO_ARP && this->current_upper->getName() == "IP")
			{
				LOG(this->log, LEVEL_WARNING,
				    "ARP is not supported on IP layer at the moment, drop it\n");
				continue;
			}

			// strip eth header to get to IP
			deenc_packet = this->current_upper->build(packet->getPayload(),
			                                          packet->getPayloadLength(),
			                                          packet->getQos(),
			                                          packet->getSrcTalId(),
			                                          packet->getDstTalId());
		}
		else
		{
			// TODO factorize
			tal_id_t dst;

			// Here we have errors because if we received this packets
			// the information should be in sarp table
			/*if(!sarp_table->getTalByMac(dst_mac, dst))
			{
				// check default tal_id
				if(dst > BROADCAST_TAL_ID)
				{
					LOG(this->log, LEVEL_WARNING,
					    "cannot find destination MAC address %s in sarp table\n",
					    dst_mac.str().c_str());
					delete deenc_packet;
					continue;
				}
				else
				{
					LOG(this->log, LEVEL_NOTICE,
					    "cannot find destination tal ID, use default (%u)\n",
					    dst);
				}
			}*/

			if(frame_type != this->lan_frame_type)
			{
				if(evc)
				{
					q_tci = (evc->getQTci() & 0xffff);
					ad_tci = (evc->getAdTci() & 0xffff);
				}
				// TODO we should cast to an EthernetPacket and use getPayload instead
				deenc_packet = this->createEthFrameData(packet->getData().substr(header_length),
				                                        src_mac, dst_mac,
				                                        ether_type,
				                                        q_tci, ad_tci,
				                                        packet->getQos(),
				                                        packet->getSrcTalId(),
				                                        dst,
				                                        this->lan_frame_type);
			}
			else
			{
				// create ETH packet
				deenc_packet = this->createPacket(packet->getData(),
				                                  data_length,
				                                  packet->getQos(),
				                                  packet->getSrcTalId(),
				                                  dst);
			}
		}
		if(!deenc_packet)
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to deencapsulated Ethernet frame\n");
			continue;
		}

		net_packets->add(std::move(deenc_packet));
	}
	LOG(this->log, LEVEL_INFO,
	    "deencapsulate %zu ethernet frames\n",
	    net_packets->size());

	// delete the burst and all frames in it
	delete burst;
	return net_packets;
}


std::unique_ptr<NetPacket> Ethernet::Context::createEthFrameData(const std::unique_ptr<NetPacket>& packet,
                                                                 uint8_t &evc_id)
{
	std::vector<MacAddress> src_macs;
	std::vector<MacAddress> dst_macs;
	MacAddress src_mac;
	MacAddress dst_mac;
	tal_id_t src_tal = packet->getSrcTalId();
	tal_id_t dst_tal = packet->getDstTalId();
	qos_t qos = packet->getQos();
	uint16_t q_tci = 0;
	uint16_t ad_tci = 0;
	uint16_t ether_type = packet->getType();
	Evc *evc = NULL;
	SarpTable *sarp_table = BlockLanAdaptation::packet_switch->getSarpTable();

	// search traffic category associated with QoS value
	// TODO we should filter on IP addresses instead of QoS
	for(std::map<qos_t, TrafficCategory *>::const_iterator it = this->category_map.begin();
	    it != this->category_map.end(); ++it)
	{
		if((*it).second->getId() == qos)
		{
			ad_tci = (*it).first;
		}
	}

	// TODO here we use the ad_tci to store the qos in order to be able to find
	//      an EVC, this is a bad workaround
	if(!sarp_table->getMacByTal(src_tal, src_macs))
	{
		LOG(this->log, LEVEL_ERROR,
		    "unable to find MAC address associated with terminal with ID %u\n",
		    src_tal);
		return NULL;
	}
	if(!sarp_table->getMacByTal(dst_tal, dst_macs))
	{
		LOG(this->log, LEVEL_ERROR,
		    "unable to find MAC address associated with terminal with ID %u\n",
		    dst_tal);
		return NULL;
	}
	for(std::vector<MacAddress>::iterator it1 = src_macs.begin();
	    it1 != src_macs.end(); ++it1)
	{
		for(std::vector<MacAddress>::iterator it2 = dst_macs.begin();
		    it2 != dst_macs.end(); ++it2)
		{
			// TODO remove tags from here and search with IP addresses
			evc = this->getEvc(*it1, *it2, q_tci, ad_tci, ether_type, evc_id);
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
		std::map<qos_t, TrafficCategory *>::const_iterator default_category;
		LOG(this->log, LEVEL_NOTICE,
		    "no EVC for this flow, use default values");
		src_mac = src_macs.front();
		dst_mac = dst_macs.front();
		// get default QoS value
		default_category = this->category_map.find(this->default_category);
		if(default_category != this->category_map.end())
		{
			ad_tci = default_category->first;
		}
		evc_id = 0;
	}
	else
	{
		q_tci = (evc->getQTci() & 0xffff);
		ad_tci = (evc->getAdTci() & 0xffff);
		src_mac = *(evc->getMacSrc());
		dst_mac = *(evc->getMacDst());
	}
	return this->createEthFrameData(packet->getData(),
	                                src_mac,
	                                dst_mac,
	                                ether_type,
	                                q_tci,
	                                ad_tci,
	                                qos,
	                                src_tal,
	                                dst_tal,
	                                this->sat_frame_type);
}


std::unique_ptr<NetPacket> Ethernet::Context::createEthFrameData(Data data,
                                                                 MacAddress src_mac,
                                                                 MacAddress dst_mac,
                                                                 uint16_t ether_type,
                                                                 uint16_t q_tci,
                                                                 uint16_t ad_tci,
                                                                 qos_t qos,
                                                                 tal_id_t src_tal_id,
                                                                 tal_id_t dst_tal_id,
                                                                 uint16_t desired_frame_type)
{
	eth_2_header_t *eth_2_hdr;
	eth_1q_header_t *eth_1q_hdr;
	eth_1ad_header_t *eth_1ad_hdr;

	unsigned char header[ETHERNET_802_1AD_HEADSIZE];

	// common part for all header
	eth_2_hdr = (eth_2_header_t *) header;
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
			data.insert(0, header, ETHERNET_2_HEADSIZE);
			LOG(this->log, LEVEL_INFO,
			    "create an Ethernet frame with src = %s, "
			    "dst = %s\n", src_mac.str().c_str(), dst_mac.str().c_str());
			break;
		case NET_PROTO_802_1Q:
			eth_1q_hdr = (eth_1q_header_t *) header;
			eth_1q_hdr->TPID = htons(NET_PROTO_802_1Q);
			eth_1q_hdr->TCI.tci = htons(q_tci);
			eth_1q_hdr->ether_type = htons(ether_type);
			data.insert(0, header, ETHERNET_802_1Q_HEADSIZE);
			LOG(this->log, LEVEL_INFO,
			    "create a 802.1Q frame with src = %s, "
			    "dst = %s, VLAN ID = %d\n", src_mac.str().c_str(),
			    dst_mac.str().c_str(), q_tci);
			break;
		case NET_PROTO_802_1AD:
			eth_1ad_hdr = (eth_1ad_header_t *) header;
			// TODO use NET_PROTO_802_1AD once kernel will support it
			eth_1ad_hdr->outer_TPID = htons(NET_PROTO_802_1Q);
			//eth_1ad_hdr->outer_TPID = htons(NET_PROTO_802_1AD);
			eth_1ad_hdr->outer_TCI.tci = htons(ad_tci);
			eth_1ad_hdr->inner_TPID = htons(NET_PROTO_802_1Q);
			eth_1ad_hdr->inner_TCI.tci = htons(q_tci);
			eth_1ad_hdr->ether_type = htons(ether_type);
			data.insert(0, header, ETHERNET_802_1AD_HEADSIZE);
			LOG(this->log, LEVEL_INFO,
			    "create a 802.1AD frame with src = %s, "
			    "dst = %s, q-tag = %u, ad-tag = %u\n",
			    src_mac.str().c_str(), dst_mac.str().c_str(), q_tci, ad_tci);
			break;
		default:
			LOG(this->log, LEVEL_ERROR,
			    "Bad protocol value (0x%.4x) for Ethernet plugin\n",
			    desired_frame_type);
			return NULL;
	}
	return this->createPacket(data, data.length(), qos,
	                          src_tal_id, dst_tal_id);

}

char Ethernet::Context::getLanHeader(unsigned int, const std::unique_ptr<NetPacket>&)
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
	uint8_t id;
  auto output = Output::Get();

	// create default probe with EVC=0 if it does no exist
	id = 0;
	// TODO try to do default in and default out
	// we can receive any type of frames
	this->probe_evc_throughput[id] =
		output->registerProbe<float>("EVC throughput.default",
		                             "kbits/s", true, SAMPLE_AVG);
	this->probe_evc_size[id] =
		output->registerProbe<float>("EVC frame size.default",
		                             "Bytes", true, SAMPLE_SUM);

	for(std::map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
	    it != this->evc_map.end(); ++it)
	{
		char probe_name[128];
		id = (*it).first;
		if(this->probe_evc_throughput.find(id) != this->probe_evc_throughput.end())
		{
			continue;
		}

		snprintf(probe_name, sizeof(probe_name),
		         "EVC throughput.%u", id);
		this->probe_evc_throughput[id] =
			output->registerProbe<float>(probe_name, "kbits/s", true, SAMPLE_AVG);
		snprintf(probe_name, sizeof(probe_name),
		         "EVC frame size.%u", id);
		this->probe_evc_size[id] =
			output->registerProbe<float>(probe_name, "Bytes", true, SAMPLE_SUM);
	}
}

void Ethernet::Context::updateStats(unsigned int period)
{
	std::map<uint8_t, size_t>::iterator it;
	std::map<uint8_t, Probe<float> *>::iterator found;
	uint8_t id;
	std::stringstream name;

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

std::unique_ptr<NetPacket> Ethernet::PacketHandler::build(const Data &data,
                                                          std::size_t data_length,
                                                          uint8_t qos,
                                                          uint8_t src_tal_id,
                                                          uint8_t dst_tal_id) const
{
	size_t head_length = 0;
	uint16_t frame_type = Ethernet::getFrameType(data);
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

	return std::unique_ptr<NetPacket>(new NetPacket(data, data_length,
	                                                this->getName(),
	                                                frame_type,
	                                                qos,
	                                                src_tal_id,
	                                                dst_tal_id,
	                                                head_length));
}

Evc *Ethernet::Context::getEvc(const MacAddress src_mac,
                               const MacAddress dst_mac,
                               uint16_t ether_type,
                               uint8_t &evc_id) const
{
	for(std::map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
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
                               uint16_t q_tci,
                               uint16_t ether_type,
                               uint8_t &evc_id) const
{
	for(std::map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
	    it != this->evc_map.end(); ++it)
	{
		if((*it).second->matches(&src_mac, &dst_mac, q_tci, ether_type))
		{
			evc_id = (*it).first;
			return (*it).second;
		}
	}
	return NULL;
}


Evc *Ethernet::Context::getEvc(const MacAddress src_mac,
                               const MacAddress dst_mac,
                               uint16_t q_tci,
                               uint16_t ad_tci,
                               uint16_t ether_type,
                               uint8_t &evc_id) const
{
	for(std::map<uint8_t, Evc *>::const_iterator it = this->evc_map.begin();
	    it != this->evc_map.end(); ++it)
	{
		if((*it).second->matches(&src_mac, &dst_mac, q_tci, ad_tci, ether_type))
		{
			evc_id = (*it).first;
			return (*it).second;
		}
	}
	return NULL;
}


// TODO ENDIANESS !
uint16_t Ethernet::getFrameType(const Data &data)
{
	uint16_t ether_type = NET_PROTO_ERROR;
	uint16_t ether_type2 = NET_PROTO_ERROR;
	if(data.length() < 13)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve EtherType in Ethernet header\n");
		return NET_PROTO_ERROR;
	}
	// read ethertype: 2 bytes at a 12 bytes offset
	ether_type = (data.at(12) << 8) | data.at(13);
	ether_type2 = (data.at(16) << 8) | data.at(17);
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

uint16_t Ethernet::getPayloadEtherType(const Data &data)
{
	uint16_t ether_type = NET_PROTO_ERROR;
	if(data.length() < 13)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve EtherType in Ethernet header\n");
		return NET_PROTO_ERROR;
	}
	// read ethertype: 2 bytes at a 12 bytes offset
	ether_type = (data.at(12) << 8) | data.at(13);
	switch(ether_type)
	{
		case NET_PROTO_802_1Q:
			if(data.length() < 17)
			{
				DFLTLOG(LEVEL_ERROR,
				        "cannot retrieve EtherType in Ethernet header\n");
				return NET_PROTO_ERROR;
			}
			ether_type = (data.at(16) << 8) | data.at(17);

			// TODO: we need the following part because we use two 802.1Q
			//       tags for kernel support
			if(ether_type != NET_PROTO_802_1Q)
			{
				break;
			}
			// fall through
		case NET_PROTO_802_1AD:
			if(data.length() < 21)
			{
				DFLTLOG(LEVEL_ERROR,
				        "cannot retrieve EtherType in Ethernet header\n");
				return NET_PROTO_ERROR;
			}
			ether_type = (data.at(20) << 8) | data.at(21);
			break;
	}

	return ether_type;
}

uint16_t Ethernet::getQTci(const Data &data)
{
	uint16_t tci = 0;
	uint16_t ether_type;
	if(data.length() < 17)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve vlan id in Ethernet header\n");
		return 0;
	}
	ether_type = (data.at(12) << 8) | data.at(13);
	switch(ether_type)
	{
		case NET_PROTO_802_1Q:
			tci = ((data.at(14) & 0xff) << 8) | data.at(15);
			// TODO: we need the following part because we use two 802.1Q
			//       tags for kernel support
			ether_type = (data.at(16) << 8) | data.at(17);
			if(ether_type != NET_PROTO_802_1Q)
			{
				break;
			}
			// fall through
		case NET_PROTO_802_1AD:
			tci = ((data.at(18) & 0xff) << 8) | data.at(19);
			break;
	}

	return tci;
}

uint16_t Ethernet::getAdTci(const Data &data)
{
	uint16_t tci = 0;
	uint16_t ether_type;
	uint16_t ether_type2;
	if(data.length() < 17)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve vlan id in Ethernet header\n");
		return 0;
	}
	ether_type = (data.at(12) << 8) | data.at(13);
	ether_type2 = (data.at(16) << 8) | data.at(17);
	// TODO: we need the following part because we use two 802.1Q tags for kernel support
	if(ether_type == NET_PROTO_802_1Q && ether_type2 == NET_PROTO_802_1Q)
	{
		ether_type = NET_PROTO_802_1AD;
	}

	switch(ether_type)
	{
		case NET_PROTO_802_1AD:
			tci = ((data.at(14) & 0xff) << 8) | data.at(15);
			break;
	}

	return tci;
}

MacAddress Ethernet::getDstMac(const Data &data)
{
	if(data.length() < 6)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve destination MAC in Ethernet header\n");
		return MacAddress(0, 0, 0, 0, 0, 0);
	}

	return MacAddress(data.at(0), data.at(1), data.at(2),
	                  data.at(3), data.at(4), data.at(5));
}

MacAddress Ethernet::getSrcMac(const Data &data)
{
	if(data.length() < 12)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve source MAC in Ethernet header\n");
		return MacAddress(0, 0, 0, 0, 0, 0);
	}
	return MacAddress(data.at(6), data.at(7), data.at(8),
	                  data.at(9), data.at(10), data.at(11));
}

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
#include "OpenSandCore.h"

#include <opensand_output/Output.h>

#include <vector>
#include <map>
#include <arpa/inet.h>


Ethernet::Ethernet():
	LanAdaptationPlugin(NET_PROTO::ETH)
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
	categories->addParameter("fifo", "Fifo Name", types->getType("string"));

	auto evcs = conf->addList("virtual_connections", "Virtual Connections", "virtual_connection")->getPattern();
	evcs->setAdvanced(true);
	evcs->addParameter("id", "Connection ID", types->getType("ubyte"));
	evcs->addParameter("mac_src", "Source MAC Address", types->getType("string"));
	evcs->addParameter("mac_dst", "Destination MAC Address", types->getType("string"));
	evcs->addParameter("tci_802_1q", "TCI of the 802.1q tag", types->getType("ushort"));
	evcs->addParameter("tci_802_1ad", "TCI of the 802.1ad tag", types->getType("ushort"));
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
		this->ether_type = NET_PROTO::ETH;
	}
	else if(sat_eth == "802.1Q")
	{
		this->ether_type = NET_PROTO::IEEE_802_1Q;
	}
	else if(sat_eth == "802.1ad")
	{
		this->ether_type = NET_PROTO::IEEE_802_1AD;
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "unknown type of Ethernet frames\n");
		this->ether_type = NET_PROTO::ERROR;
	}
	return true;
}

Ethernet::Context::Context(LanAdaptationPlugin &plugin):
	LanAdaptationContext(plugin),
	default_category{nullptr}
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
		return false;
	}

	std::string sat_eth;
	auto sat_qos = qos->getParameter("sat_frame_type");
	if(!OpenSandModelConf::extractParameterData(sat_qos, sat_eth))
	{
		LOG(this->log, LEVEL_ERROR,
		    "Section QoS settings, missing parameter satellite frame type\n");
		return false;
	}

	if(!this->initEvc())
	{
		LOG(this->log, LEVEL_ERROR,
		    "failed to Initialize EVC\n");
		return false;
	}

	if(!this->initTrafficCategories())
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot Initialize traffic categories\n");
		return false;
	}

	if(lan_eth == "Ethernet")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer without extension on network\n");
		this->lan_frame_type = NET_PROTO::ETH;
	}
	else if(lan_eth == "802.1Q")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1Q extension on network\n");
		this->lan_frame_type = NET_PROTO::IEEE_802_1Q;
	}
	else if(lan_eth == "802.1ad")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1ad extension on network\n");
		this->lan_frame_type = NET_PROTO::IEEE_802_1AD;
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "unknown type of Ethernet layer on network\n");
		this->lan_frame_type = NET_PROTO::ERROR;
	}

	if(sat_eth == "Ethernet")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer without extension on satellite\n");
		this->sat_frame_type = NET_PROTO::ETH;
	}
	else if(sat_eth == "802.1Q")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1Q extension on satellite\n");
		this->sat_frame_type = NET_PROTO::IEEE_802_1Q;
	}
	else if(sat_eth == "802.1ad")
	{
		LOG(this->log, LEVEL_INFO,
		    "Ethernet layer support 802.1ad extension on satellite\n");
		this->sat_frame_type = NET_PROTO::IEEE_802_1AD;
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "unknown type of Ethernet layer on satellite\n");
		this->sat_frame_type = NET_PROTO::ERROR;
	}

	return true;
}

Ethernet::Context::~Context()
{
	this->evc_map.clear();
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

		uint8_t id;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("id"), id))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection ID\n");
			return false;
		}

		std::string src;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("mac_src"), src))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection MAC source\n");
			return false;
		}
		MacAddress mac_src{src};

		std::string dst;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("mac_dst"), dst))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection MAC destination\n");
			return false;
		}
		MacAddress mac_dst{dst};

		uint16_t q_tci;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("tci_802_1q"), q_tci))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection TCI for 802.1q tag\n");
			return false;
		}

		uint16_t ad_tci;
		if(!OpenSandModelConf::extractParameterData(vconnection->getParameter("tci_802_1ad"), ad_tci))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing virtual connection TCI for 802.1ad tag\n");
			return false;
		}

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
		    mac_src.str(), mac_dst.str(),
		    q_tci, ad_tci, pt);

		if(this->evc_map.find(id) != this->evc_map.end())
		{
			LOG(this->log, LEVEL_ERROR,
			    "Duplicated ID %u in Ethernet Virtual Connections\n", id);
			return false;
		}

		auto evc = std::make_unique<Evc>(mac_src, mac_dst, q_tci, ad_tci, to_enum<NET_PROTO>(pt));
		this->evc_map.emplace(id, std::move(evc));
	}
	// initialize the statistics on EVC
	this->initStats();

	return true;
}

bool Ethernet::Context::initTrafficCategories()
{
	auto network = OpenSandModelConf::Get()->getProfileData()->getComponent("network");

	std::map<std::string, int> fifo_priorities;
	
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
			priority = fifo_priorities.emplace(fifo_name, fifo_priorities.size()).first;
		}
		int mac_queue_prio = priority->second;

		std::string class_name;
		if(!OpenSandModelConf::extractParameterData(category->getParameter("name"), class_name))
		{
			LOG(this->log, LEVEL_ERROR,
			    "Section network, missing QoS class FIFO priority parameter\n");
			return false;
		}

		auto existing_category = this->category_map.find(pcp);
		if(existing_category != this->category_map.end())
		{
			LOG(this->log, LEVEL_ERROR,
			    "Traffic category %ld - [%s] rejected: identifier "
			    "already exists for [%s]\n", pcp,
			    class_name,
			    existing_category->second->getName());
			return false;
		}

		std::unique_ptr<TrafficCategory> traffic_category = std::make_unique<TrafficCategory>(pcp);

		traffic_category->setId(mac_queue_prio);
		traffic_category->setName(class_name);
		this->category_map.emplace(pcp, std::move(traffic_category));
	}

	auto default_pcp = network->getComponent("qos_settings")->getParameter("default_pcp");
	int default_category;
	if(!OpenSandModelConf::extractParameterData(default_pcp, default_category))
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot find default MAC traffic category\n");
		return false;
	}

	auto found_default_category = this->category_map.find(default_category);
	if (found_default_category == this->category_map.end())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Default PCP level does not map to a registered traffic category");
		return false;
	}
	this->default_category = found_default_category->second.get();
	
	return true;
}


bool Ethernet::Context::initLanAdaptationContext(tal_id_t tal_id, std::shared_ptr<PacketSwitch> packet_switch)
{
	return LanAdaptationPlugin::LanAdaptationContext::initLanAdaptationContext(tal_id, packet_switch);
}


Rt::Ptr<NetBurst> Ethernet::Context::encapsulate(Rt::Ptr<NetBurst> burst, std::map<long, int> &)
{
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
	Rt::Ptr<NetBurst> eth_frames = Rt::make_ptr<NetBurst>(nullptr);
	try
	{
		eth_frames = Rt::make_ptr<NetBurst>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of ETH frames\n");
		return Rt::make_ptr<NetBurst>(nullptr);
	}

	for(auto&& packet : *burst)
	{
		Rt::Ptr<NetPacket> eth_frame = Rt::make_ptr<NetPacket>(nullptr);
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
			NET_PROTO ether_type = Ethernet::getPayloadEtherType(packet->getData());
			NET_PROTO frame_type = Ethernet::getFrameType(packet->getData());
			MacAddress src_mac = Ethernet::getSrcMac(packet->getData());
			MacAddress dst_mac = Ethernet::getDstMac(packet->getData());
			tal_id_t src = 255 ;
			tal_id_t dst = 255;
			uint16_t q_tci = Ethernet::getQTci(packet->getData());
			uint16_t ad_tci = Ethernet::getAdTci(packet->getData());
			qos_t pcp = (q_tci & 0xe000) >> 13;
			qos_t qos = 0;
			Evc *evc;

			// Do not print errors here because we may want to reject trafic as spanning
			// tree coming from miscellaneous host
			if(!packet_switch->getPacketDestination(packet->getData(), src, dst))
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

			switch(frame_type)
			{
				case NET_PROTO::ETH:
					header_length = ETHERNET_2_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, ether_type, evc_id);
					qos = this->default_category->getId();
					break;
				case NET_PROTO::IEEE_802_1Q:
					header_length = ETHERNET_802_1Q_HEADSIZE;
					evc = this->getEvc(src_mac, dst_mac, q_tci, ether_type, evc_id);
					LOG(this->log, LEVEL_INFO,
					    "TCI = %u\n", q_tci);
					break;
				case NET_PROTO::IEEE_802_1AD:
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

			if(frame_type != NET_PROTO::ETH)
			{
				// get the QoS from the PCP if there is a PCP
				auto found_category = this->category_map.find(pcp);
				if (found_category == this->category_map.end())
				{
					qos = this->default_category->getId();
				}
				else
				{
					qos = found_category->second->getId();
				}
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
					auto found_category = this->category_map.find(pcp);
					if (found_category == this->category_map.end())
					{
						qos = this->default_category->getId();
					}
					else
					{
						qos = found_category->second->getId();
					}
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

	// avoid returning empty bursts
	if(eth_frames->size() > 0)
	{
		// copy elision here, no move needed
		return eth_frames;
	}

	return Rt::make_ptr<NetBurst>(nullptr);
}


Rt::Ptr<NetBurst> Ethernet::Context::deencapsulate(Rt::Ptr<NetBurst> burst)
{
	if(burst == nullptr || burst->front() == nullptr)
	{
		LOG(this->log, LEVEL_ERROR,
		    "empty burst received\n");
		return Rt::make_ptr<NetBurst>(nullptr);
	}

	LOG(this->log, LEVEL_INFO,
	    "got a burst of %s packets to deencapsulate\n",
	    (burst->front())->getName().c_str());

	// create an empty burst of network frames
	Rt::Ptr<NetBurst> net_packets = Rt::make_ptr<NetBurst>(nullptr);
	try
	{
		net_packets = Rt::make_ptr<NetBurst>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of network frames\n");
		return Rt::make_ptr<NetBurst>(nullptr);
	}

	for(auto&& packet : *burst)
	{
		Rt::Ptr<NetPacket> deenc_packet = Rt::make_ptr<NetPacket>(nullptr);
		size_t data_length = packet->getTotalLength();
		MacAddress dst_mac = Ethernet::getDstMac(packet->getData());
		MacAddress src_mac = Ethernet::getSrcMac(packet->getData());
		uint16_t q_tci = Ethernet::getQTci(packet->getData());
		uint16_t ad_tci = Ethernet::getAdTci(packet->getData());
		NET_PROTO ether_type = Ethernet::getPayloadEtherType(packet->getData());
		NET_PROTO frame_type = Ethernet::getFrameType(packet->getData());
		Evc *evc;
		size_t header_length;
		uint8_t evc_id = 0;

		switch(frame_type)
		{
			case NET_PROTO::ETH:
				header_length = ETHERNET_2_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, ether_type, evc_id);
				break;
			case NET_PROTO::IEEE_802_1Q:
				header_length = ETHERNET_802_1Q_HEADSIZE;
				evc = this->getEvc(src_mac, dst_mac, q_tci, ether_type, evc_id);
				break;
			case NET_PROTO::IEEE_802_1AD:
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
			if(ether_type == NET_PROTO::ARP && this->current_upper->getName() == "IP")
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
			tal_id_t dst{};  // Do we need a proper initial value here?

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

	// copy elision here, no move needed
	return net_packets;
}


Rt::Ptr<NetPacket> Ethernet::Context::createEthFrameData(const Rt::Ptr<NetPacket>& packet,
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
	NET_PROTO ether_type = packet->getType();
	Evc *evc = nullptr;
	SarpTable *sarp_table = packet_switch->getSarpTable();

	// search traffic category associated with QoS value
	// TODO we should filter on IP addresses instead of QoS
	for (auto &&[traffic_qos, traffic_category]: this->category_map)
	{
		if(traffic_category->getId() == qos)
		{
			ad_tci = traffic_qos;
		}
	}

	// TODO here we use the ad_tci to store the qos in order to be able to find
	//      an EVC, this is a bad workaround
	if(!sarp_table->getMacByTal(src_tal, src_macs))
	{
		LOG(this->log, LEVEL_ERROR,
		    "unable to find MAC address associated with terminal with ID %u\n",
		    src_tal);
		return Rt::make_ptr<NetPacket>(nullptr);
	}
	if(!sarp_table->getMacByTal(dst_tal, dst_macs))
	{
		LOG(this->log, LEVEL_ERROR,
		    "unable to find MAC address associated with terminal with ID %u\n",
		    dst_tal);
		return Rt::make_ptr<NetPacket>(nullptr);
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
		LOG(this->log, LEVEL_NOTICE,
		    "no EVC for this flow, use default values");
		src_mac = src_macs.front();
		dst_mac = dst_macs.front();
		// get default QoS value
		ad_tci = this->default_category->getPcp();
		evc_id = 0;
	}
	else
	{
		q_tci = (evc->getQTci() & 0xffff);
		ad_tci = (evc->getAdTci() & 0xffff);
		src_mac = evc->getMacSrc();
		dst_mac = evc->getMacDst();
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


Rt::Ptr<NetPacket> Ethernet::Context::createEthFrameData(Rt::Data data,
                                                         const MacAddress &src_mac,
                                                         const MacAddress &dst_mac,
                                                         NET_PROTO ether_type,
                                                         uint16_t q_tci,
                                                         uint16_t ad_tci,
                                                         qos_t qos,
                                                         tal_id_t src_tal_id,
                                                         tal_id_t dst_tal_id,
                                                         NET_PROTO desired_frame_type)
{
	eth_2_header_t *eth_2_hdr;
	eth_1q_header_t *eth_1q_hdr;
	eth_1ad_header_t *eth_1ad_hdr;

	unsigned char header[ETHERNET_802_1AD_HEADSIZE];
	uint16_t ether_type_value = to_underlying(ether_type);

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
		case NET_PROTO::ETH:
			eth_2_hdr->ether_type = htons(ether_type_value);
			data.insert(0, header, ETHERNET_2_HEADSIZE);
			LOG(this->log, LEVEL_INFO,
			    "create an Ethernet frame with src = %s, "
			    "dst = %s\n", src_mac.str().c_str(), dst_mac.str().c_str());
			break;
		case NET_PROTO::IEEE_802_1Q:
			eth_1q_hdr = (eth_1q_header_t *) header;
			eth_1q_hdr->TPID = htons(to_underlying(NET_PROTO::IEEE_802_1Q));
			eth_1q_hdr->TCI.tci = htons(q_tci);
			eth_1q_hdr->ether_type = htons(ether_type_value);
			data.insert(0, header, ETHERNET_802_1Q_HEADSIZE);
			LOG(this->log, LEVEL_INFO,
			    "create a 802.1Q frame with src = %s, "
			    "dst = %s, VLAN ID = %d\n", src_mac.str().c_str(),
			    dst_mac.str().c_str(), q_tci);
			break;
		case NET_PROTO::IEEE_802_1AD:
			eth_1ad_hdr = (eth_1ad_header_t *) header;
			// TODO use NET_PROTO::IEEE_802_1AD once kernel will support it
			eth_1ad_hdr->outer_TPID = htons(to_underlying(NET_PROTO::IEEE_802_1Q));
			//eth_1ad_hdr->outer_TPID = htons(NET_PROTO::IEEE_802_1AD);
			eth_1ad_hdr->outer_TCI.tci = htons(ad_tci);
			eth_1ad_hdr->inner_TPID = htons(to_underlying(NET_PROTO::IEEE_802_1Q));
			eth_1ad_hdr->inner_TCI.tci = htons(q_tci);
			eth_1ad_hdr->ether_type = htons(ether_type_value);
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
			return Rt::make_ptr<NetPacket>(nullptr);
	}
	return this->createPacket(data, data.length(), qos,
	                          src_tal_id, dst_tal_id);
}

char Ethernet::Context::getLanHeader(unsigned int, const Rt::Ptr<NetPacket>&)
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
	auto output = Output::Get();
	// create default probe with EVC=0 if it does no exist
	uint8_t id = 0;

	// TODO try to do default in and default out
	// we can receive any type of frames
	this->probe_evc_throughput[id] = output->registerProbe<float>("EVC throughput.default", "kbits/s", true, SAMPLE_AVG);
	this->probe_evc_size[id] = output->registerProbe<float>("EVC frame size.default", "Bytes", true, SAMPLE_SUM);

	for(auto &&evc_it: this->evc_map)
	{
		auto id = evc_it.first;
		if(this->probe_evc_throughput.find(id) != this->probe_evc_throughput.end())
		{
			continue;
		}

		this->probe_evc_throughput[id] = output->registerProbe<float>(Format("EVC throughput.%u", id), "kbits/s", true, SAMPLE_AVG);
		this->probe_evc_size[id] = output->registerProbe<float>(Format("EVC frame size.%u", id), "Bytes", true, SAMPLE_SUM);
	}
}

void Ethernet::Context::updateStats(const time_ms_t &period)
{
	for(auto it = this->evc_data_size.begin(); it != this->evc_data_size.end(); ++it)
	{
		uint8_t id = (*it).first;
		if(this->probe_evc_throughput.find(id) == this->probe_evc_throughput.end())
		{
			// use the default id
			id = 0;
		}
		this->probe_evc_throughput[id]->put(time_ms_t(it->second * 8) / period);
		this->probe_evc_size[id]->put(it->second);
		(*it).second = 0;
	}
}

Rt::Ptr<NetPacket> Ethernet::PacketHandler::build(const Rt::Data &data,
                                                  std::size_t data_length,
                                                  uint8_t qos,
                                                  uint8_t src_tal_id,
                                                  uint8_t dst_tal_id) const
{
	size_t head_length = 0;
	NET_PROTO frame_type = Ethernet::getFrameType(data);
	switch(frame_type)
	{
		case NET_PROTO::IEEE_802_1Q:
			head_length = ETHERNET_802_1Q_HEADSIZE;
			break;
		case NET_PROTO::IEEE_802_1AD:
			head_length = ETHERNET_802_1AD_HEADSIZE;
			break;
		// Ethernet packet, this is the ethertype of the payload
		default:
			head_length = ETHERNET_2_HEADSIZE;
			break;
	}

	return Rt::make_ptr<NetPacket>(data, data_length,
	                               this->getName(),
	                               frame_type,
	                               qos,
	                               src_tal_id,
	                               dst_tal_id,
	                               head_length);
}

Evc *Ethernet::Context::getEvc(const MacAddress &src_mac,
                               const MacAddress &dst_mac,
                               NET_PROTO ether_type,
                               uint8_t &evc_id) const
{
	for (auto &&[id, evc]: this->evc_map)
	{
		if(evc->matches(src_mac, dst_mac, ether_type))
		{
			evc_id = id;
			return evc.get();
		}
	}
	return nullptr;
}


Evc *Ethernet::Context::getEvc(const MacAddress &src_mac,
                               const MacAddress &dst_mac,
                               uint16_t q_tci,
                               NET_PROTO ether_type,
                               uint8_t &evc_id) const
{
	for (auto &&[id, evc]: this->evc_map)
	{
		if(evc->matches(src_mac, dst_mac, q_tci, ether_type))
		{
			evc_id = id;
			return evc.get();
		}
	}
	return nullptr;
}


Evc *Ethernet::Context::getEvc(const MacAddress &src_mac,
                               const MacAddress &dst_mac,
                               uint16_t q_tci,
                               uint16_t ad_tci,
                               NET_PROTO ether_type,
                               uint8_t &evc_id) const
{
	for (auto &&[id, evc]: this->evc_map)
	{
		if(evc->matches(src_mac, dst_mac, q_tci, ad_tci, ether_type))
		{
			evc_id = id;
			return evc.get();
		}
	}
	return nullptr;
}


// TODO ENDIANESS !
NET_PROTO Ethernet::getFrameType(const Rt::Data &data)
{
	NET_PROTO ether_type = NET_PROTO::ERROR;
	NET_PROTO ether_type2 = NET_PROTO::ERROR;
	if(data.length() < 13)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve EtherType in Ethernet header\n");
		return NET_PROTO::ERROR;
	}
	// read ethertype: 2 bytes at a 12 bytes offset
	ether_type = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(12) << 8) | data.at(13)));
	ether_type2 = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(16) << 8) | data.at(17)));
	if(ether_type != NET_PROTO::IEEE_802_1Q && ether_type != NET_PROTO::IEEE_802_1AD)
	{
		ether_type = NET_PROTO::ETH;
	}
	// TODO: we need the following part because we use two 802.1Q tags for kernel support
	else if(ether_type == NET_PROTO::IEEE_802_1Q && ether_type2 == NET_PROTO::IEEE_802_1Q)
	{
		ether_type = NET_PROTO::IEEE_802_1AD;
	}
	return ether_type;
}

NET_PROTO Ethernet::getPayloadEtherType(const Rt::Data &data)
{
	NET_PROTO ether_type = NET_PROTO::ERROR;
	if(data.length() < 13)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve EtherType in Ethernet header\n");
		return NET_PROTO::ERROR;
	}
	// read ethertype: 2 bytes at a 12 bytes offset
	ether_type = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(12) << 8) | data.at(13)));
	switch(ether_type)
	{
		case NET_PROTO::IEEE_802_1Q:
			if(data.length() < 17)
			{
				DFLTLOG(LEVEL_ERROR,
				        "cannot retrieve EtherType in Ethernet header\n");
				return NET_PROTO::ERROR;
			}
			ether_type = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(16) << 8) | data.at(17)));

			// TODO: we need the following part because we use two 802.1Q
			//       tags for kernel support
			if(ether_type != NET_PROTO::IEEE_802_1Q)
			{
				break;
			}
			// fall through
		case NET_PROTO::IEEE_802_1AD:
			if(data.length() < 21)
			{
				DFLTLOG(LEVEL_ERROR,
				        "cannot retrieve EtherType in Ethernet header\n");
				return NET_PROTO::ERROR;
			}
			ether_type = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(20) << 8) | data.at(21)));
			break;
		default:
			DFLTLOG(LEVEL_ERROR,
			        "unsupported packet type for Ethernet header\n");
			return NET_PROTO::ERROR;
	}

	return ether_type;
}

uint16_t Ethernet::getQTci(const Rt::Data &data)
{
	uint16_t tci = 0;
	NET_PROTO ether_type;
	if(data.length() < 17)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve vlan id in Ethernet header\n");
		return 0;
	}
	ether_type = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(12) << 8) | data.at(13)));
	switch(ether_type)
	{
		case NET_PROTO::IEEE_802_1Q:
			tci = ((data.at(14) & 0xff) << 8) | data.at(15);
			// TODO: we need the following part because we use two 802.1Q
			//       tags for kernel support
			ether_type = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(16) << 8) | data.at(17)));
			if(ether_type != NET_PROTO::IEEE_802_1Q)
			{
				break;
			}
			// fall through
		case NET_PROTO::IEEE_802_1AD:
			tci = ((data.at(18) & 0xff) << 8) | data.at(19);
			break;
		default:
			DFLTLOG(LEVEL_ERROR,
			        "cannot retrieve vlan id in non-Ethernet header\n");
			return 0;
	}

	return tci;
}

uint16_t Ethernet::getAdTci(const Rt::Data &data)
{
	NET_PROTO ether_type;
	NET_PROTO ether_type2;
	if(data.length() < 17)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot retrieve vlan id in Ethernet header\n");
		return 0;
	}
	ether_type = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(12) << 8) | data.at(13)));
	ether_type2 = to_enum<NET_PROTO>(static_cast<uint16_t>((data.at(16) << 8) | data.at(17)));
	// TODO: we need the following part because we use two 802.1Q tags for kernel support
	if(ether_type == NET_PROTO::IEEE_802_1Q && ether_type2 == NET_PROTO::IEEE_802_1Q)
	{
		ether_type = NET_PROTO::IEEE_802_1AD;
	}

	if(ether_type == NET_PROTO::IEEE_802_1AD)
	{
		return ((data.at(14) & 0xff) << 8) | data.at(15);
	}

	DFLTLOG(LEVEL_ERROR,
	        "cannot retrieve vlan id in non-Ethernet header\n");
	return 0;
}

MacAddress Ethernet::getDstMac(const Rt::Data &data)
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

MacAddress Ethernet::getSrcMac(const Rt::Data &data)
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

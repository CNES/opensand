/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file Rohc.cpp
 * @brief ROHC compression plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Rohc.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include <opensand_conf/uti_debug.h>
#include <opensand_conf/ConfigurationFile.h>
#include <vector>
#include <map>

#define MAX_CID "max_cid"
#define ROHC_SECTION "rohc"
#define CONF_ROHC_FILE "/etc/opensand/plugins/rohc.conf"

Rohc::Rohc():
	EncapPlugin()
{
	this->ether_type = NET_PROTO_ROHC;
	this->encap_name = "ROHC";

	this->upper[TRANSPARENT].push_back("IP");
	this->upper[REGENERATIVE].push_back("IP");
}


Rohc::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin)
{
	const char *FUNCNAME = "[Rohc::Context::Context]";
	int max_cid;
	ConfigurationFile config;

	if(config.loadConfig(CONF_ROHC_FILE) < 0)
	{   
		UTI_ERROR("%s failed to load config file '%s'",
		          FUNCNAME, CONF_ROHC_FILE);
		goto error;
	}   
	// Retrieving the QoS number
	if(!config.getValue(ROHC_SECTION, MAX_CID, max_cid)) 
	{   
		UTI_ERROR("%s missing %s parameter\n", FUNCNAME, MAX_CID);
		goto unload;
	}   
	UTI_DEBUG("%s Max CID: %d\n", FUNCNAME, max_cid);

	// init the CRC tables of the ROHC library
	crc_init_table(crc_table_3, crc_get_polynom(CRC_TYPE_3));
	crc_init_table(crc_table_7, crc_get_polynom(CRC_TYPE_7));
	crc_init_table(crc_table_8, crc_get_polynom(CRC_TYPE_8));

	// create the ROHC compressor
	this->comp = rohc_alloc_compressor(max_cid, 0, 0, 0);
	if(this->comp == NULL)
	{
		UTI_ERROR("%s cannot create ROHC compressor\n", FUNCNAME);
		goto unload;
	}

	// activate the compression profiles
	rohc_activate_profile(this->comp, ROHC_PROFILE_UNCOMPRESSED);
	rohc_activate_profile(this->comp, ROHC_PROFILE_IP);

	for(uint8_t tal_id = 0; tal_id <= BROADCAST_TAL_ID; ++tal_id)
	{
		this->decompressors[tal_id] = rohc_alloc_decompressor(this->comp);
		if(this->decompressors[tal_id] == NULL)
		{
			UTI_ERROR("%s cannot create ROHC decompressor\n", FUNCNAME);
			for(uint8_t i = 0; i < tal_id; ++i)
			{
				rohc_free_decompressor(this->decompressors[i]);
			}
			goto free_comp;
		}
	}


	config.unloadConfig();

	return;

free_comp:
	rohc_free_compressor(this->comp);
unload:
	config.unloadConfig();
error:
	this->comp = NULL;
}

Rohc::Context::~Context()
{
	// free ROHC compressor/decompressor if created
	if(this->comp != NULL)
		rohc_free_compressor(this->comp);

	for(uint8_t tal_id = 0; tal_id <= BROADCAST_TAL_ID; ++tal_id)
	{
		if(this->decompressors[tal_id] != NULL)
		{
			rohc_free_decompressor(this->decompressors[tal_id]);
		}
	}
}

NetBurst *Rohc::Context::encapsulate(NetBurst *burst,
                                     std::map<long, int> &UNUSED(time_contexts))
{
	const char *FUNCNAME = "[Rohc::Context::encapsulate]";
	NetBurst *rohc_packets = NULL;

	NetBurst::iterator packet;

	// create an empty burst of ROHC packets
	rohc_packets = new NetBurst();
	if(rohc_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of ROHC packets\n",
		          FUNCNAME);
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		if(!this->compressRohc(*packet, rohc_packets))
		{
			UTI_ERROR("%s ROHC encapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return rohc_packets;
}


NetBurst *Rohc::Context::deencapsulate(NetBurst *burst)
{
	const char *FUNCNAME = "[Rohc::Context::deencapsulate]";
	NetBurst *net_packets;

	NetBurst::iterator packet;

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of network packets\n",
		          FUNCNAME);
		delete burst;
		return false;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		uint8_t tal_id;

		// packet must be valid
		if(*packet == NULL)
		{
			UTI_ERROR("%s encapsulation packet is not valid, drop the packet\n",
			          FUNCNAME);
			continue;
		}

		// packet must be a ROHC packet
		if((*packet)->getType() != this->getEtherType())
		{
			UTI_ERROR("%s encapsulation packet is not a ROHC packet "
			          "(type = 0x%04x), drop the packet\n",
			          FUNCNAME, (*packet)->getType());
			continue;
		}

		tal_id = (*packet)->getSrcTalId();
		if(this->decompressors.find(tal_id) == this->decompressors.end())
		{
			UTI_ERROR("%s Could not find decompressor associated with SRC Tal Id %u\n",
			          FUNCNAME, tal_id);
			continue;
		}

		if(!this->decompressRohc(*packet, net_packets))
		{
			UTI_ERROR("%s ROHC desencapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return net_packets;
}

bool Rohc::Context::compressRohc(NetPacket *packet,
                                 NetBurst *rohc_packets)
{
	const char *FUNCNAME = "[Rohc::Context::compressRohc]";
	RohcPacket *rohc_packet;
	static unsigned char rohc_data[MAX_ROHC_SIZE];
	int rohc_len;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	// packet must be IPv4 or IPv6
	if(packet->getType() != NET_PROTO_IPV4 &&
	   packet->getType() != NET_PROTO_IPV6)
	{
		UTI_ERROR("%s packet is neither IPv4 nor IPv6, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	UTI_DEBUG("%s encapsulate a %d-byte packet of type 0x%04x\n",
	          FUNCNAME, packet->getTotalLength(), packet->getType());

	// the ROHC compressor must be ready
	if(this->comp == NULL)
	{
		UTI_ERROR("%s ROHC compressor not ready, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// compress the IP packet thanks to the ROHC library
	rohc_len = rohc_compress(this->comp,
	                         (unsigned char *) packet->getData().c_str(),
	                         packet->getTotalLength(),
	                         rohc_data, MAX_ROHC_SIZE);
	if(rohc_len <= 0)
	{
		UTI_ERROR("%s ROHC compression failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	// create a ROHC packet from data computed by the ROHC library
	rohc_packet =  new RohcPacket(rohc_data, rohc_len);
	if(rohc_packet == NULL)
	{
		UTI_ERROR("%s cannot create ROHC packet, "
		          "drop the network packet\n", FUNCNAME);
		goto drop;
	}
	rohc_packet->setSrcTalId(packet->getSrcTalId());
	rohc_packet->setDstTalId(packet->getDstTalId());
	rohc_packet->setQos(packet->getQos());

	// set the destination spot ID
	rohc_packet->setDstSpot(dest_spot);
	// add ROHC packet to burst
	rohc_packets->add(rohc_packet);

	UTI_DEBUG("%s %d-byte %s packet/frame => %d-byte ROHC packet\n",
	          FUNCNAME, packet->getTotalLength(), packet->getName().c_str(),
	          rohc_packet->getTotalLength());

	return true;

drop:
	return false;
}


bool Rohc::Context::decompressRohc(NetPacket *packet,
                                   NetBurst *net_packets)
{
	const char *FUNCNAME = "[Rohc::Context::decompressRohc]";
	NetPacket *net_packet;
	RohcPacket *rohc_packet;
	static unsigned char ip_data[MAX_ROHC_SIZE];
	int ip_len;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	packet->addTrace(HERE());

	rohc_packet = new RohcPacket(packet->getData());
	if(rohc_packet == NULL)
	{
		UTI_ERROR("%s cannot create RohcPacket from NetPacket\n", FUNCNAME);
		goto drop;
	}
	rohc_packet->addTrace(HERE());

	// decompress the IP packet thanks to the ROHC library
	ip_len = rohc_decompress(this->decompressors[packet->getSrcTalId()],
	                         (unsigned char *) rohc_packet->getData().c_str(),
	                         rohc_packet->getTotalLength(),
	                         ip_data, MAX_ROHC_SIZE);
	if(ip_len <= 0)
	{
		UTI_ERROR("%s ROHC decompression failed, "
		          "drop packet\n", FUNCNAME);
		goto drop;
	}

	net_packet = this->current_upper->build(ip_data, ip_len,
	                                        packet->getQos(),
	                                        packet->getSrcTalId(),
	                                        packet->getDstTalId());
	if(net_packet == NULL)
	{
		UTI_ERROR("%s cannot create IP packet, "
		          "drop the ROHC packet\n", FUNCNAME);
		goto drop;
	}

	// set the destination spot ID
	net_packet->setDstSpot(dest_spot);
	// add network packet to burst
	net_packets->add(net_packet);

	UTI_DEBUG("%s %d-byte ROHC packet => %d-byte %s packet/frame\n",
	          FUNCNAME, rohc_packet->getTotalLength(),
	          net_packet->getTotalLength(), net_packet->getName().c_str());

	delete rohc_packet;

	return net_packets;

drop:
	delete rohc_packet;
	return NULL;
}


NetPacket *Rohc::PacketHandler::build(unsigned char *data, size_t data_length,
                                      uint8_t qos,
                                      uint8_t src_tal_id, uint8_t dst_tal_id)
{
	size_t header_length = 0;
	return new NetPacket(data, data_length,
	                     this->getName(), this->getEtherType(),
	                     qos, src_tal_id, dst_tal_id, header_length);
}


Rohc::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
}

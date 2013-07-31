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
 * @file Rohc.cpp
 * @brief ROHC compression plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Rohc.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_QOS_DATA
#include <opensand_conf/uti_debug.h>
#include <opensand_conf/ConfigurationFile.h>
#include <NetPacket.h>
#include <vector>
#include <map>

#define MAX_CID "max_cid"
#define ROHC_SECTION "rohc"
#define CONF_ROHC_FILE "/etc/opensand/plugins/rohc.conf"

#define IS_ETHERNET(type) (type == NET_PROTO_802_1Q || \
                           type == NET_PROTO_802_1AD || \
                           type == NET_PROTO_ETH)

Rohc::Rohc():
	LanAdaptationPlugin(NET_PROTO_ROHC)
{
	this->upper[TRANSPARENT].push_back("IP");
	this->upper[TRANSPARENT].push_back("Ethernet");
	this->upper[TRANSPARENT].push_back("PHS");
	this->upper[REGENERATIVE].push_back("IP");
	this->upper[REGENERATIVE].push_back("Ethernet");
	this->upper[REGENERATIVE].push_back("PHS");
}


Rohc::Context::Context(LanAdaptationPlugin &plugin):
	LanAdaptationPlugin::LanAdaptationContext(plugin)
{
	int max_cid;
	ConfigurationFile config;

	if(config.loadConfig(CONF_ROHC_FILE) < 0)
	{
		UTI_ERROR("failed to load config file '%s'",
		          CONF_ROHC_FILE);
		goto error;
	}
	// Retrieving the QoS number
	if(!config.getValue(ROHC_SECTION, MAX_CID, max_cid))
	{
		UTI_ERROR("missing %s parameter\n", MAX_CID);
		goto unload;
	}
	UTI_DEBUG("Max CID: %d\n", max_cid);

	// init the CRC tables of the ROHC library
	crc_init_table(crc_table_3, crc_get_polynom(CRC_TYPE_3));
	crc_init_table(crc_table_7, crc_get_polynom(CRC_TYPE_7));
	crc_init_table(crc_table_8, crc_get_polynom(CRC_TYPE_8));

	// create the ROHC compressor
	this->comp = rohc_alloc_compressor(max_cid, 0, 0, 0);
	if(this->comp == NULL)
	{
		UTI_ERROR("cannot create ROHC compressor\n");
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
			UTI_ERROR("cannot create ROHC decompressor\n");
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
	NetBurst *rohc_packets = NULL;

	NetBurst::iterator packet;

	if(burst == NULL)
	{
		UTI_ERROR("input burst is NULL\n");
		return NULL;
	}
	// create an empty burst of ROHC packets
	rohc_packets = new NetBurst();
	if(rohc_packets == NULL)
	{
		UTI_ERROR("cannot allocate memory for burst of ROHC packets\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		NetPacket *comp_packet;
		bool is_eth = false;
		size_t head_length = 0;
		unsigned char head_buffer[MAX_ETHERNET_SIZE];
		NetPacket *payload;

		UTI_DEBUG("received a packet with type 0x%.4x\n", (*packet)->getType());
		// handle Ethernet packets
		if(IS_ETHERNET(this->current_upper->getEtherType()))
		{
			is_eth = true;

			if(!extractPacketFromEth(*packet, head_length,
			                         head_buffer, &payload))
			{
				continue;
			}
			
		}
		else
		{
			payload = *packet;
		}
		if(!this->compressRohc(payload, &comp_packet))
		{
			UTI_ERROR("ROHC compression failed, drop packet\n");
			continue;
		}
		if(is_eth)
		{
			// modify EtherType for ROHC
			//head_buffer[head_length -1] = (this->getEtherType() >> 8) & 0xff;
			//head_buffer[head_length] = this->getEtherType() & 0xff;

			// rebuild ethernet frame
			NetPacket *rohc_packet = comp_packet;
			buildEthFromPacket(comp_packet, head_length,
			                   (unsigned char *)head_buffer, &comp_packet);
			delete rohc_packet;
			if(comp_packet == NULL)
			{
				UTI_ERROR("cannot create ETHERNET frame");
				continue;
			}
		}
		rohc_packets->add(comp_packet);
	}

	// delete the burst and all packets in it
	delete burst;
	return rohc_packets;
}


NetBurst *Rohc::Context::deencapsulate(NetBurst *burst)
{
	NetBurst *net_packets;
    NetPacket *dec_packet;
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
		bool is_eth = false;
		size_t head_length = 0;
		unsigned char head_buffer[MAX_ETHERNET_SIZE];
		uint8_t tal_id;
		NetPacket *payload;

		// packet must be valid
		if(*packet == NULL)
		{
			UTI_ERROR("encapsulation packet is not valid, drop the packet\n");
			continue;
		}
		// handle Ethernet packets
		if(IS_ETHERNET(this->current_upper->getEtherType()))
		{
			is_eth = true;

			if(!extractPacketFromEth(*packet, head_length,
			                         head_buffer, &payload))
			{
				UTI_ERROR("cannot get IP packet from Ethernet frame\n");
				continue;
			}
		}
		else
		{
			payload = *packet;
		}
		// payload must be a ROHC packet
		if(payload->getType() != this->getEtherType())
		{
			UTI_ERROR("payload is not a ROHC packet "
			          "(type = 0x%04x), drop the packet\n",
			          payload->getType());
			continue;
		}

		tal_id = payload->getSrcTalId();
		if(this->decompressors.find(tal_id) == this->decompressors.end())
		{
			UTI_ERROR("Could not find decompressor associated with "
			          "SRC Tal Id %u\n", tal_id);
			continue;
		}

		if(!this->decompressRohc(payload, &dec_packet))
		{
			UTI_ERROR("ROHC decompression failed, drop packet\n");
			continue;
		}
		if(is_eth)
		{
			// rebuild ethernet frame
			NetPacket *rohc_packet = dec_packet;
			buildEthFromPacket(dec_packet, head_length,
			                   (unsigned char *)head_buffer, &dec_packet);
			delete rohc_packet;
			if(dec_packet == NULL)
			{
				UTI_ERROR("cannot create ETHERNET frame");
				continue;
			}
		}
		net_packets->add(dec_packet);
	}

	// delete the burst and all packets in it
	delete burst;
	return net_packets;
}

bool Rohc::Context::compressRohc(NetPacket *packet,
                                 NetPacket **comp_packet)
{
	RohcPacket *rohc_packet;
	static unsigned char rohc_data[MAX_ROHC_SIZE];
	int rohc_len;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	UTI_DEBUG("compress a %d-byte packet of type 0x%04x\n",
	          packet->getTotalLength(), packet->getType());

	// the ROHC compressor must be ready
	if(this->comp == NULL)
	{
		UTI_ERROR("ROHC compressor not ready, "
		          "drop packet\n");
		goto drop;
	}

	// compress the IP packet thanks to the ROHC library
	rohc_len = rohc_compress(this->comp,
	                         (unsigned char *) packet->getData().c_str(),
	                         packet->getTotalLength(),
	                         rohc_data, MAX_ROHC_SIZE);
	if(rohc_len <= 0)
	{
		UTI_ERROR("ROHC compression failed, "
		          "drop packet\n");
		goto drop;
	}

	// create a ROHC packet from data computed by the ROHC library
	rohc_packet =  new RohcPacket(rohc_data, rohc_len, packet->getType());
	if(rohc_packet == NULL)
	{
		UTI_ERROR("cannot create ROHC packet, "
		          "drop the network packet\n");
		goto drop;
	}
	rohc_packet->setSrcTalId(packet->getSrcTalId());
	rohc_packet->setDstTalId(packet->getDstTalId());
	rohc_packet->setQos(packet->getQos());

	// set the destination spot ID
	rohc_packet->setDstSpot(dest_spot);
	// set OUT parameter with compressed packet
	*comp_packet = rohc_packet;

	UTI_DEBUG("%d-byte %s packet/frame => %d-byte ROHC packet\n",
	          packet->getTotalLength(), packet->getName().c_str(),
	          rohc_packet->getTotalLength());

	return true;

drop:
	return false;
}


bool Rohc::Context::decompressRohc(NetPacket *packet,
                                   NetPacket **dec_packet)
{
	NetPacket *net_packet;
	RohcPacket *rohc_packet;
	static unsigned char ip_data[MAX_ROHC_SIZE];
	int ip_len;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	rohc_packet = new RohcPacket(packet->getData(), NET_PROTO_ROHC);
	if(rohc_packet == NULL)
	{
		UTI_ERROR("cannot create RohcPacket from NetPacket\n");
		goto drop;
	}

	// decompress the IP packet thanks to the ROHC library
	ip_len = rohc_decompress(this->decompressors[packet->getSrcTalId()],
	                         (unsigned char *) rohc_packet->getData().c_str(),
	                         rohc_packet->getTotalLength(),
	                         ip_data, MAX_ROHC_SIZE);
	if(ip_len <= 0)
	{
		UTI_ERROR("ROHC decompression failed, "
		          "drop packet\n");
		goto drop;
	}

	net_packet = this->current_upper->build(ip_data, ip_len,
	                                        packet->getQos(),
	                                        packet->getSrcTalId(),
	                                        packet->getDstTalId());
	if(net_packet == NULL)
	{
		UTI_ERROR("cannot create IP packet, "
		          "drop the ROHC packet\n");
		goto drop;
	}

	// set the destination spot ID
	net_packet->setDstSpot(dest_spot);
	// set OUT parameter with decompressed packet
	*dec_packet = net_packet;

	UTI_DEBUG("%d-byte ROHC packet => %d-byte %s packet/frame\n",
	          rohc_packet->getTotalLength(),
	          net_packet->getTotalLength(), net_packet->getName().c_str());

	delete rohc_packet;

	return true;

drop:
	delete rohc_packet;
	return false;
}

bool Rohc::Context::extractPacketFromEth(NetPacket *frame,
                                         size_t &head_length,
                                         unsigned char *head_buffer,
                                         NetPacket **payload)
{
	// build Ethernet frame to get the correct EtherType in
	// extractPacketFromEth function
	NetPacket *eth_frame =
		this->current_upper->build((unsigned char *)frame->getData().c_str(),
		                           frame->getTotalLength(),
		                           frame->getQos(),
		                           frame->getSrcTalId(),
		                           frame->getDstTalId());
	if(eth_frame == NULL)
	{
		UTI_ERROR("cannot create Ethernet packet");
		return false;
	}
	head_length = eth_frame->getHeaderLength();

	// keep the Ethernet header
	memcpy(head_buffer, (unsigned char *)eth_frame->getData().c_str(), head_length);
	// create a packet without ethernet header
	*payload = this->createPacket((unsigned char *)eth_frame->getPayload().c_str(),
	                              eth_frame->getPayloadLength(),
	                              frame->getQos(), frame->getSrcTalId(),
	                              frame->getDstTalId());
	delete eth_frame;
	return true;
}

bool Rohc::Context::buildEthFromPacket(NetPacket *packet,
                                       size_t head_length,
                                       unsigned char *head_buffer,
                                       NetPacket **eth_frame)
{
	size_t new_eth_size;

	new_eth_size = head_length + packet->getTotalLength();

	if(new_eth_size > MAX_ETHERNET_SIZE)
	{
		UTI_ERROR("ethernet frame length (%zu) exceeds maximum length (%d)\n",
		          new_eth_size, MAX_ETHERNET_SIZE);
		return false;
	}
	// ethernet frame << eth header << rohc packet data
	memcpy(head_buffer + head_length, packet->getData().c_str(),
	       packet->getTotalLength());
	*eth_frame = this->createPacket(head_buffer, new_eth_size,
	                                packet->getQos(),
	                                packet->getSrcTalId(),
	                                packet->getDstTalId());
	return true;
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

Rohc::PacketHandler::PacketHandler(LanAdaptationPlugin &plugin):
	LanAdaptationPlugin::LanAdaptationPacketHandler(plugin)
{
}

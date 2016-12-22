/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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

#include <NetPacket.h>
#include <opensand_conf/ConfigurationFile.h>
#include <opensand_output/Output.h>

#include <vector>
#include <map>
#include <ctime>

#define MAX_CID "max_cid"
#define ROHC_SECTION "rohc"
#define CONF_ROHC_FILE "/etc/opensand/plugins/rohc.conf"

#define IS_ETHERNET(type) (type == NET_PROTO_802_1Q || \
                           type == NET_PROTO_802_1AD || \
                           type == NET_PROTO_ETH)

/* Callbacks */
static int random_cb(const struct rohc_comp *const UNUSED(comp),
                      void *const UNUSED(user_context))
{
	return rand();
}

static void rohc_traces(void *const priv_ctx,
                        const rohc_trace_level_t level,
                        const rohc_trace_entity_t UNUSED(entity),
                        const int UNUSED(profile),
                        const char *const format,
                        ...)
{
	OutputLog *rohc_log = (OutputLog *)priv_ctx;
	log_level_t output_level = LEVEL_DEBUG;
	char buf[1024];
	va_list args;

	if(level == ROHC_TRACE_DEBUG)
	{
		output_level = LEVEL_DEBUG;
	}
	else if(level == ROHC_TRACE_INFO)
	{
		output_level = LEVEL_INFO;
	}
	else if(level == ROHC_TRACE_WARNING)
	{
		output_level = LEVEL_WARNING;
	}
	else if(level == ROHC_TRACE_ERROR)
	{
		output_level = LEVEL_ERROR;
	}
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	LOG(rohc_log, output_level, "%s", buf);
}

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
}

void Rohc::Context::init()
{
	LanAdaptationPlugin::LanAdaptationContext::init();
	int max_cid;
	int max_alloc = 0;
	rohc_cid_type_t cid_type = ROHC_SMALL_CID;
	bool status;
	ConfigurationFile config;
	map<string, ConfigurationList> config_section_map;

	if(config.loadConfig(CONF_ROHC_FILE) < 0)
	{
		LOG(this->log, LEVEL_ERROR,
		    "failed to load config file '%s'",
		    CONF_ROHC_FILE);
		goto error;
	}
	
	config.loadSectionMap(config_section_map);

	// Retrieving the QoS number
	if(!config.getValue(config_section_map[ROHC_SECTION], MAX_CID, max_cid))
	{
		LOG(this->log, LEVEL_ERROR,
		    "missing %s parameter\n", MAX_CID);
		goto unload;
	}
	LOG(this->log, LEVEL_INFO,
	    "Max CID: %d\n", max_cid);
	if(max_cid > ROHC_SMALL_CID_MAX)
	{
		cid_type = ROHC_LARGE_CID;
		max_cid = std::min(max_cid, ROHC_LARGE_CID_MAX);
	}

	// create the ROHC compressor
	this->comp = rohc_comp_new2(cid_type, max_cid, random_cb, NULL);
	if(this->comp == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create ROHC compressor\n");
		goto unload;
	}

	status = rohc_comp_set_traces_cb2(this->comp, rohc_traces, this->log);
	if(!status)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot enable traces\n");
		goto free_comp;
	}
	status = rohc_comp_enable_profiles(this->comp,
	                                   ROHC_PROFILE_UNCOMPRESSED,
	                                   ROHC_PROFILE_UDP,
	                                   ROHC_PROFILE_IP,
	                                   ROHC_PROFILE_UDPLITE,
	                                   ROHC_PROFILE_RTP,
	                                   ROHC_PROFILE_ESP,
	                                   ROHC_PROFILE_TCP, -1);
	if(!status)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot enable compression profiles\n");
		goto free_comp;
	}

	for(uint8_t tal_id = 0; tal_id <= BROADCAST_TAL_ID; ++tal_id)
	{
		this->decompressors[tal_id] = rohc_decomp_new2(cid_type, max_cid,
		                                              ROHC_O_MODE);
		if(this->decompressors[tal_id] == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot create ROHC decompressor\n");
			goto free_decomp;
		}
		max_alloc = tal_id;
		status = rohc_decomp_set_traces_cb2(this->decompressors[tal_id],
		                                    rohc_traces, this->log);
		if(!status)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot enable traces\n");
			goto free_decomp;
		}
		status = rohc_decomp_enable_profiles(this->decompressors[tal_id],
		                                     ROHC_PROFILE_UNCOMPRESSED,
		                                     ROHC_PROFILE_UDP,
		                                     ROHC_PROFILE_IP,
		                                     ROHC_PROFILE_UDPLITE,
		                                     ROHC_PROFILE_RTP,
		                                     ROHC_PROFILE_ESP,
		                                     ROHC_PROFILE_TCP, -1);
		if(!status)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot enable decompression profiles\n");
			goto free_decomp;
		}
	}

	config.unloadConfig();

	return;

free_decomp:
	for(uint8_t i = 0; i <= max_alloc; ++i)
	{
		rohc_decomp_free(this->decompressors[i]);
	}
free_comp:
	rohc_comp_free(this->comp);
unload:
	config.unloadConfig();
error:
	this->comp = NULL;
}

Rohc::Context::~Context()
{
	// free ROHC compressor/decompressor if created
	if(this->comp != NULL)
		rohc_comp_free(this->comp);

	for(uint8_t tal_id = 0; tal_id <= BROADCAST_TAL_ID; ++tal_id)
	{
		if(this->decompressors[tal_id] != NULL)
		{
			rohc_decomp_free(this->decompressors[tal_id]);
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
		LOG(this->log, LEVEL_ERROR,
		    "input burst is NULL\n");
		return NULL;
	}
	// create an empty burst of ROHC packets
	rohc_packets = new NetBurst();
	if(rohc_packets == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of ROHC packets\n");
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

		LOG(this->log, LEVEL_INFO,
		    "received a packet with type 0x%.4x\n", (*packet)->getType());
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
			continue;
		}
		if(is_eth)
		{
			delete payload;
			// modify EtherType for ROHC
			//head_buffer[head_length -1] = (this->getEtherType() >> 8) & 0xff;
			//head_buffer[head_length] = this->getEtherType() & 0xff;

			// rebuild ethernet frame
			NetPacket *rohc_packet = comp_packet;
			if(!buildEthFromPacket(comp_packet, head_length,
			                       (unsigned char *)head_buffer,
			                       &comp_packet))
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create Ethernet frame");
				delete rohc_packet;
				continue;
			}
			delete rohc_packet;
			if(comp_packet == NULL)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create ETHERNET frame");
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
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of network packets\n");
		delete burst;
		return NULL;
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
			LOG(this->log, LEVEL_ERROR,
			    "encapsulation packet is not valid, drop the packet\n");
			continue;
		}
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
		// payload must be a ROHC packet
		if(payload->getType() != this->getEtherType())
		{
			LOG(this->log, LEVEL_ERROR,
			    "payload is not a ROHC packet "
			    "(type = 0x%04x), drop the packet\n",
			    payload->getType());
			delete payload;
			continue;
		}

		tal_id = payload->getSrcTalId();
		if(this->decompressors.find(tal_id) == this->decompressors.end())
		{
			LOG(this->log, LEVEL_ERROR,
			    "Could not find decompressor associated with "
			    "SRC Tal Id %u\n", tal_id);
			continue;
		}

		if(!this->decompressRohc(payload, &dec_packet))
		{
			continue;
		}
		if(is_eth)
		{
			delete payload;
			// rebuild ethernet frame
			NetPacket *rohc_packet = dec_packet;
			if(!buildEthFromPacket(dec_packet, head_length,
			                       (unsigned char *)head_buffer,
			                       &dec_packet))
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create Ethernet frame");
				delete rohc_packet;
				continue;
			}
			delete rohc_packet;
			if(dec_packet == NULL)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create Ethernet frame");
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
	unsigned char rohc_data[MAX_ROHC_SIZE];
	Data packet_data = packet->getData();
	struct rohc_buf packet_buffer;
	struct rohc_buf rohc_buffer;
	// keep the destination spot
	uint16_t dest_spot = packet->getSpot();
	int ret;

	LOG(this->log, LEVEL_INFO,
	    "compress a %zu-byte packet of type 0x%04x\n",
	    packet->getTotalLength(), packet->getType());

	// the ROHC compressor must be ready
	if(this->comp == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "ROHC compressor not ready, drop packet\n");
		goto drop;
	}

	// packet_buffer
	packet_buffer.time.sec = 0;
	packet_buffer.time.nsec = 0;
	packet_buffer.data = (uint8_t *)packet_data.c_str();
	packet_buffer.max_len = packet->getTotalLength();
	packet_buffer.offset = 0;
	packet_buffer.len = packet->getTotalLength();
	// rohc_buffer
	rohc_buffer.time.sec = 0;
	rohc_buffer.time.nsec = 0;
	rohc_buffer.data = (uint8_t *)rohc_data;
	rohc_buffer.max_len = MAX_ROHC_SIZE;
	rohc_buffer.offset = 0;
	rohc_buffer.len = 0;

	// compress the IP packet thanks to the ROHC library
	ret = rohc_compress4(this->comp,
	                     packet_buffer, &rohc_buffer);
	if(ret == ROHC_STATUS_SEGMENT)
	{
		LOG(this->log, LEVEL_WARNING,
		    "Implement RoHC segment part !!\n");
	}
	else if(ret != ROHC_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "ROHC compression failed (%d), drop packet\n", ret);
		goto drop;
	}

	// create a ROHC packet from data computed by the ROHC library
	rohc_packet =  new RohcPacket(rohc_buffer.data, rohc_buffer.len, packet->getType());
	if(rohc_packet == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create ROHC packet, "
		    "drop the network packet\n");
		goto drop;
	}
	rohc_packet->setSrcTalId(packet->getSrcTalId());
	rohc_packet->setDstTalId(packet->getDstTalId());
	rohc_packet->setQos(packet->getQos());

	// set the destination spot ID
	rohc_packet->setSpot(dest_spot);
	// set OUT parameter with compressed packet
	*comp_packet = rohc_packet;

	LOG(this->log, LEVEL_INFO,
	    "%zu-byte %s packet/frame => %zu-byte ROHC packet\n",
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
	unsigned char ip_data[MAX_ROHC_SIZE];
	Data ip_packet;
	// keep the destination spot
	uint16_t dest_spot = packet->getSpot();
	int ret;
	struct rohc_buf packet_buffer;
	struct rohc_buf rohc_buffer;

	// packet_buffer
	packet_buffer.time.sec = 0;
	packet_buffer.time.nsec = 0;
	packet_buffer.data = (uint8_t *)ip_data;
	packet_buffer.max_len = MAX_ROHC_SIZE;
	packet_buffer.offset = 0;
	packet_buffer.len = 0;
	// rohc_buffer
	rohc_buffer.time.sec = 0;
	rohc_buffer.time.nsec = 0;
	rohc_buffer.max_len = packet->getTotalLength();
	rohc_buffer.offset = 0;
	rohc_buffer.len = packet->getTotalLength();
	rohc_buffer.data = (uint8_t *)packet->getData().c_str();

	// decompress the IP packet thanks to the ROHC library
	ret = rohc_decompress3(this->decompressors[packet->getSrcTalId()],
	                       rohc_buffer, &packet_buffer, NULL, NULL);
	if (ret != ROHC_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "ROHC decompression failed (%d), drop packet\n", ret);
		goto drop;
	}

	ip_packet.append(packet_buffer.data, packet_buffer.len);
	net_packet = this->current_upper->build(ip_packet, packet_buffer.len,
	                                        packet->getQos(),
	                                        packet->getSrcTalId(),
	                                        packet->getDstTalId());
	if(net_packet == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create IP packet, drop the ROHC packet\n");
		goto drop;
	}

	// set the destination spot ID
	net_packet->setSpot(dest_spot);
	// set OUT parameter with decompressed packet
	*dec_packet = net_packet;

	LOG(this->log, LEVEL_INFO,
	    "%zu-byte ROHC packet => %zu-byte %s packet/frame\n",
	    packet->getTotalLength(), net_packet->getTotalLength(),
	    net_packet->getName().c_str());

	return true;

drop:
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
		this->current_upper->build(frame->getData(),
		                           frame->getTotalLength(),
		                           frame->getQos(),
		                           frame->getSrcTalId(),
		                           frame->getDstTalId());
	if(eth_frame == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create Ethernet packet");
		return false;
	}
	head_length = eth_frame->getHeaderLength();

	// keep the Ethernet header
	memcpy(head_buffer, eth_frame->getData().c_str(), head_length);
	// create a packet without ethernet header
	*payload = this->createPacket(eth_frame->getPayload(),
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
	Data eth_packet;

	*eth_frame = NULL;
	new_eth_size = head_length + packet->getTotalLength();

/*	if(new_eth_size > MAX_ETHERNET_SIZE)
	{
		LOG(this->log, LEVEL_ERROR,
		    "ethernet frame length (%zu) exceeds maximum length (%d)\n",
		    new_eth_size, MAX_ETHERNET_SIZE);
		return false;
	}*/
	// ethernet frame << eth header << rohc packet data
	memcpy(head_buffer + head_length, packet->getData().c_str(),
	       packet->getTotalLength());
	eth_packet.append(head_buffer, new_eth_size);
	*eth_frame = this->createPacket(eth_packet, new_eth_size,
	                                packet->getQos(),
	                                packet->getSrcTalId(),
	                                packet->getDstTalId());
	return true;
}

NetPacket *Rohc::PacketHandler::build(const Data &data,
                                      size_t data_length,
                                      uint8_t qos,
                                      uint8_t src_tal_id,
                                      uint8_t dst_tal_id) const
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

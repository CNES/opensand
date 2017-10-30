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
 * @file Rlee.cpp
 * @brief RLE encapsulation plugin implementation
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "Rle.h"

#include <opensand_output/Output.h>
#include <opensand_conf/ConfigurationFile.h>
#include <NetPacket.h>
#include <vector>
#include <map>

#define PACKING_THRESHOLD "packing_threshold"
#define ALPDU_PROTECTION "alpdu_protection"
#define ALPDU_PROTECTION_CRC "crc"
#define ALPDU_PROTECTION_SEQ_NUM "sequence_number"
#define RLE_SECTION "rle"
#define CONF_RLE_FILE "/etc/opensand/plugins/rle.conf"

#define LABEL_SIZE 3 // bytes

Rle::Rle():
	EncapPlugin(NET_PROTO_RLE)
{
	this->upper[TRANSPARENT].push_back("ROHC");
	this->upper[TRANSPARENT].push_back("PHS");
	this->upper[TRANSPARENT].push_back("IP");
	this->upper[TRANSPARENT].push_back("Ethernet");
	
	this->upper[REGENERATIVE].push_back("ROHC");
	this->upper[REGENERATIVE].push_back("PHS");
	this->upper[REGENERATIVE].push_back("IP");
	this->upper[REGENERATIVE].push_back("Ethernet");
}

Rle::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin)
{
}

Rle::Context::~Context()
{
}

NetBurst *Rle::Context::encapsulate(NetBurst *burst,
                                    map<long, int> &UNUSED(time_encap_contexts))
{
	NetBurst *encap_burst;

	// Create a new burst
	encap_burst = new NetBurst();
	if(!encap_burst)
	{
		LOG(this->log, LEVEL_ERROR,
			"cannot allocate memory for burst of network "
			"packets\n");
		delete burst;
		return NULL;
	}

	// Encapsulate each packet of the burst
	for(NetBurst::iterator it = burst->begin();
		it != burst->end();
		++it)
	{
		NetPacket *packet;
		NetPacket *encap_packet;

		packet = *it;

		// Create a new packet (already encapsulated)
		encap_packet = new NetPacket(packet->getData(),
			packet->getTotalLength(),
			this->getName(),
			this->getEtherType(),
			packet->getQos(),
			packet->getSrcTalId(),
			packet->getDstTalId(),
			0);
		if(!encap_packet)
		{
			LOG(this->log, LEVEL_ERROR,
				"cannot create a burst of packets, "
				"drop the packet\n");
			continue;
		}
		
		// Add the current encapsulated packet to the encapsulated burst
		encap_burst->add(encap_packet);
	}
	delete burst;
	return encap_burst;
}

NetBurst *Rle::Context::deencapsulate(NetBurst *burst)
{
	NetBurst *decap_burst;

	// Create a new burst
	decap_burst = new NetBurst();
	if(!decap_burst)
	{
		LOG(this->log, LEVEL_ERROR,
			"cannot allocate memory for burst of network "
			"packets\n");
		delete burst;
		return NULL;
	}

	// Decapsulate each packet of the burst
	for(NetBurst::iterator it = burst->begin();
		it != burst->end();
		++it)
	{
		NetPacket *packet;
		NetPacket *decap_packet;

		// Get and check the current packet
		packet = *it;
		if(packet->getType() != this->getEtherType())
		{
			LOG(this->log, LEVEL_ERROR,
				"encapsulation packet is not a RLE packet "
				"(type = 0x%04x), drop the packet\n",
				packet->getType());
			continue;
		}

		// Create a new packet (already decapsulated)
		decap_packet = this->current_upper->build(packet->getData(),
			packet->getTotalLength(),
			packet->getQos(),
			packet->getSrcTalId(),
			packet->getDstTalId());
		if(!decap_packet)
		{
			LOG(this->log, LEVEL_ERROR,
				"cannot create a burst of packets, "
				"drop the packet\n");
			continue;
		}
		
		// Add the current decapsulated packet to the decapsulated burst
		decap_burst->add(decap_packet);
	}
	delete burst;
	return decap_burst;
}

NetBurst *Rle::Context::flush(int UNUSED(context_id))
{
	return NULL;
}

NetBurst *Rle::Context::flushAll()
{
	return NULL;
}

Rle::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
	this->rle_conf.allow_ptype_omission = 1;
	this->rle_conf.use_compressed_ptype = 1;
	this->rle_conf.allow_alpdu_crc = 1;
	this->rle_conf.allow_alpdu_sequence_number = 1;
	this->rle_conf.use_explicit_payload_header_map = 0;
	this->rle_conf.implicit_protocol_type = 0x30; // IPv4/IPv6
	this->rle_conf.implicit_ppdu_label_size = 0;
	this->rle_conf.implicit_payload_label_size = 0;
	this->rle_conf.type_0_alpdu_label_size = 0;
}

Rle::PacketHandler::~PacketHandler()
{
	map<RleIdentifier *, struct rle_transmitter *, ltRleIdentifier>::iterator trans_it;
	map<RleIdentifier *, struct rle_receiver *, ltRleIdentifier>::iterator recei_it;
	
	// Reset and clean encapsulation
	this->resetPacketToEncap();
	for(trans_it = this->transmitters.begin();
		trans_it != this->transmitters.end();
		++trans_it)
	{
		rle_transmitter_destroy(&(trans_it->second));
	}
	this->transmitters.clear();

	// Reset and clean decapsulation
	this->resetPacketToDecap();
	for(recei_it = this->receivers.begin();
		recei_it != this->receivers.end(); ++recei_it)
	{
		rle_receiver_destroy(&(recei_it->second));
	}
	this->receivers.clear();
}

bool Rle::PacketHandler::init(void)
{
	ConfigurationFile config;
	string protection;
	rle_alpdu_protection_t alpdu_protection;
	map<string, ConfigurationList> config_section_map;

	if(!EncapPlugin::EncapPacketHandler::init())
	{
		return false;
	}

	// Load configuration
	if(config.loadConfig(CONF_RLE_FILE) < 0)
	{
		LOG(this->log, LEVEL_ERROR,
		    "failed to load config file '%s'",
		    CONF_RLE_FILE);
		return false;
	}

	config.loadSectionMap(config_section_map);

	// Retrieving the ALPDU protection
	if(!config.getValue(config_section_map[RLE_SECTION],
	                    ALPDU_PROTECTION, protection))
	{
		LOG(this->log, LEVEL_ERROR,
		    "missing %s parameter\n", ALPDU_PROTECTION);
		goto unload;
	}
	if(protection == ALPDU_PROTECTION_CRC)
	{
		alpdu_protection = rle_alpdu_crc;
	}
	else if(protection == ALPDU_PROTECTION_SEQ_NUM)
	{
		alpdu_protection = rle_alpdu_sequence_number;
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "invalid value %s for %s parameter\n",
		    protection.c_str(),
		    ALPDU_PROTECTION);
		goto unload;
	}
	LOG(this->log, LEVEL_NOTICE,
	    "ALPDU protection: %s\n", protection.c_str());
	
	// Update RLE configuration
	switch(alpdu_protection)
	{
	case rle_alpdu_crc:
		this->rle_conf.allow_alpdu_sequence_number = 0;
		this->rle_conf.allow_alpdu_crc = 1;
		break;
	case rle_alpdu_sequence_number:
		this->rle_conf.allow_alpdu_sequence_number = 1;
		this->rle_conf.allow_alpdu_crc = 0;
		break;
	default:
		break;
	}

	// Unload configuration
	config.unloadConfig();
	
	return true;
unload:
	// Unload configuration
	config.unloadConfig();
	
	return false;
}

NetPacket *Rle::PacketHandler::build(const Data &data,
                                     size_t data_length,
                                     uint8_t qos,
                                     uint8_t src_tal_id,
                                     uint8_t dst_tal_id) const
{
	return new NetPacket(data, data_length,
	                     this->getName(), this->getEtherType(),
	                     qos, src_tal_id, dst_tal_id, 0);
}

size_t Rle::PacketHandler::getLength(const unsigned char *UNUSED(data)) const
{
	LOG(this->log, LEVEL_ERROR,
		"The %s getLength method will never be called\n",
		this->getName().c_str());
	assert(0);
	return 0;
}

bool Rle::PacketHandler::encapNextPacket(NetPacket *packet,
	size_t remaining_length,
	bool &partial_encap,
	NetPacket **encap_packet)
{
	uint8_t frag_id;
	uint8_t src_tal_id, dst_tal_id, qos;
	struct rle_transmitter *transmitter;
	map<RleIdentifier *, struct rle_transmitter *, ltRleIdentifier>::iterator it;
	RleIdentifier *identifier = NULL;
	uint8_t label[LABEL_SIZE];

	enum rle_frag_status frag_status;
	enum rle_pack_status pack_status;
	unsigned char *ppdu;
	size_t ppdu_size;

	bool first = true;
	Data fpdu;
	unsigned char *fpdu_buffer;
	
	// Set default returned values
	*encap_packet = NULL;
	partial_encap = false;

	// Get data which identify the transmitter
	src_tal_id = packet->getSrcTalId();
	if((src_tal_id & 0x1f) != src_tal_id)
	{
		LOG(this->log, LEVEL_ERROR,
			"The source terminal id %u of %s packet is too longer\n",
			src_tal_id, this->getName().c_str());
		goto error;
	}
	dst_tal_id = packet->getDstTalId();
	if((dst_tal_id & 0x1f) != dst_tal_id)
	{
		LOG(this->log, LEVEL_ERROR,
			"The destination terminal id %u of %s packet is too longer\n",
			dst_tal_id, this->getName().c_str());
		goto error;
	}
	qos = packet->getQos();
	if((qos & 0x07) != qos)
	{
		LOG(this->log, LEVEL_ERROR,
			"The QoS %u of %s packet is too longer\n",
			qos, this->getName().c_str());
		goto error;
	}

	// Get fragment id
	frag_id = qos;

	// Get transmitter
	identifier = new RleIdentifier(src_tal_id, dst_tal_id, qos);
	it = this->transmitters.find(identifier);
	if(it == this->transmitters.end())
	{
		uint16_t upper_ether_type;
		StackPlugin::StackPacketHandler *upper_pkt_hd;

		// Get upper ether type
		LOG(this->log, LEVEL_DEBUG, "Rle::PacketHandler::encapNextPacket -> getCurrentUpperPacketHandler");
		upper_pkt_hd = this->getCurrentUpperPacketHandler();
		if(upper_pkt_hd == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot get the upper packet handler\n");
			return false;
		}
		upper_ether_type = upper_pkt_hd->getEtherType();
		if(upper_ether_type == 0)
		{
			LOG(this->log, LEVEL_ERROR,
			    "invalid value of upper protocol type\n");
			return false;
		}
		this->rle_conf.implicit_protocol_type = upper_ether_type;

		// Create transmitter
		transmitter = rle_transmitter_new(&this->rle_conf);
		if(!transmitter)
		{
			LOG(this->log, LEVEL_ERROR,
				"cannot create a RLE transmitter\n");
			goto error;
		}

		// Store transmitter
		this->transmitters[identifier] = transmitter;

		// Build RLE SDU
		struct rle_sdu sdu;
		sdu.protocol_type = packet->getType();
		sdu.size = packet->getTotalLength();
		//sdu.buffer = packet->getData().c_str();
		
		sdu.buffer = (unsigned char *)malloc(sizeof(unsigned char) * sdu.size);
		memcpy(sdu.buffer, packet->getData().c_str(), sdu.size);

		// Encapsulate RLE SDU
		if(rle_encapsulate(transmitter, &sdu, frag_id) != 0)
		{
			LOG(this->log, LEVEL_ERROR,
				"RLE failed to encaspulate SDU\n");
			goto error;
		}

		// Check queue is empty to remove label size from remain
		remaining_length -= LABEL_SIZE;
	}
	else
	{
		// Get existing identifier and context
		delete identifier;
		identifier = it->first;
		transmitter = it->second;
		first = false;
	}

	// Prepare label to RLE
	if(Rle::getLabel(packet, label))
	{
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to get label\n");
		goto error_cleaning;
	}


	// Check there is data to fragment
	if(rle_transmitter_stats_get_queue_size(transmitter, frag_id) <= 0)
	{
		// Check data to fragment is this of the packet
		if(!first)
		{
			LOG(this->log, LEVEL_WARNING,
				"RLE is encapsulating another packet with fragment id %u\n",
				frag_id);
			return false;
		}
	}

	// Fragment RLE SDU to RLE PPDU
	frag_status = rle_fragment(transmitter, frag_id, remaining_length, &ppdu, &ppdu_size);
	if(frag_status == RLE_FRAG_ERR_BURST_TOO_SMALL)
	{
		// Set partial encapsulation status
		partial_encap = true;
		return true;
	}
	else if(frag_status != 0)
	{
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to fragment ALPDU\n");
		goto error_cleaning;
	}
	
	// Pack RLE PPDU to FPDU
	fpdu.reserve(ppdu_size);
	fpdu_buffer = (unsigned char *)fpdu.c_str();
	pack_status = rle_pack(ppdu, ppdu_size, label, LABEL_SIZE, fpdu_buffer, 0, &ppdu_size);
	if(pack_status != 0)
	{
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to pack PPDU\n");
		goto error_cleaning;
	}
	*encap_packet = new NetPacket(fpdu, ppdu_size,
		this->getName(), this->getEtherType(),
		qos, src_tal_id, dst_tal_id, 0);

	// Check remaining RLE PPDU
	ppdu_size = rle_transmitter_stats_get_queue_size(transmitter, frag_id);
	if(0 < ppdu_size)
	{
		// Set partial encapsulation status
		partial_encap = true;
	}
	else
	{
		delete identifier;
	}
	
	return true;

error_cleaning:
	// Clean RLE queue
	if(first)
	{
		rle_transmitter_stats_reset_counters(transmitter, frag_id);
	}
	delete identifier;
error:
	return false;
}

bool Rle::PacketHandler::resetPacketToEncap(NetPacket *packet)
{
	if(!packet)
	{
		return this->resetAllPacketToEncap();
	}
	return this->resetOnePacketToEncap(packet);
}

bool Rle::PacketHandler::resetOnePacketToEncap(NetPacket *packet)
{
	uint8_t frag_id;
	struct rle_transmitter *transmitter;
	map<RleIdentifier *, struct rle_transmitter *, ltRleIdentifier>::iterator transmitter_it;
	RleIdentifier *identifier = NULL;

	// Get transmitter
	identifier = new RleIdentifier(packet->getSrcTalId(),
		packet->getDstTalId(),
		packet->getQos());
	transmitter_it = this->transmitters.find(identifier);
	if(transmitter_it == this->transmitters.end())
	{
		return true;
	}
	transmitter = transmitter_it->second;

	// Get fragment id
	frag_id = packet->getQos();

	// Check there is data to fragment
	if(rle_transmitter_stats_get_queue_size(transmitter, frag_id) <= 0)
	{
		return true;
	}
	
	// Reset transmitter
	rle_transmitter_stats_reset_counters(transmitter, frag_id);

	return true;
}

bool Rle::PacketHandler::resetAllPacketToEncap()
{
	uint8_t frag_id;
	struct rle_transmitter *transmitter;
	map<RleIdentifier *, struct rle_transmitter *, ltRleIdentifier>::iterator transmitter_it;

	// Reset all fragment id of all transmitter
	for(transmitter_it = this->transmitters.begin();
		transmitter_it != this->transmitters.end();
		++transmitter_it)
	{
		transmitter = transmitter_it->second;
		
		// Reset all fragment id of the current transmitter
		for(frag_id = 0; frag_id <= RLE_MAX_FRAG_ID; ++frag_id)
		{
			rle_transmitter_stats_reset_counters(transmitter, frag_id);	
		}
	}

	return true;
}

bool Rle::PacketHandler::decapNextPacket(NetContainer *packet,
	bool &partial_decap,
	vector<NetPacket *> &decap_packets,
	unsigned int UNUSED(decap_packets_count))
{
	uint8_t frag_id;
	uint8_t src_tal_id, dst_tal_id, qos;
	uint8_t label[LABEL_SIZE];
	RleIdentifier *identifier = NULL;
	map<RleIdentifier *, struct rle_receiver *, ltRleIdentifier>::iterator it;

	struct rle_receiver *receiver;
	struct rle_sdu *sdus = NULL;
	size_t sdus_count;
	size_t sdus_max_count;
	enum rle_decap_status status;

	// Set default returned values
	decap_packets = vector<NetPacket *>();
	partial_decap = false;

	// Get data which identify the receiver
	if(!Rle::getLabel(packet->getData(), label))
	{
		LOG(this->log, LEVEL_ERROR,
			"Unable to get label from %s packet\n",
			this->getName().c_str());
		goto error;
	}
	src_tal_id = label[0];
	dst_tal_id = label[1];
	qos = label[2];

	// Get fragment id
	frag_id = qos;

	// Get receiver
	identifier = new RleIdentifier(src_tal_id, dst_tal_id, qos);
	it = this->receivers.find(identifier);
	if(it == this->receivers.end())
	{
		uint16_t upper_ether_type;
		StackPlugin::StackPacketHandler *upper_pkt_hd;

		// Get upper ether type
		LOG(this->log, LEVEL_DEBUG, "Rle::PacketHandler::decapNextPacket -> getCurrentUpperPacketHandler");
		upper_pkt_hd = this->getCurrentUpperPacketHandler();
		if(upper_pkt_hd == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot get the upper packet handler\n");
			return false;
		}
		upper_ether_type = upper_pkt_hd->getEtherType();
		if(upper_ether_type == 0)
		{
			LOG(this->log, LEVEL_ERROR,
			    "invalid value of upper protocol type\n");
			return false;
		}
		this->rle_conf.implicit_protocol_type = upper_ether_type;

		// Create receiver
		receiver = rle_receiver_new(&this->rle_conf);
		if(!receiver)
		{
			LOG(this->log, LEVEL_ERROR,
				"cannot create a RLE receiver\n");
			goto error;
		}

		// Store receiver
		this->receivers[identifier] = receiver;
	}
	else
	{
		// Get existing identifier and context
		delete identifier;
		identifier = it->first;
		receiver = it->second;
	}

	// Prepare SDUs structures
	sdus_max_count = packet->getPayloadLength() / LABEL_SIZE;
	sdus = new struct rle_sdu[sdus_max_count];

	// Decapsulate RLE FPDU
	status = rle_decapsulate(receiver, (unsigned char *)packet->getPayload().c_str(),
		 packet->getPayloadLength(), sdus, sdus_max_count, &sdus_count, label, sizeof(label));
	if(status != RLE_DECAP_OK)
	{
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to encaspulate SDU\n");
		goto error;
	}

	// Add all SDUs to decapsulated packets list
	for(unsigned int i = 0; i< sdus_count; ++i)
	{
		// Create packet from SDU
		struct rle_sdu sdu = sdus[i];
		NetPacket *decap_packet = new NetPacket(Data(sdu.buffer), sdu.size,
			this->getName(), sdu.protocol_type,
			qos, src_tal_id, dst_tal_id, 0);
		
		// Add SDU to decapsulated packets list
		decap_packets.push_back(decap_packet);
		//TODO: destroy SDU buffer ???
	}
	delete sdus;

	// Get fragment id
	frag_id = qos;
	
	// Set returned value
	if(0 < rle_receiver_stats_get_queue_size(receiver, frag_id))
	{
		partial_decap = true;
	}

	return true;
	
error:
	// TODO: destroy SDU buffer ???
	if(sdus)
	{
		delete sdus;
	}
	return false;
}

bool Rle::PacketHandler::resetPacketToDecap()
{
	return true;
}

bool Rle::PacketHandler::getChunk(NetPacket *UNUSED(packet), size_t UNUSED(remaining_length),
                                  NetPacket **UNUSED(data), NetPacket **UNUSED(remaining_data)) const
{
	LOG(this->log, LEVEL_ERROR,
		"The %s getChunk method will never be called\n",
		this->getName().c_str());
	assert(0);
	return false;
}

bool Rle::PacketHandler::getSrc(const Data &data, tal_id_t &tal_id) const
{
	uint8_t label[LABEL_SIZE];

	// Get label (this will success if it is the first fragment)
	if(!Rle::getLabel(data, label))
	{
		return false;
	}

	tal_id = label[0];
	return true;
}

bool Rle::PacketHandler::getQos(const Data &data, qos_t &qos) const
{
	uint8_t label[LABEL_SIZE];

	// Get label (this will success if it is the first fragment)
	if(!Rle::getLabel(data, label))
	{
		return false;
	}

	qos = label[2];
	return true;
}

// Static methods

bool Rle::getLabel(NetPacket *packet, uint8_t label[])
{
	tal_id_t src_tal_id = packet->getSrcTalId();
	tal_id_t dst_tal_id = packet->getDstTalId();
	qos_t qos = packet->getQos();

	if((src_tal_id & 0x1F) != src_tal_id
	   || (dst_tal_id & 0x1F) != dst_tal_id
	   || (qos & 0x07) != qos)
	{
		return false;
	}
	label[0] = src_tal_id & 0x1F;
	label[1] = dst_tal_id & 0x1F;
	label[2] = qos & 0x07;
	return true;
}

bool Rle::getLabel(const Data &data, uint8_t label[])
{
	uint8_t src_tal_id = (uint8_t)(data.at(0));
	uint8_t dst_tal_id = (uint8_t)(data.at(1));
	uint8_t qos = (uint8_t)(data.at(2));

	if((src_tal_id & 0x1F) != src_tal_id
	   || (dst_tal_id & 0x1F) != dst_tal_id
	   || (qos & 0x07) != qos)
	{
		return false;
	}
	label[0] = src_tal_id & 0x1F;
	label[1] = dst_tal_id & 0x1F;
	label[2] = qos & 0x07;
	return true;
}

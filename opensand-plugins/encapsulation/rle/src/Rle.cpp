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
#include <algorithm>

#define PACKING_THRESHOLD "packing_threshold"
#define ALPDU_PROTECTION "alpdu_protection"
#define ALPDU_PROTECTION_CRC "crc"
#define ALPDU_PROTECTION_SEQ_NUM "sequence_number"
#define RLE_SECTION "rle"
#define CONF_RLE_FILE "/etc/opensand/plugins/rle.conf"

#define LABEL_SIZE 3 // bytes

void Rle::rle_traces(const int module_id,
		const int level,
		const char *const file,
		const int line,
		const char *const func,
		const char *const message,
		...)
{
	int ret = 0;
	log_level_t output_level = LEVEL_DEBUG;
	char buf[4096];
	va_list args;
		
	if(level == RLE_LOG_LEVEL_DEBUG)
	{
		output_level = LEVEL_DEBUG;
	}
	else if(level == RLE_LOG_LEVEL_INFO)
	{
		output_level = LEVEL_INFO;
	}
	else if(level == RLE_LOG_LEVEL_WARNING)
	{
		output_level = LEVEL_WARNING;
	}
	else if(level == RLE_LOG_LEVEL_ERROR)
	{
		output_level = LEVEL_ERROR;
	}
	else if(level == RLE_LOG_LEVEL_CRI)
	{
		output_level = LEVEL_CRITICAL;
	}

	va_start(args, message);
	ret = vsnprintf(buf, sizeof(buf), message, args);
	va_end(args);
	if(0 < ret && ret < (int)sizeof(buf))
	{
		DFLTLOG(output_level, "[%s:%d][%s][Module %d] %s", 
			file, line, func, module_id, buf);
	}
}

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

	rle_set_trace_callback(&(Rle::rle_traces));
}

Rle::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin)
{
	this->rle_conf.allow_ptype_omission = 0;//1;
	this->rle_conf.use_compressed_ptype = 1;
	this->rle_conf.allow_alpdu_crc = 1;
	this->rle_conf.allow_alpdu_sequence_number = 1;
	this->rle_conf.use_explicit_payload_header_map = 0;
	this->rle_conf.implicit_protocol_type = 0x30; // IPv4/IPv6
	this->rle_conf.implicit_ppdu_label_size = 0;
	this->rle_conf.implicit_payload_label_size = 0;
	this->rle_conf.type_0_alpdu_label_size = 0;
}

Rle::Context::~Context()
{
	map<RleIdentifier *, struct rle_receiver *, ltRleIdentifier>::iterator recei_it;

	// Clean decapsulation
	for(recei_it = this->receivers.begin();
		recei_it != this->receivers.end(); ++recei_it)
	{
		delete recei_it->first;
		rle_receiver_destroy(&(recei_it->second));
	}
	this->receivers.clear();

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
		vector<NetPacket *> decap_packets;
		vector<NetPacket *>::iterator pkt_it;

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

		// Deencapsulate RLE packets
		if(!this->decapNextPacket(packet, decap_burst))
		{
			LOG(this->log, LEVEL_ERROR,
				"cannot decapsulate a RLE packet, "
				"drop the packet\n");
			continue;
		}
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

bool Rle::Context::decapNextPacket(NetPacket *packet, NetBurst *burst)
{
	uint8_t src_tal_id, dst_tal_id, qos;
	uint8_t label[LABEL_SIZE];
	RleIdentifier *identifier = NULL;
	map<RleIdentifier *, struct rle_receiver *, ltRleIdentifier>::iterator it;

	struct rle_receiver *receiver;
	struct rle_sdu *sdus = NULL;
	size_t sdus_count = 0;
	size_t sdus_max_count = 0;
	enum rle_decap_status status;

	// Get data which identify the receiver
	if(packet->getPayloadLength() <= LABEL_SIZE)
	{
		LOG(this->log, LEVEL_ERROR,
			"Not enough payload in %s packet\n",
			this->getName().c_str());
		goto error;
	}
	if(!Rle::getLabel(packet->getPayload(), label))
	{
		LOG(this->log, LEVEL_ERROR,
			"Unable to get label from %s packet\n",
			this->getName().c_str());
		goto error;
	}
	src_tal_id = label[0];
	dst_tal_id = label[1];
	qos = label[2];

	// Get receiver
	identifier = new RleIdentifier(src_tal_id, dst_tal_id, qos);
	it = this->receivers.find(identifier);
	if(it == this->receivers.end())
	{
		// Create receiver
		receiver = rle_receiver_new(&this->rle_conf);
		if(!receiver)
		{
			delete identifier;
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
	sdus_count = 0;
	sdus = new struct rle_sdu[sdus_max_count];
	for(unsigned int i = 0; i < sdus_max_count; ++i)
	{
		sdus[i].buffer = new unsigned char[packet->getPayloadLength()];
	}

	// Decapsulate RLE FPDU
	//LOG(this->log, LEVEL_DEBUG,
	//		"DEVEL> before decap: payload_length=%u bytes, sdus_max_count=%u, sdu_count=%u, label_size=%u bytes",
	//		packet->getPayloadLength(), sdus_max_count, sdus_count, LABEL_SIZE);
	status = rle_decapsulate(receiver, (unsigned char *)packet->getPayload().c_str(),
		 packet->getPayloadLength(), sdus, sdus_max_count, &sdus_count, label, LABEL_SIZE);
	//LOG(this->log, LEVEL_DEBUG,
	//		"DEVEL> after decap: payload_length=%u bytes, sdus_max_count=%u, sdu_count=%u, label_size=%u bytes",
	//		packet->getPayloadLength(), sdus_max_count, sdus_count, LABEL_SIZE);
	if(status != RLE_DECAP_OK)
	{
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to decaspulate SDU\n");
		goto error;
	}

	// Add all SDUs to decapsulated packets list
	for(unsigned int i = 0; i< sdus_count; ++i)
	{
		// Create packet from SDU
		struct rle_sdu sdu = sdus[i];
		NetPacket *decap_packet;

		//LOG(this->log, LEVEL_DEBUG,
		//		"DEVEL> SDU size=%u bytes",
		//		sdu.size);
		decap_packet = this->current_upper->build(Data(sdu.buffer, sdu.size), sdu.size,
			qos, src_tal_id, dst_tal_id);
		if(!decap_packet)
		{
			LOG(this->log, LEVEL_ERROR,
				"RLE failed to create decapsulated packet\n");
			goto error;
		}

		
		// Add SDU to decapsulated packets list
		burst->add(decap_packet);
		//LOG(this->log, LEVEL_DEBUG, "DEVEL> deleting sdus[%u]", i);
		delete[] sdus[i].buffer;
		//LOG(this->log, LEVEL_DEBUG, "DEVEL> sdus[%u] deleted", i);
		sdus[i].buffer = NULL;
		sdus[i].size = 0;
	}
	//LOG(this->log, LEVEL_DEBUG, "DEVEL> deleting sdus");
	delete[] sdus;
	//LOG(this->log, LEVEL_DEBUG, "DEVEL> sdus deleted");

	return true;
	
error:
	if(sdus)
	{
		for(unsigned int i = 0; i< sdus_count; ++i)
		{
			if(sdus[i].buffer != NULL)
			{
				//LOG(this->log, LEVEL_DEBUG, "DEVEL> error: deleting sdus[%u]", i);
				delete[] sdus[i].buffer;
				//LOG(this->log, LEVEL_DEBUG, "DEVEL> error: sdus[%u] deleted", i);
			}
		}
		delete[] sdus;
	}
	return false;
}

Rle::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
	this->rle_conf.allow_ptype_omission = 0;//1;
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
	map<RleIdentifier *, rle_trans_ctxt_t, ltRleIdentifier>::iterator trans_it;
	
	// Reset and clean encapsulation
	for(trans_it = this->transmitters.begin();
		trans_it != this->transmitters.end();
		++trans_it)
	{
		delete trans_it->first;
		rle_transmitter_destroy(&(trans_it->second.first));
		trans_it->second.second.clear();
	}
	this->transmitters.clear();
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
	// Check payload length
	if(data_length < LABEL_SIZE)
	{
		LOG(this->log, LEVEL_ERROR, "Payload length (%zu bytes) is lower than RLE label length (%u bytes)",
			data_length, LABEL_SIZE);
		return NULL;
	}

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
	bool new_burst,
	bool &partial_encap,
	NetPacket **encap_packet)
{
	uint8_t frag_id;
	uint8_t src_tal_id, dst_tal_id, qos;
	struct rle_transmitter *transmitter;
	vector<NetPacket *>::iterator pkt_it;
	map<RleIdentifier *, rle_trans_ctxt_t, ltRleIdentifier>::iterator it;
	RleIdentifier *identifier = NULL;
	uint8_t label[LABEL_SIZE];

	enum rle_frag_status frag_status;
	enum rle_pack_status pack_status;
	struct rle_sdu sdu;
	unsigned char *ppdu = NULL;
	size_t ppdu_size;
	size_t fpdu_size;
	size_t fpdu_cur_pos;

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
		return false;
	}
	dst_tal_id = packet->getDstTalId();
	if((dst_tal_id & 0x1f) != dst_tal_id)
	{
		LOG(this->log, LEVEL_ERROR,
			"The destination terminal id %u of %s packet is too longer\n",
			dst_tal_id, this->getName().c_str());
		return false;
	}
	qos = packet->getQos();
	if((qos & 0x07) != qos)
	{
		LOG(this->log, LEVEL_ERROR,
			"The QoS %u of %s packet is too longer\n",
			qos, this->getName().c_str());
		return false;
	}

	// Prepare label to RLE
	if(!Rle::getLabel(packet, label))
	{
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to get label\n");
		return false;
	}

	// Get fragment id
	frag_id = qos;

	// Get transmitter
	identifier = new RleIdentifier(src_tal_id, dst_tal_id, qos);
	it = this->transmitters.find(identifier);
	if(it == this->transmitters.end())
	{
		// Create transmitter
		transmitter = rle_transmitter_new(&this->rle_conf);
		if(!transmitter)
		{
			delete identifier;
			LOG(this->log, LEVEL_ERROR,
				"cannot create a RLE transmitter\n");
			return false;
		}

		// Store transmitter
		this->transmitters[identifier] = pair<struct rle_transmitter *, vector<NetPacket *> >(transmitter, vector<NetPacket *>());
		it = this->transmitters.find(identifier);
		if(it == this->transmitters.end())
		{
			delete identifier;
			rle_transmitter_destroy(&transmitter);
			LOG(this->log, LEVEL_ERROR,
				"cannot store the RLE transmitter\n");
			return false;
		}
	}
	else
	{
		// Get existing identifier and context
		delete identifier;
		identifier = it->first;
		transmitter = it->second.first;
	}

	// Check packet has already been partially sent
	vector<NetPacket *> &sent_packets = it->second.second;
	pkt_it = std::find(sent_packets.begin(), sent_packets.end(), packet);

	if(pkt_it == sent_packets.end())
	{
		// Build RLE SDU
		sdu.protocol_type = packet->getType();
		sdu.size = packet->getTotalLength();
		sdu.buffer = new unsigned char[sdu.size];
		memcpy(sdu.buffer, packet->getData().c_str(), sdu.size);

		// Encapsulate RLE SDU
		if(rle_encapsulate(transmitter, &sdu, frag_id) != 0)
		{
			delete[] sdu.buffer;
			LOG(this->log, LEVEL_ERROR,
				"RLE failed to encaspulate SDU\n");
			return false;
		}
		delete[] sdu.buffer;
		sdu.size = 0;
		//LOG(this->log, LEVEL_DEBUG, "DEVEL> Initial remaining_length=%zu bytes", remaining_length);
	}
	else
	{
		LOG(this->log, LEVEL_DEBUG,
			"RLE encapsulation of partial sent packet)\n");
	}
	if(new_burst)
	{
		remaining_length -= LABEL_SIZE;
	}

	// Fragment RLE SDU to RLE PPDU
	ppdu_size = 0;
	//LOG(this->log, LEVEL_DEBUG,
	//		"DEVEL> before fragmentation: remaining_length=%u bytes, label_size=%u bytes, ppdu_size=%u bytes",
	//		new_burst ? remaining_length + LABEL_SIZE : remaining_length,
	//		new_burst ? LABEL_SIZE : 0,
	//		ppdu_size);
	frag_status = rle_fragment(transmitter, frag_id, remaining_length, &ppdu, &ppdu_size);
	//LOG(this->log, LEVEL_DEBUG,
	//		"DEVEL> after fragmentation: remaining_length=%u bytes, label_size=%u bytes, ppdu_size=%u bytes",
	//		new_burst ? remaining_length + LABEL_SIZE : remaining_length,
	//		new_burst ? LABEL_SIZE : 0,
	//		ppdu_size);
	if(frag_status != 0)
	{
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to fragment ALPDU (code=%d)\n",
			(int)frag_status);
		return false;
	}

	// Prepare FPDU
	fpdu_size = ppdu_size;
	if(new_burst)
	{
		fpdu_size += LABEL_SIZE;
	}
	fpdu_buffer = new unsigned char[fpdu_size];
	fpdu_cur_pos = 0;

	if(new_burst)
	{
		// Initialize pack adding the label to FPDU
		LOG(this->log, LEVEL_DEBUG, "Add label to RLE FPDU");
		//LOG(this->log, LEVEL_DEBUG,
		//		"DEVEL> before pack init: label_size=%u bytes, fpdu_cur_pos=%u, fpdu_size=%u bytes",
		//		LABEL_SIZE, fpdu_cur_pos, fpdu_size);
		pack_status = rle_pack_init(label, LABEL_SIZE, fpdu_buffer, &fpdu_cur_pos, &fpdu_size);
		//LOG(this->log, LEVEL_DEBUG,
		//		"DEVEL> after pack init: label_size=%u bytes, fpdu_cur_pos=%u, fpdu_size=%u bytes",
		//		LABEL_SIZE, fpdu_cur_pos, fpdu_size);
		if(pack_status != 0)
		{
			delete[] fpdu_buffer;
			LOG(this->log, LEVEL_ERROR,
				"RLE failed to pack PPDU (code=%d)\n",
				(int)pack_status);
			return false;
		}
	}

	// Pack RLE PPD to RLE FPDU
	//LOG(this->log, LEVEL_DEBUG,
	//		"DEVEL> before packing: ppdu_size=%u bytes, label_size=%u bytes, fpdu_cur_pos=%u, fpdu_size=%u bytes",
	//		ppdu_size, LABEL_SIZE, fpdu_cur_pos, fpdu_size);
	pack_status = rle_pack(ppdu, ppdu_size, label, LABEL_SIZE, fpdu_buffer, &fpdu_cur_pos, &fpdu_size);
	//LOG(this->log, LEVEL_DEBUG,
	//		"DEVEL> after packing: ppdu_size=%u bytes, label_size=%u bytes, fpdu_cur_pos=%u, fpdu_size=%u bytes",
	//		ppdu_size, LABEL_SIZE, fpdu_cur_pos, fpdu_size);
	if(pack_status == RLE_PACK_ERR_FPDU_TOO_SMALL)
	{
		// Set partial encapsulation status
		LOG(this->log, LEVEL_DEBUG,
			"RLE partial packing)\n");
		partial_encap = true;
	}
	else if(pack_status != 0)
	{
		delete[] fpdu_buffer;
		LOG(this->log, LEVEL_ERROR,
			"RLE failed to pack PPDU (code=%d)\n",
			(int)pack_status);
		return false;
	}
	fpdu.assign(fpdu_buffer, fpdu_cur_pos);
	delete[] fpdu_buffer;
	*encap_packet = new NetPacket(fpdu, fpdu_cur_pos,
		this->getName(), this->getEtherType(),
		qos, src_tal_id, dst_tal_id, 0);

	if(pkt_it == sent_packets.end() && partial_encap)
	{
		// Add packet to partial sent packets list
		LOG(this->log, LEVEL_DEBUG, "Add packet to partial sent packets list");
		sent_packets.push_back(packet);
	}
	else if(pkt_it != sent_packets.end() && !partial_encap)
	{
		// Remove packet from partial sent packets list
		LOG(this->log, LEVEL_DEBUG, "Remove packet from partial sent packets list");
		sent_packets.erase(pkt_it);
	}
	return true;
}

bool Rle::PacketHandler::getEncapsulatedPackets(NetContainer *packet,
	bool &partial_decap,
	vector<NetPacket *> &decap_packets,
	unsigned int UNUSED(decap_packet_count))
{
	NetPacket *decap_packet;

	// Set default
	partial_decap = false;

	// Build packet
	decap_packet = this->build(packet->getPayload(), packet->getPayloadLength(),
		0x00, BROADCAST_TAL_ID, BROADCAST_TAL_ID);
	if(!decap_packet)
	{
		LOG(this->log, LEVEL_ERROR,
			"cannot create one %s packet (length = %zu bytes, payload length = %zu bytes)\n",
			this->getName().c_str(), packet->getTotalLength(), packet->getPayloadLength());
		return false;
	}
	
	// Add the packet to the list
	decap_packets.push_back(decap_packet);
	
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

	//DFLTLOG(LEVEL_ERROR, "Src_tal_id = %u (& 0x1F = %u)",
	//	src_tal_id, src_tal_id & 0x1F);
	//DFLTLOG(LEVEL_ERROR, "Dst_tal_id = %u (& 0x1F = %u)",
	//	dst_tal_id, dst_tal_id & 0x1F);
	//DFLTLOG(LEVEL_ERROR, "Qos = %u (& 0x07 = %u)",
	//	qos, qos & 0x07);
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

	//DFLTLOG(LEVEL_ERROR, "Src_tal_id = %u (& 0x1F = %u)",
	//	src_tal_id, src_tal_id & 0x1F);
	//DFLTLOG(LEVEL_ERROR, "Dst_tal_id = %u (& 0x1F = %u)",
	//	dst_tal_id, dst_tal_id & 0x1F);
	//DFLTLOG(LEVEL_ERROR, "Qos = %u (& 0x07 = %u)",
	//	qos, qos & 0x07);
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

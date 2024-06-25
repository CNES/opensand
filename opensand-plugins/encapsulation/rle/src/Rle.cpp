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
 * @file Rlee.cpp
 * @brief Rle encapsulation plugin implementation
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include <algorithm>
#include <cstring>

#include <opensand_output/Output.h>

#include <NetPacket.h>
#include <NetBurst.h>
#include <OpenSandModelConf.h>

#include "Rle.h"

const std::string ALPDU_PROTECTION_CRC{"CRC"};
const std::string ALPDU_PROTECTION_SEQ_NUM{"Sequence Number"};

constexpr std::size_t LABEL_SIZE = 3;		 // bytes
constexpr std::size_t SDU_MAX_SIZE = 4096;	 // bytes
constexpr std::size_t ALPDU_HEADER_SIZE = 3; // bytes

void rle_log(const int module_id,
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

	if (level == RLE_LOG_LEVEL_DEBUG)
	{
		output_level = LEVEL_DEBUG;
	}
	else if (level == RLE_LOG_LEVEL_INFO)
	{
		output_level = LEVEL_INFO;
	}
	else if (level == RLE_LOG_LEVEL_WARNING)
	{
		output_level = LEVEL_WARNING;
	}
	else if (level == RLE_LOG_LEVEL_ERROR)
	{
		output_level = LEVEL_ERROR;
	}
	else if (level == RLE_LOG_LEVEL_CRI)
	{
		output_level = LEVEL_CRITICAL;
	}

	va_start(args, message);
	ret = vsnprintf(buf, sizeof(buf), message, args);
	va_end(args);
	if (0 < ret && ret < (int)sizeof(buf))
	{
		DFLTLOG(output_level, "[%s:%d][%s][Module %d] %s",
				file, line, func, module_id, buf);
	}
}

void copyRleConf(const struct rle_config &src, struct rle_config &dst)
{
	dst.allow_ptype_omission = src.allow_ptype_omission;
	dst.use_compressed_ptype = src.use_compressed_ptype;
	dst.allow_alpdu_crc = src.allow_alpdu_crc;
	dst.allow_alpdu_sequence_number = src.allow_alpdu_sequence_number;
	dst.use_explicit_payload_header_map = src.use_explicit_payload_header_map;
	dst.implicit_protocol_type = src.implicit_protocol_type;
	dst.implicit_ppdu_label_size = src.implicit_ppdu_label_size;
	dst.implicit_payload_label_size = src.implicit_payload_label_size;
	dst.type_0_alpdu_label_size = src.type_0_alpdu_label_size;
}

void initRleConf(struct rle_config &conf)
{
	conf.allow_ptype_omission = 0; // 1;
	conf.use_compressed_ptype = 0; // 1; modified
	conf.allow_alpdu_crc = 0;
	conf.allow_alpdu_sequence_number = 0;
	conf.use_explicit_payload_header_map = 0;
	conf.implicit_protocol_type = 0x30; // IPv4/IPv6
	conf.implicit_ppdu_label_size = 0;
	conf.implicit_payload_label_size = 0;
	conf.type_0_alpdu_label_size = 0;
}



Rle::Rle() : SimpleEncapPlugin(NET_PROTO::RLE, "RLE")
{
	rle_set_trace_callback(&(rle_log));
}

Rle::~Rle()
{
		std::map<RleIdentifier *, rle_trans_ctxt_t, ltRleIdentifier>::iterator trans_it;

	// Reset and clean encapsulation
	for (auto &&trans_it : this->transmitters)
	{
		delete trans_it.first;
		rle_transmitter_destroy(&(trans_it.second.first));
		trans_it.second.second.clear();
	}
	this->transmitters.clear();
}

void Rle::generateConfiguration(const std::string &, const std::string &, const std::string &)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	types->addEnumType("alpdu_protection_kind", "ALPDU Protection", {ALPDU_PROTECTION_CRC, ALPDU_PROTECTION_SEQ_NUM});

	auto conf = Conf->getOrCreateComponent("encap", "Encapsulation", "The Encapsulation Plugins Configuration");
	auto rle = conf->addComponent("rle", "RLE", "The RLE Plugin Configuration");
	rle->setAdvanced(true);
	rle->addParameter("alpdu_protection", "ALPDU Protection", types->getType("alpdu_protection_kind"));
}

bool Rle::init()
{
	if (!SimpleEncapPlugin::init())
	{
		return false;
	}

	bool status = true;
	struct rle_config conf;

	initRleConf(conf);
	auto rle = OpenSandModelConf::Get()->getProfileData()->getComponent("encap")->getComponent("rle");

	std::string protection;
	rle_alpdu_protection_t alpdu_protection;

	if (!OpenSandModelConf::extractParameterData(rle->getParameter("alpdu_protection"), protection))
	{
		LOG(this->log, LEVEL_ERROR,
			"Section Rle, missing ALPDU protection parameter\n");
		status = false;
		goto unload;
	}
	if (protection == ALPDU_PROTECTION_CRC)
	{
		alpdu_protection = rle_alpdu_crc;
	}
	else if (protection == ALPDU_PROTECTION_SEQ_NUM)
	{
		alpdu_protection = rle_alpdu_sequence_number;
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
			"Section Rle: invalid value %s for ALPDU protection parameter\n",
			protection.c_str());
		status = false;
		goto unload;
	}
	LOG(this->log, LEVEL_NOTICE,
		"ALPDU protection: %s\n", protection.c_str());

	// Update rle configuration
	switch (alpdu_protection)
	{
	case rle_alpdu_crc:
		conf.allow_alpdu_sequence_number = 0;
		conf.allow_alpdu_crc = 1;
		break;
	case rle_alpdu_sequence_number:
		conf.allow_alpdu_sequence_number = 1;
		conf.allow_alpdu_crc = 0;
		break;
	default:
		status = false;
		goto unload;
	}

unload:
	this->loadRleConf(conf);
	return status;
}

void Rle::loadRleConf(const struct rle_config &conf)
{
	copyRleConf(conf, this->rle_conf);
}

bool Rle::decapNextPacket(Rt::Ptr<NetPacket> packet, std::vector<Rt::Ptr<NetPacket>> &decap_packets)
{
	bool success = false;

	uint8_t src_tal_id, dst_tal_id, qos;
	uint8_t label[LABEL_SIZE];
	unsigned char label_str[LABEL_SIZE];
	RleIdentifier *identifier = nullptr;
	std::map<RleIdentifier *, struct rle_receiver *, ltRleIdentifier>::iterator it;

	struct rle_receiver *receiver;
	struct rle_sdu *sdus = nullptr;
	size_t sdus_count = 0;
	size_t sdus_capacity = 0;
	size_t sdus_max_count = 0;
	enum rle_decap_status status;

	Rt::Data payload;

	LOG(this->log, LEVEL_DEBUG,
		"New packet to decapsulate using rle (len=%u bytes)",
		packet->getTotalLength());

	// Get data which identify the receiver
	if (packet->getPayloadLength() <= LABEL_SIZE + ALPDU_HEADER_SIZE)
	{
		LOG(this->log, LEVEL_ERROR,
			"Not enough payload in %s packet\n",
			this->getName().c_str());
		goto error;
	}
	if (!this->getLabel(packet->getPayload(), label))
	{
		LOG(this->log, LEVEL_ERROR,
			"Unable to get label from %s packet\n",
			this->getName().c_str());
		goto error;
	}
	src_tal_id = label[0];
	dst_tal_id = label[1];
	qos = label[2];
	LOG(this->log, LEVEL_DEBUG,
		"rle packet from tal %u to tal %u with qos %u",
		src_tal_id, dst_tal_id, qos);

	// Get receiver
	identifier = new RleIdentifier(src_tal_id, dst_tal_id);
	it = this->receivers.find(identifier);
	if (it == this->receivers.end())
	{
		LOG(this->log, LEVEL_DEBUG, "Packet requiring a new rle receiver");

		// Create receiver
		receiver = rle_receiver_new(&this->rle_conf);
		if (!receiver)
		{
			delete identifier;
			LOG(this->log, LEVEL_ERROR,
				"cannot create a rle receiver\n");
			goto error;
		}

		// Store receiver
		this->receivers[identifier] = receiver;
		LOG(this->log, LEVEL_DEBUG, "rle receiver created");
	}
	else
	{
		LOG(this->log, LEVEL_DEBUG, "Packet requiring an existing rle receiver");
		// Get existing identifier and context
		delete identifier;
		identifier = it->first;
		receiver = it->second;
	}

	// Prepare SDUs structures
	sdus_max_count = packet->getPayloadLength() / LABEL_SIZE;
	sdus_capacity = SDU_MAX_SIZE;
	sdus_count = 0;
	sdus = new struct rle_sdu[sdus_max_count];
	for (unsigned int i = 0; i < sdus_max_count; ++i)
	{
		sdus[i].size = 0;
		sdus[i].buffer = new unsigned char[sdus_capacity];
	}
	LOG(this->log, LEVEL_DEBUG, "Initialize SDUs before rle decapsulation (max_count=%u, count=%u)",
		sdus_max_count, sdus_count);

	// Decapsulate rle FPDU
	payload = packet->getPayload();
	status = rle_decapsulate(receiver,
							 payload.data(),
							 packet->getPayloadLength(),
							 sdus,
							 sdus_max_count,
							 &sdus_count,
							 label_str,
							 LABEL_SIZE);
	if (status != RLE_DECAP_OK)
	{
		LOG(this->log, LEVEL_ERROR,
			"rle failed to decaspulate SDU\n");
		goto error;
	}
	LOG(this->log, LEVEL_DEBUG,
		"Decapsulated SDUs (max_count=%u, count=%u)",
		sdus_max_count, sdus_count);

	// Add all SDUs to decapsulated packets list
	for (unsigned int i = 0; i < sdus_count; ++i)
	{
		struct rle_sdu sdu = sdus[i];
		Rt::Ptr<NetPacket> decap_packet = Rt::make_ptr<NetPacket>(nullptr);

		LOG(this->log, LEVEL_DEBUG,
			"Build decapsulated packet %u/%u (len=%u bytes)",
			i + 1, sdus_count, sdu.size);
		// Check SDU size
		if (sdu.size <= 0)
		{
			LOG(this->log, LEVEL_ERROR,
				"Empty rle decapsulated packet\n");
			goto error;
		}

		// Create packet from SDU
		try
		{
			decap_packet = this->build(Rt::Data(sdu.buffer, sdu.size),
									   sdu.size,
									   qos,
									   src_tal_id,
									   dst_tal_id);
		}
		catch (const std::bad_alloc &)
		{
			LOG(this->log, LEVEL_ERROR,
				"rle failed to create decapsulated packet\n");
			goto error;
		}

		// Add SDU to decapsulated packets list
		decap_packets.push_back(std::move(decap_packet));
	}

	success = true;

error:
	if (sdus)
	{
		for (unsigned int i = 0; i < sdus_max_count; ++i)
		{
			delete[] sdus[i].buffer;
		}
		delete[] sdus;
	}

	return success;
}

Rt::Ptr<NetPacket> Rle::build(const Rt::Data &data,
										  std::size_t data_length,
										  uint8_t qos,
										  uint8_t src_tal_id,
										  uint8_t dst_tal_id)
{
	// Check payload length
	if (data_length < LABEL_SIZE)
	{
		LOG(this->log, LEVEL_ERROR,
			"Payload length (%zu bytes) is lower than rle label length (%u bytes)",
			data_length, LABEL_SIZE);
		throw std::bad_alloc();
	}

	return Rt::make_ptr<NetPacket>(data, data_length,
								   this->getName(), this->getEtherType(),
								   qos, src_tal_id, dst_tal_id, 0);
}

bool Rle::encapNextPacket(Rt::Ptr<NetPacket> packet,
									  std::size_t remaining_length,
									  bool new_burst,
									  Rt::Ptr<NetPacket> &encap_packet,
									  Rt::Ptr<NetPacket> &remaining_data)
{
	uint8_t frag_id;
	uint8_t src_tal_id, dst_tal_id, qos;
	struct rle_transmitter *transmitter;
	std::vector<NetPacket *>::iterator pkt_it;
	std::map<RleIdentifier *, rle_trans_ctxt_t, ltRleIdentifier>::iterator it;
	RleIdentifier *identifier = nullptr;
	uint8_t label[LABEL_SIZE];
	enum rle_frag_status frag_status;
	enum rle_pack_status pack_status;
	struct rle_sdu sdu;
	unsigned char *ppdu = nullptr;
	size_t label_size;
	size_t ppdu_size;
	size_t fpdu_size;
	size_t fpdu_cur_pos;
	size_t prev_queue_size, queue_size;

	Rt::Data fpdu;
	unsigned char *fpdu_buffer;
	// Set default returned values
	bool partial_encap = false;
	LOG(this->log, LEVEL_DEBUG,
		"%s to encapsulate using rle (remaining len=%u bytes, packet len=%u bytes)",
		new_burst ? "New burst" : "Still same burst",
		remaining_length,
		packet->getTotalLength());

	// Get data which identify the transmitter
	src_tal_id = packet->getSrcTalId();
	if ((src_tal_id & 0x1f) != src_tal_id)
	{
		LOG(this->log, LEVEL_ERROR,
			"The source terminal id %u of %s packet is too long\n",
			src_tal_id, this->getName().c_str());
		return false;
	}

	dst_tal_id = packet->getDstTalId();
	if ((dst_tal_id & 0x1f) != dst_tal_id)
	{
		LOG(this->log, LEVEL_ERROR,
			"The destination terminal id %u of %s packet is too long\n",
			dst_tal_id, this->getName().c_str());
		return false;
	}
	qos = packet->getQos();
	if ((qos & 0x07) != qos)
	{
		LOG(this->log, LEVEL_ERROR,
			"The QoS %u of %s packet is too long\n",
			qos, this->getName().c_str());
		return false;
	}

	LOG(this->log, LEVEL_DEBUG,
		"rle packet from tal %u to tal %u with qos %u",
		src_tal_id,
		dst_tal_id,
		qos);
	// Prepare label to rle
	if (!this->getLabel(*packet, label))
	{
		LOG(this->log, LEVEL_ERROR,
			"rle failed to get label\n");
		return false;
	}

	// Get fragment id
	frag_id = qos;

	// Get transmitter
	identifier = new RleIdentifier(src_tal_id, dst_tal_id);
	it = this->transmitters.find(identifier);
	if (it == this->transmitters.end())
	{

		LOG(this->log, LEVEL_DEBUG, "Packet requiring a new rle transmitter");

		// Create transmitter
		transmitter = rle_transmitter_new(&this->rle_conf);
		if (!transmitter)
		{
			delete identifier;
			LOG(this->log, LEVEL_ERROR,
				"cannot create a rle transmitter\n");
			return false;
		}

		// Store transmitter
		auto result = this->transmitters.emplace(identifier,
												 std::make_pair(transmitter, std::vector<NetPacket *>{}));
		it = result.first;
		if (it == this->transmitters.end() || !result.second)
		{
			delete identifier;
			rle_transmitter_destroy(&transmitter);
			LOG(this->log, LEVEL_ERROR,
				"cannot store the rle transmitter\n");
			return false;
		}
		LOG(this->log, LEVEL_DEBUG, "rle transmitter created");
	}
	else
	{

		LOG(this->log, LEVEL_DEBUG, "Packet requiring an existing rle transmitter");
		// Get existing identifier and context
		delete identifier;
		identifier = it->first;
		transmitter = it->second.first;
	}

	// Check packet has already been partially sent
	std::vector<NetPacket *> &sent_packets = it->second.second;
	prev_queue_size = rle_transmitter_stats_get_queue_size(transmitter, frag_id);
	LOG(this->log, LEVEL_DEBUG,
		"Already sent packets (total=%u)",
		sent_packets.size());

	pkt_it = std::find(sent_packets.begin(), sent_packets.end(), packet.get());
	if (pkt_it == sent_packets.end())
	{
		LOG(this->log, LEVEL_DEBUG, "rle encapsulation of this SDU (len=%u bytes)",
			packet->getTotalLength());
		if (0 < prev_queue_size)
		{
			LOG(this->log, LEVEL_ERROR, "rle encapsulation already in progress (queue size=%u bytes)",
				prev_queue_size);
		}

		// Build rle SDU
		sdu.protocol_type = to_underlying(packet->getType());
		sdu.size = packet->getTotalLength();
		sdu.buffer = new unsigned char[sdu.size];
		memcpy(sdu.buffer, packet->getRawData(), sdu.size);

		// Encapsulate rle SDU
		if (rle_encapsulate(transmitter, &sdu, frag_id) != 0)
		{
			delete[] sdu.buffer;
			LOG(this->log, LEVEL_ERROR,
				"rle failed to encaspulate SDU\n");
			return false;
		}
		delete[] sdu.buffer;
		sdu.size = 0;
	}
	else
	{
		LOG(this->log, LEVEL_DEBUG,
			"This SDU (len=%u bytes) is already encapsulated using rle)",
			packet->getTotalLength());
	}

	// Update label size
	label_size = new_burst ? LABEL_SIZE : 0;
	remaining_length -= label_size;

	// Fragment rle SDU to rle PPDU
	LOG(this->log, LEVEL_DEBUG, "rle fragmentation");
	ppdu_size = 0;
	LOG(this->log, LEVEL_DEBUG,
		"transmitter=%s, frag_id=%u, remaining_len=%u, ppdu=%s, ppdu_len=%u",
		transmitter ? "not null" : "NULL",
		frag_id,
		remaining_length,
		ppdu ? "not null" : "NULL",
		ppdu_size);
	frag_status = rle_fragment(transmitter, frag_id, remaining_length, &ppdu, &ppdu_size);
	LOG(this->log, LEVEL_DEBUG,
		"transmitter=%s, frag_id=%u, remaining_len=%u, ppdu=%s, ppdu_len=%u",
		transmitter ? "not null" : "NULL",
		frag_id,
		remaining_length,
		ppdu ? "not null" : "NULL",
		ppdu_size);
	if (frag_status == RLE_FRAG_ERR_BURST_TOO_SMALL)
	{
		LOG(this->log, LEVEL_INFO,
			"Not enough remaining length to fragment PPDU using rle (%u bytes)",
			remaining_length);
		partial_encap = true;
		goto encap_end;
	}
	else if (frag_status != 0)
	{
		LOG(this->log, LEVEL_ERROR,
			"rle failed to fragment ALPDU (code=%d)\n",
			(int)frag_status);
		return false;
	}
	LOG(this->log, LEVEL_DEBUG,
		"rle PPDU len=%u bytes",
		ppdu_size);

	// Check encapsualtion is partial or complete
	queue_size = rle_transmitter_stats_get_queue_size(transmitter, frag_id);
	partial_encap = (0 < queue_size);
	LOG(this->log, LEVEL_DEBUG,
		"%s rle encapsulation (queue size=%u bytes, variation=%s%d bytes)",
		partial_encap ? "Partial" : "Complete",
		queue_size,
		prev_queue_size < queue_size ? "+" : "",
		(int)queue_size - (int)prev_queue_size);

	// Prepare FPDU
	fpdu_size = ppdu_size + label_size;
	fpdu_cur_pos = 0;
	fpdu_buffer = new unsigned char[fpdu_size];

	// Pack rle PPD to rle FPDU
	LOG(this->log, LEVEL_DEBUG,
		"rle packing (FPDU len=%u bytes, FPDU pos=%u)",
		fpdu_size, fpdu_cur_pos);
	pack_status = rle_pack(ppdu, ppdu_size, label, label_size, fpdu_buffer, &fpdu_cur_pos, &fpdu_size);
	if (pack_status == RLE_PACK_ERR_FPDU_TOO_SMALL)
	{
		LOG(this->log, LEVEL_INFO,
			"Not enough remaining length to pack FPDU using rle (%u bytes)",
			fpdu_size);
		partial_encap = true;
		goto encap_end;
	}
	if (pack_status != 0)
	{
		delete[] fpdu_buffer;
		LOG(this->log, LEVEL_ERROR,
			"rle failed to pack PPDU (code=%d)\n",
			(int)pack_status);
		return false;
	}
	fpdu.assign(fpdu_buffer, fpdu_cur_pos);
	delete[] fpdu_buffer;

	encap_packet.reset(new NetPacket(fpdu, fpdu_cur_pos,
									 this->getName(),
									 this->getEtherType(),
									 qos, src_tal_id, dst_tal_id, 0));

encap_end:
	if ((pkt_it == sent_packets.end()) && partial_encap)
	{
		// Add packet to partial sent packets list
		LOG(this->log, LEVEL_DEBUG, "Add packet to partially sent packets list");
		sent_packets.push_back(packet.get());
	}
	else if ((pkt_it != sent_packets.end()) && !partial_encap)
	{
		// Remove packet from partial sent packets list
		LOG(this->log, LEVEL_DEBUG, "Remove packet from partially sent packets list");
		sent_packets.erase(pkt_it);
	}

	if (partial_encap)
	{
		remaining_data = std::move(packet);
	}

	return true;
}

bool Rle::decapAllPackets(Rt::Ptr<NetContainer> encap_packets,
									  std::vector<Rt::Ptr<NetPacket>> &decap_packets,
									  unsigned int)
{
	// Set default

	// Build packet
	Rt::Ptr<NetPacket> decap_packet = Rt::make_ptr<NetPacket>(nullptr);
	try
	{
		decap_packet = this->build(encap_packets->getPayload(), encap_packets->getPayloadLength(),
								   0x00, BROADCAST_TAL_ID, BROADCAST_TAL_ID);
	}
	catch (const std::bad_alloc &)
	{
		LOG(this->log, LEVEL_ERROR,
			"cannot create one %s packet (length = %zu bytes, payload length = %zu bytes)\n",
			this->getName().c_str(), encap_packets->getTotalLength(), encap_packets->getPayloadLength());
		return false;
	}
	this->decapNextPacket(std::move(decap_packet), decap_packets);

	return true;
}

bool Rle::getSrc(const Rt::Data &data, tal_id_t &tal_id) const
{
	uint8_t label[LABEL_SIZE];

	// Get label (this will success if it is the first fragment)
	if (!this->getLabel(data, label))
	{
		return false;
	}

	tal_id = label[0];
	return true;
}

bool Rle::getQos(const Rt::Data &data, qos_t &qos) const
{
	uint8_t label[LABEL_SIZE];

	// Get label (this will success if it is the first fragment)
	if (!this->getLabel(data, label))
	{
		return false;
	}

	qos = label[2];
	return true;
}

bool Rle::getDst(const Rt::Data &data, tal_id_t &tal_id) const
{
	uint8_t label[LABEL_SIZE];

	// Get label (this will success if it is the first fragment)
	if (!this->getLabel(data, label))
	{
		return false;
	}

	tal_id = label[1];
	return true;
}

// Static methods

bool Rle::getLabel(const NetPacket &packet, uint8_t label[])
{
	tal_id_t src_tal_id = packet.getSrcTalId();
	tal_id_t dst_tal_id = packet.getDstTalId();
	qos_t qos = packet.getQos();
	if ((src_tal_id & 0x1F) != src_tal_id || (dst_tal_id & 0x1F) != dst_tal_id || (qos & 0x07) != qos)
	{
		return false;
	}
	label[0] = src_tal_id & 0x1F;
	label[1] = dst_tal_id & 0x1F;
	label[2] = qos & 0x07;
	return true;
}

bool Rle::getLabel(const Rt::Data &data, uint8_t label[])
{
	uint8_t src_tal_id = (uint8_t)(data.at(0));
	uint8_t dst_tal_id = (uint8_t)(data.at(1));
	uint8_t qos = (uint8_t)(data.at(2));
	if ((src_tal_id & 0x1F) != src_tal_id || (dst_tal_id & 0x1F) != dst_tal_id || (qos & 0x07) != qos)
	{
		return false;
	}
	label[0] = src_tal_id & 0x1F;
	label[1] = dst_tal_id & 0x1F;
	label[2] = qos & 0x07;
	return true;
}

bool Rle::setHeaderExtensions(Rt::Ptr<NetPacket>,
										  Rt::Ptr<NetPacket> &,
										  tal_id_t,
										  tal_id_t,
										  std::string,
										  void *)
{
	return false;
}

bool Rle::getHeaderExtensions(const Rt::Ptr<NetPacket> &,
										  std::string,
										  void *)
{
	return false;
}

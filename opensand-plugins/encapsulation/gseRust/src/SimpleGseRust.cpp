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
 * @file GseRust.cpp
 * @brief GSE encapsulation plugin implementation
 * @author Axel Pinel <axel.pinel@viveris.fr>
 */

#include "SimpleGseRust.h"
#include <NetPacket.h>
#include <NetBurst.h>
#include <OpenSandModelConf.h>
#include <opensand_output/Output.h>
#include <memory>
#include <iostream>
#include <iomanip>
#include <string.h>

bool SimpleGseRust::init()
{
	return true;
}

Rt::Ptr<NetPacket> SimpleGseRust::decapNextPacket(const Rt::Data &data, size_t &length_decap)
{
	length_decap = 0;

	RustSlice gse_pkt = {.size = data.size(), .bytes = data.data()};
	RustDecapStatus status = rust_decap(gse_pkt, this->rust_decapsulator);
	length_decap = status.len_pkt;

	switch (status.status)
	{
	case RustDecapStatusType::DecapCompletedPkt:
	{

		uint8_t *label_bytes = status.value.completed_pkt.metadata.label.bytes;
		uint8_t src_tal_id = SimpleGseRust::getSrcTalIdFromLabel(label_bytes);
		uint8_t dst_tal_id = SimpleGseRust::getDstTalIdFromLabel(label_bytes);
		uint8_t qos = SimpleGseRust::getQosFromLabel(label_bytes);

		Rt::Ptr<NetPacket> packet = Rt::make_ptr<NetPacket>(
			status.value.completed_pkt.pdu.bytes,
			status.value.completed_pkt.metadata.pdu_len,
			this->getName(),
			this->getEtherType(),
			qos,
			src_tal_id,
			dst_tal_id,
			0);

		LOG(this->log_simple, LEVEL_INFO,
			"Completed Decapsulation of a GSE packet of %lu-pdu-bytes (payload of %lu bytes), with QoS = %u, Dst Id = %u, Src Id = %u\n\n",
			status.len_pkt, packet->getPayloadLength(), qos, dst_tal_id, src_tal_id);

		// the decapbuffer can be free after the copy of the GSE payload  in the NetPacket
		if (!c_memory_provision_storage(&this->decap_buffer, status.value.completed_pkt.pdu))
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"failed to free the decapsulation buffer");
		}
		LOG(this->log_simple, LEVEL_INFO,
			"Checking for extension header header");

		// RustGetExtensionsHeader ext_header = rust_getExtensionHeaders(gse_pkt, this->rust_decapsulator);

		CHeaderExtensionSlice extensions = status.value.completed_pkt.metadata.extensions;

		if (extensions.size == 0)
		{
			LOG(this->log_simple, LEVEL_INFO,
				"No extension header found");
			return packet;
		}

		LOG(this->log_simple, LEVEL_DEBUG,
			"Read %lu extension headers", extensions.size);
		for (size_t i = 0; i < extensions.size; i++)
		{
			Rt::Data header_ext_data;
			uint16_t id = extensions.bytes[i].id;
			LOG(this->log_simple, LEVEL_DEBUG,
				"extension %lu : id = %u", i, id);

			LOG(this->log_simple, LEVEL_DEBUG,
				"applying mask to get the size of the extension data");

			uint16_t ext_data_len = (id >> 8) & 0b111;

			LOG(this->log_simple, LEVEL_DEBUG,
				"H-LEN is %u", ext_data_len);

			switch (ext_data_len) // TODO function
			{
			case 2:
			{
				header_ext_data.assign(extensions.bytes[i].data, extensions.bytes[i].data + 2 * sizeof(extensions.bytes[i].data[0]));
				LOG(this->log_simple, LEVEL_DEBUG,
					"extension has 2 b data");
				break;
			}
			case 3:
			{
				header_ext_data.assign(extensions.bytes[i].data, extensions.bytes[i].data + 4 * sizeof(extensions.bytes[i].data[0]));
				LOG(this->log_simple, LEVEL_DEBUG,
					"extension has 4 b data");
				break;
			}
			case 4:
			{
				header_ext_data.assign(extensions.bytes[i].data, extensions.bytes[i].data + 6 * sizeof(extensions.bytes[i].data[0]));
				LOG(this->log_simple, LEVEL_DEBUG,
					"extension has 6 b data");
				break;
			}
			case 5:
			{
				header_ext_data.assign(extensions.bytes[i].data, extensions.bytes[i].data + 8 * sizeof(extensions.bytes[i].data[0]));
				LOG(this->log_simple, LEVEL_DEBUG,
					"extension has 8 b data");
				break;
			}
			case 0:
			{
				LOG(this->log_simple, LEVEL_ERROR,
					"extension is mandatory extension header. Unreachable. Aborting");
				assert(false);
			}
			case 1:
			{
				LOG(this->log_simple, LEVEL_DEBUG,
					"extension has no data");
				break;
			}
			default:
			{
				LOG(this->log_simple, LEVEL_ERROR,
					"Unexpected H-LEN : %u. Unreachable. Aborting.", ext_data_len);
				break;
			}
			}

			if (!packet->addExtensionHeader(extensions.bytes[i].id, header_ext_data))
			{
				LOG(this->log_simple, LEVEL_DEBUG,
					"failed to add extension (id = %u) to Netpacket", extensions.bytes[i].id);
			};
		}
		return packet;
	}

	case RustDecapStatusType::DecapFragmentedPkt:
	{ // TODO gerer les cas ou le premier fragment a des extensions d'headers.
		LOG(this->log_simple, LEVEL_INFO,
			"Packet is a first / intermediate fragment. Fragment stored in memory.");

		LOG(this->log_simple, LEVEL_INFO,
			"NOOOOO Checking for ext header");
		// TODO RustGetExtensionsHeader ext_header = rust_getExtensionHeaders(gse_pkt, this->rust_decapsulator);

		return Rt::make_ptr<NetPacket>(nullptr);
	}

	case RustDecapStatusType::DecapPadding:
	{ // no more data in the bb frame
		// getChunk iterate over the BBFrame knowing how many packets are contained
		LOG(this->log_simple, LEVEL_ERROR,
			"ERROR Rust Decapsulation found padding data (return DecapPadding). Supposed to be unreachable");
		assert(false);
		return Rt::make_ptr<NetPacket>(nullptr);
	}

	default:
	{

		char error[50];
		if (decapstatus_to_string(status.status, error))
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"error during decapsulation : %s", error);
		}
		else
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"error during decapsulation. Impossible to convert error to string; Unknow Error.");
		}
		return Rt::make_ptr<NetPacket>(nullptr);
	}
	}
	LOG(this->log_simple, LEVEL_DEBUG,
		"Checking now for header ext");
}

bool SimpleGseRust::decapAllPackets(Rt::Ptr<NetContainer> encap_packets,
									std::vector<Rt::Ptr<NetPacket>> &decap_packets,
									unsigned int decap_packets_count)
{ //TODO refactor in one loop
	std::vector<Rt::Ptr<NetPacket>> packets_decap{};
	std::vector<Rt::Ptr<NetPacket>> packets_decap_ret{};
	std::size_t previous_length = 0;

	if (decap_packets_count <= 0)
	{
		decap_packets = std::move(packets_decap);
		LOG(this->log_simple, LEVEL_INFO,
			"No packet to decapsulate in this BBFrame\n");
		return true;
	}

	LOG(this->log_simple, LEVEL_INFO,
		"%u packet(s) to decapsulate\n",
		decap_packets_count);

	size_t length_pkt_decap = 0;

	for (unsigned int i = 0; i < decap_packets_count; ++i)
	{
		// Get the current packet
		Rt::Ptr<NetPacket> current = Rt::make_ptr<NetPacket>(nullptr);
		try
		{
			current = this->decapNextPacket(
				encap_packets->getPayload(previous_length),
				length_pkt_decap);
		}
		catch (const std::bad_alloc &)
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"cannot create one %s packet (length = %zu bytes)\n",
				this->getName().c_str(), 0);
			return false;
		}

		if (current != nullptr)
		{
			// Add the current packet to decapsulated packets
			packets_decap.push_back(std::move(current));

			previous_length += length_pkt_decap;
		}
		// otherwise, most likely a first or intermediate fragment -> continue
	}

	// Dropping packet not adressed to me
	for (auto &&packet : packets_decap)
	{
		uint8_t dst_tal_id;
		// packet must be valid
		if (!packet)
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"encapsulation packet is not valid, drop the packet\n");
			continue;
		}
		// Filter if packet is for me
		dst_tal_id = packet->getDstTalId();

		if (this->dst_tal_id == BROADCAST_TAL_ID)
		{
			LOG(this->log_simple, LEVEL_INFO,
				"My id is broadcast id (#%u). Keeping this packet with destination TAL id #%u",
				this->dst_tal_id, dst_tal_id);
			packets_decap_ret.push_back(std::move(packet));
			continue;
		}
		if (dst_tal_id == BROADCAST_TAL_ID)
		{
			LOG(this->log_simple, LEVEL_INFO,
				"Packet destination adress is broadcast (id #%u). ",
				dst_tal_id);
			packets_decap_ret.push_back(std::move(packet));
			continue;
		}
		if (dst_tal_id == this->dst_tal_id)
		{
			LOG(this->log_simple, LEVEL_INFO,
				"Packet destination adress is me (id #%u).",
				dst_tal_id);
			packets_decap_ret.push_back(std::move(packet));
			continue;
		}
		LOG(this->log_simple, LEVEL_INFO,
			"encapsulation packet dst id is #%u. Drop\n",
			packet->getDstTalId());
		continue;
	}

	// Set returned decapsulated packets
	decap_packets = std::move(packets_decap_ret);
	return true;
}
bool SimpleGseRust::setHeaderExtensions(Rt::Ptr<NetPacket> packet,
										Rt::Ptr<NetPacket> &new_packet,
										tal_id_t tal_id_src,
										tal_id_t tal_id_dst,
										std::string callback_name,
										void *opaque)
{
	LOG(this->log_simple, LEVEL_DEBUG,
		"setting header extension for CNI");
	static unsigned char empty_gse[7] =
		{
			0xd0, /* LT = 01 (three bytes label) */
			0x05, /* length */
			to_underlying(NET_PROTO::IPV4) >> 8 & 0xff,
			to_underlying(NET_PROTO::IPV4) & 0xff,
			(unsigned char)tal_id_src,
			(unsigned char)tal_id_dst,
			0x00 /* highest priority fifo (eg. NM FIFO) */
		};

	if (!packet)
	{
		LOG(this->log_simple, LEVEL_INFO,
			"no packet, create empty one\n");
		packet = Rt::make_ptr<NetPacket>(empty_gse, 7);
	}

	uint16_t ext_id = to_underlying(NET_PROTO::GSE_EXTENSION_CNI) | 0x0300;
	uint32_t *cni = (uint32_t *)opaque;

	new_packet = Rt::make_ptr<NetPacket>(
		packet->getData(),
		packet->getTotalLength(),
		this->getName(),
		this->getEtherType(),
		0x00, /* highest priority fifo (eg. NM FIFO) */
		(uint8_t)(tal_id_src),
		(uint8_t)(tal_id_dst),
		0);

	std::array<char, 4> chars;
	chars[0] = static_cast<char>((*cni >> 24) & 0xFF);
	chars[1] = static_cast<char>((*cni >> 16) & 0xFF);
	chars[2] = static_cast<char>((*cni >> 8) & 0xFF);
	chars[3] = static_cast<char>(*cni & 0xFF);

	Rt::Data rtData(chars.begin(), chars.end());

	if (!new_packet->addExtensionHeader(ext_id, rtData))
	{
		LOG(this->log_simple, LEVEL_ERROR,
			"adding ExtensionHeader to NetPacket failed, id was %u\n", ext_id);
		return false;
	}
	LOG(this->log_simple, LEVEL_ERROR,
		"added ExtensionHeader (id = %u) to NetPacket map", ext_id);
	return true;
}

bool SimpleGseRust::getHeaderExtensions(const Rt::Ptr<NetPacket> &packet,
										std::string callback_name,
										void *opaque)
{
	LOG(this->log_simple, LEVEL_INFO,
		"Reading Header Extension from a %u-bytes packet", packet->getData().size());

	std::vector<uint16_t> ext_ids = packet->getAllExtensionHeadersId();

	if (ext_ids.empty())
	{
		LOG(this->log_simple, LEVEL_INFO,
			"no Header Extension");
		return true;
	}
	LOG(this->log_simple, LEVEL_INFO,
		"Packet has %zx header extensions.", ext_ids.size());
	uint16_t id_cni = to_underlying(NET_PROTO::GSE_EXTENSION_CNI) | 0x0300;

	auto it = std::find(ext_ids.begin(), ext_ids.end(), id_cni);

	if (it == ext_ids.end())
	{
		LOG(this->log_simple, LEVEL_INFO,
			"No CNI header extension");
		return true;
	}

	Rt::Data data_cni = packet->getExtensionHeaderValueById(id_cni);

	if (data_cni.size() < 4)
	{
		LOG(this->log_simple, LEVEL_ERROR,
			"CNI header extension founded but it has less than 4 bytes of data (%zx bytes) ", data_cni.size());
		return true;
	}
	uint32_t result = 0;
	result |= (static_cast<uint32_t>(data_cni[0]) << 24);
	result |= (static_cast<uint32_t>(data_cni[1]) << 16);
	result |= (static_cast<uint32_t>(data_cni[2]) << 8);
	result |= static_cast<uint32_t>(data_cni[3]);
	memcpy(opaque, &result, sizeof(uint32_t));

	return true;
}

bool SimpleGseRust::encapNextPacket(Rt::Ptr<NetPacket> packet,
									std::size_t remaining_length,
									bool new_burst,
									Rt::Ptr<NetPacket> &encap_packet,
									Rt::Ptr<NetPacket> &remaining_data)
{
	bool contestExists = false;

	size_t size_data_encap = remaining_length;

	// TODO buffer_encap should not exceed 4Ko + 2 o (check)
	uint8_t buffer_encap[size_data_encap];
	uint8_t frag_id = SimpleGseRust::getFragId(*packet);
	RustSlice payload = (RustSlice){.size = packet->getTotalLength(),
									.bytes = packet->getRawData()};
	RustMutSlice gse_pck = {.size = size_data_encap,
							.bytes = buffer_encap};
	GseIdentifier identifier{packet->getSrcTalId(),
							 packet->getDstTalId(),
							 packet->getQos()};
	RustEncapStatus status_encap;
	// looking for the context and encapsulate
	auto context_it = this->contexts.find(identifier);
	if (context_it != this->contexts.end())
	{ // find the context, this is a fragment, already saw this payload
		contestExists = true;
		LOG(this->log_simple, LEVEL_DEBUG,
			"context exist, calling rust_encap_frag()\n");
		status_encap = rust_encap_frag(payload, context_it->second, gse_pck, this->rust_encapsulator);
	}
	else
	{ // no context corresponding, first time with this payload
		// checking for header extension
		bool PacketHaxExtHeader = false;
		std::vector<uint16_t> ext_head = packet->getAllExtensionHeadersId();
		CHeaderExtension extensions[ext_head.size()];
		CHeaderExtensionSlice slice;

		if (!ext_head.empty())
		{
			PacketHaxExtHeader = true;
			LOG(this->log_simple, LEVEL_ERROR,
				"Packet has %zx header ext\n", ext_head.size());

			std::size_t size = ext_head.size();

			std::size_t index = 0;

			for (uint16_t id : ext_head)
			{
				Rt::Data ext_data = packet->getExtensionHeaderValueById(id);

				std::size_t dataLength = ext_data.size();
				extensions[index].id = id;
				if (dataLength > 8)
				{
					LOG(this->log_simple, LEVEL_ERROR,
						" header ext with ID %zx has more than 8 bytes of data ( %zx found)\n", ext_head.size(), dataLength);
					return false;
				}
				// Set the type based on the length of data
				LOG(this->log_simple, LEVEL_DEBUG,
					" header ext with ID %zx, data size of %zx bytes)\n", ext_head.size(), dataLength);

				if (dataLength != 0 && dataLength != 2 && dataLength != 4 && dataLength != 6 && dataLength != 8)
				{
					LOG(this->log_simple, LEVEL_DEBUG,
						" header ext with ID %zx has not-expected data size : %zx bytes. Expected 0, 2, 4, 6 or 8.). Skipping this extension.\n", ext_head.size(), dataLength);
					continue;
				}
				extensions[index].data = new uint8_t[dataLength];
				std::copy(ext_data.begin(), ext_data.begin() + dataLength, extensions[index].data);
				index++;
			}
			slice = {size, extensions};
		}

		struct RustLabel rustLabel;
		if (this->force_compatibility)
		{ // C library works only with 6-bytes labels
			rustLabel.label_type = RustLabelType::SixBytes;
		}
		else
		{
			rustLabel.label_type = RustLabelType::ThreeBytes;
		}

		if (!SimpleGseRust::setLabel(*packet, rustLabel.bytes))
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"Failed to set the label for rust encapsulation\n");
			return false;
		}

		RustEncapMetadata meta = {
			.protocol_type = static_cast<uint16_t>(packet->getType()),
			.label = rustLabel};

		if (this->rust_encapsulator == nullptr)
		{
			LOG(this->log_simple, LEVEL_CRITICAL,
				"Lost pointer to Rust encapsulator object. Ptr is nullptr. UB. Aborting\n");
			assert(false);
		}

		// Call rust_encap function with appropriate parameters
		if (PacketHaxExtHeader)
		{
			LOG(this->log_simple, LEVEL_DEBUG,
				"Encapsulating using rust_encap_ext\n");

			status_encap = rust_encap_ext(payload, frag_id, meta, gse_pck, this->rust_encapsulator, slice);
		}
		else
		{
			LOG(this->log_simple, LEVEL_DEBUG,
				"Encapsulating using rust_encap_ext\n");
			status_encap = rust_encap(payload, frag_id, meta, gse_pck, this->rust_encapsulator);
		}

	} // end if looking for the context and encapsulate

	switch (status_encap.status)
	{
	case RustEncapStatusType::EncapCompletedPkt:
	{
		LOG(this->log_simple, LEVEL_DEBUG,
			"Entire encapsulation of a %zu-bytes packet in a %zu-bytes GSE packet"
			"with SRC TAL Id = %u, DST TAL Id = %u, QoS = %u, network type = 0x%04x, FragId (if used): %d\n",
			packet->getTotalLength(), status_encap.value.completed_pkt,
			packet->getSrcTalId(), packet->getDstTalId(), packet->getQos(), packet->getType(), frag_id);
		if (contestExists)
		{

			LOG(this->log_simple, LEVEL_DEBUG,
				"Context associated deleted");
			this->contexts.erase(identifier);
			remaining_data = nullptr;
		}

		encap_packet = Rt::make_ptr<NetPacket>(gse_pck.bytes, status_encap.value.completed_pkt,
											   this->getName(), this->getEtherType(),
											   packet->getQos(), packet->getSrcTalId(), packet->getDstTalId(), 0);
		return true;
	}

	case RustEncapStatusType::EncapFragmentedPkt:
	{
		LOG(this->log_simple, LEVEL_DEBUG,
			"Partial encapsulation of a %zu bytes of  the %zu bytes packet in a %zu-bytes GSE packet"
			"with SRC TAL Id = %u, DST TAL Id = %u, QoS = %u, network type = 0x%04x, FragId (if used): %d\n",
			status_encap.value.fragmented_pkt.len_pkt, packet->getTotalLength(), status_encap.value.completed_pkt,
			packet->getSrcTalId(), packet->getDstTalId(), packet->getQos(), packet->getType(), frag_id);

		encap_packet = Rt::make_ptr<NetPacket>(gse_pck.bytes, status_encap.value.completed_pkt,
											   this->getName(), this->getEtherType(),
											   packet->getQos(), packet->getSrcTalId(), packet->getDstTalId(), 0);

		// save the context
		this->contexts.emplace(identifier, status_encap.value.fragmented_pkt.context);

		// the entire packet need to be saved for next time, size read is stored in the context
		remaining_data = std::move(packet); // remaining data is the entire packet

		return true;
	}
	default:
	{
		char error[50];
		if (encapstatus_to_string(status_encap.status, error))
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"error during encapsulation : %s", error);
		}
		else
		{
			LOG(this->log_simple, LEVEL_ERROR,
				"error during encapsulation. Impossible to convert error to string; Unknow Error.");
		}
		return false;
	}
	}
	return true;
}

// CONSTRUCTORS
SimpleGseRust::SimpleGseRust() : SimpleEncapPlugin(NET_PROTO::GSE, "gse")
{
	// initialize using default value
	uint8_t max_frag_id = 5;		   // 5 FIFO so 5 id should be ok, but Gse protocol allows 256 different id
	uint16_t decap_buffer_len = 12000; // GSE protocol allows entire packet of 65 536 bytes
	this->force_compatibility = false;

	auto gse = OpenSandModelConf::Get()->getProfileData()->getComponent("encap")->getComponent("gse_Rust");
	if (!gse)
	{
		return;
	}
	OpenSandModelConf::extractParameterData(gse->getParameter("max_frag_id"), max_frag_id);

	OpenSandModelConf::extractParameterData(gse->getParameter("decap_buffer_len"), decap_buffer_len);
	OpenSandModelConf::extractParameterData(gse->getParameter("compatibility_mode"), this->force_compatibility);

	this->decap_buffer = c_memory_new(max_frag_id, decap_buffer_len);
	this->rust_encapsulator = create_encapsulator();

	if (this->force_compatibility == true)
	{
		enable_labelReUse(this->rust_encapsulator, false);
	}

	this->rust_decapsulator = create_deencapsulator(this->decap_buffer);
}

SimpleGseRust::~SimpleGseRust()
{
	c_memory_delete(this->decap_buffer);
}

void SimpleGseRust::generateConfiguration(const std::string &, const std::string &, const std::string &)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	auto conf = Conf->getOrCreateComponent("encap", "Encapsulation", "The Encapsulation Plugins Configuration");
	auto gse = conf->addComponent("gse_Rust", "GSE", "The GSE Plugin Configuration");
	conf->setAdvanced(true);
	auto gse_enum = std::dynamic_pointer_cast<OpenSANDConf::MetaEnumType>(types->getType("GSE_library_type"));
	if (gse_enum)
	{
		gse_enum->getMutableValues().push_back("Rust");
	}
	else
	{
		types->addEnumType("GSE_library_type", "GSE protocol libraries types", {"Rust"});
		conf->addParameter("GSE_library", "the GSE protocol library used", types->getType("GSE_library_type"));
	}

	auto lib_type = conf->getParameter("GSE_library");

	gse->addParameter("max_frag_id", "Maximum frag id possible (= number of deencapsulation buffer)", types->getType("ubyte"));
	gse->addParameter("decap_buffer_len", "Maximal Packet length", types->getType("ushort"));
	gse->addParameter("compatibility_mode", "Force compatibility with lib DVB-GSE written in language C", types->getType("bool"));

	Conf->setProfileReference(gse, lib_type, "Rust");
}

// Static methods
bool SimpleGseRust::setLabel(const NetPacket &packet, uint8_t label[])
{
	tal_id_t src_tal_id = packet.getSrcTalId();
	tal_id_t dst_tal_id = packet.getDstTalId();
	qos_t qos = packet.getQos();

	if (((src_tal_id & 0x1F) != src_tal_id) || ((dst_tal_id & 0x1F) != dst_tal_id) || ((qos & 0x07) != qos))
	{
		return false;
	}

	label[0] = static_cast<uint8_t>(src_tal_id & 0x1F);
	label[1] = static_cast<uint8_t>(dst_tal_id & 0x1F);
	label[2] = static_cast<uint8_t>(qos & 0x07);
	label[3] = 0;
	label[4] = 0;
	label[5] = 0;
	return true;
}

uint8_t SimpleGseRust::getSrcTalIdFromLabel(const uint8_t label[])
{
	return label[0] & 0x1F;
}

uint8_t SimpleGseRust::getDstTalIdFromLabel(const uint8_t label[])
{
	return label[1] & 0x1F;
}

uint8_t SimpleGseRust::getQosFromLabel(const uint8_t label[])
{
	return label[2] & 0x07;
}

uint8_t SimpleGseRust::getFragId(const NetPacket &packet)
{
	uint8_t src_tal_id = packet.getSrcTalId();
	uint8_t qos = packet.getQos();
	return ((src_tal_id & 0x1F) << 3 | ((qos & 0x07)));
}

uint8_t SimpleGseRust::getSrcTalIdFromFragId(const uint8_t frag_id)
{
	return (frag_id >> 3) & 0x1F;
}

uint8_t SimpleGseRust::getQosFromFragId(const uint8_t frag_id)
{
	return frag_id & 0x07;
}
// ------------------------------------------------ END USELESS BUT CALLED METHODS ------------------------------------------------

// ------------------------------------------------- USELESS AND NOT CALLED METHOD -------------------------------------------------
// these methods must not be called. If they are, stop the process

// this method must not be called sauf en SCPC idiot
bool SimpleGseRust::getSrc(const Rt::Data &data, tal_id_t &tal_id) const
{
	LOG(this->log_simple, LEVEL_DEBUG,
		"Looking for FragId or Label in a %lu-bytes packet", data.size());
	RustSlice packet = (RustSlice){.size = data.size(),
								   .bytes = data.data()};
	RustExtractLabelorFragIdStatus status = rust_getFragIdOrLbl(packet, this->rust_decapsulator);

	switch (status.status)
	{
	case RustExtractLabelorFragIdType::ResLbl:
		LOG(this->log_simple, LEVEL_DEBUG,
			"This is a complete packet or first fragment. Extracting Src from Label.");
		tal_id = getSrcTalIdFromLabel(status.value.label.bytes);

		LOG(this->log_simple, LEVEL_DEBUG,
			"Source ID is %u", tal_id);
		return true;

	case RustExtractLabelorFragIdType::ResFragId:
		LOG(this->log_simple, LEVEL_DEBUG,
			"Packet given is a intermediate / last fragment. Extracting Src from Label.");
		tal_id = getSrcTalIdFromFragId(status.value.fragid);
		LOG(this->log_simple, LEVEL_DEBUG,
			"Source ID is %u", tal_id);
		return true;

	case RustExtractLabelorFragIdType::ErrorLabelReUse:
		LOG(this->log_simple, LEVEL_ERROR,
			"Read label Reuse but no Label stored.");

	case RustExtractLabelorFragIdType::ErrorSizeBuffer:
		LOG(this->log_simple, LEVEL_ERROR,
			"Packet too small to be a Gse Packet");

	case RustExtractLabelorFragIdType::ErrorHeaderRead:
		LOG(this->log_simple, LEVEL_ERROR,
			"Gse Rust library failed to read Header");
		return false;

	default:
		LOG(this->log_simple, LEVEL_ERROR,
			"rust_getFragIdOrLbl failed with unknown error. Supposed to be unreachable");
		return false;
	}
	return true;
}

// this method must not be called
bool SimpleGseRust::getDst(const Rt::Data &data, tal_id_t &tal_id) const
{
	LOG(this->log_simple, LEVEL_ERROR,
		"GseRust::Handler::getDst called");
	assert(false);

	return true;
}

// this method must not be called
bool SimpleGseRust::getQos(const Rt::Data &data, qos_t &qos) const
{
	LOG(this->log_simple, LEVEL_ERROR,
		"GseRust::Handler::getQos called");
	assert(false);
	return true;
}

// this method must not be called
Rt::Ptr<NetPacket> SimpleGseRust::build(const Rt::Data &data,
										size_t data_length,
										uint8_t qos,
										uint8_t src_tal_id,
										uint8_t dst_tal_id)
{
	LOG(this->log_simple, LEVEL_ERROR,
		"ERROR GseRust::Handler::build() has been called. Aborting.");
	assert(false);
	return Rt::make_ptr<NetPacket>(nullptr);
}

// ------------------------------------------------ END USELESS AND NOT CALLED METHOD ------------------------------------------------

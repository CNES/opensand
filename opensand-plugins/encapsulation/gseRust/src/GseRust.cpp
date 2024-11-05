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
 * @file Gse.cpp
 * @brief GSE encapsulation plugin implementation
 * @author Axel Pinel <axel.pinel@viveris.fr>
 */

#include "GseRust.h"
#include <NetPacket.h>
#include <NetBurst.h>
#include <OpenSandModelConf.h>
#include <opensand_output/Output.h>
#include <memory>

// constexpr std::size_t GSE_MANDATORY_FIELDS_LENGTH = 2; // fields : Start	(1bit) + End (1bit) +	Label Type (2bits) + packet_Length (12bits)
// constexpr std::size_t GSE_FRAG_ID_LENGTH = 1;
// constexpr std::size_t GSE_TOTAL_LENGTH_LENGTH = 2;
// constexpr std::size_t GSE_PROTOCOL_TYPE_LENGTH = 2;


GseRust::PacketHandler::PacketHandler(EncapPlugin &plugin) : EncapPlugin::EncapPacketHandler(plugin)
{
	this->plugin = dynamic_cast<GseRust *>(&plugin);
}

bool GseRust::PacketHandler::getEncapsulatedPackets(Rt::Ptr<NetContainer> packet,
													bool &UNUSED(partial_decap),
													std::vector<Rt::Ptr<NetPacket>> &decap_packets,
													unsigned int decap_packets_count)
{
	return this->plugin->decapAllPackets(std::move(packet), decap_packets, decap_packets_count);
}


void GseRust::Context::setFilterTalId(uint8_t tal_id){
	this->plugin->setFilterTalId(tal_id);
}

bool GseRust::PacketHandler::getChunk(Rt::Ptr<NetPacket> packet,
									  std::size_t remaining_length,
									  Rt::Ptr<NetPacket> &data,
									  Rt::Ptr<NetPacket> &remaining_data)

{
	return this->plugin->encapNextPacket(std::move(packet), remaining_length, 0, data, remaining_data);
}

// CONSTRUCTORS
GseRust::GseRust() : OpenSandPlugin(), EncapPlugin(NET_PROTO::GSE), SimpleGseRust()
{
	// a laisser
	this->upper.push_back("ROHC");
	this->upper.push_back("Ethernet");
}

Rt::Ptr<NetBurst> GseRust::Context::deencapsulate(Rt::Ptr<NetBurst> burst)
{
	// Rt::Ptr<NetBurst> empty_burst = Rt::make_ptr<NetBurst>();
	// return empty_burst;
	return burst;
}

GseRust::~GseRust() {}

GseRust::PacketHandler::~PacketHandler()
{
}

void GseRust::generateConfiguration(const std::string &parent_path, const std::string &param_id, const std::string &plugin_name)
{
	return SimpleGseRust::generateConfiguration(parent_path,param_id,plugin_name);
}

// Static methods, pure virtual from EncapPlugin and SimpleEncapPlugin
bool GseRust::setLabel(const NetPacket &packet, uint8_t label[])
{
	return SimpleGseRust::setLabel(packet, label);
}

uint8_t GseRust::getSrcTalIdFromLabel(const uint8_t label[])
{
	return SimpleGseRust::getSrcTalIdFromFragId(*label);
}

uint8_t GseRust::getDstTalIdFromLabel(const uint8_t label[])
{
	return SimpleGseRust::getDstTalIdFromLabel(label);
}

uint8_t GseRust::getQosFromLabel(const uint8_t label[])
{
	return SimpleGseRust::getQosFromLabel(label);
}

uint8_t GseRust::getFragId(const NetPacket &packet)
{
	return SimpleGseRust::getFragId(packet);
}

uint8_t GseRust::getSrcTalIdFromFragId(const uint8_t frag_id)
{
	return SimpleGseRust::getSrcTalIdFromFragId(frag_id);
}

uint8_t GseRust::getQosFromFragId(const uint8_t frag_id)
{
	return SimpleGseRust::getQosFromFragId(frag_id);
}

bool GseRust::init(void)
{
	if (!EncapPlugin::init())
	{
		return false;
	}

	if (!SimpleGseRust::init())
	{
		return false;
	}
	return true;
}

// ------------------------------------------------- USELESS BUT CALLED METHODS -------------------------------------------------
// THESE METHODS DON'T ANYTHING BUT THEY MUST EXIST AND ARE CALLED
bool GseRust::Context ::init(void)
{
	if (!EncapPlugin::EncapContext::init())
	{
		return false;
	}
	return true;
}

// this method must not be called
bool GseRust::PacketHandler::getHeaderExtensions(const Rt::Ptr<NetPacket> &packet,
												 std::string callback_name,
												 void *opaque)
{
	return this->plugin->getHeaderExtensions(packet,callback_name,opaque);
}

// useless, should be deleted soon
Rt::Ptr<NetBurst> GseRust::Context::encapsulate(Rt::Ptr<NetBurst> burst, std::map<long, int> &UNUSED(time_contexts))
{
	// Rt::Ptr<NetBurst> empty_burst = Rt::make_ptr<NetBurst>();
	// return empty_burst;
	return burst;
}

GseRust::Context::~Context() {}

GseRust::Context::Context(EncapPlugin &plugin) : EncapPlugin::EncapContext(plugin)
{
	this->plugin = dynamic_cast<GseRust *>(&plugin);
}
// ------------------------------------------------ END USELESS BUT CALLED METHODS ------------------------------------------------

// ------------------------------------------------- USELESS AND NOT CALLED METHOD -------------------------------------------------
// these methods must not be called. If they are, stop the process

// this method must not be called
Rt::Ptr<NetBurst> GseRust::Context::flush(int context_id)
{
	LOG(this->log, LEVEL_ERROR,
		"The %s flush method should never be called\n",
		this->getName().c_str());
	assert(false);
}

// this method must not be called
Rt::Ptr<NetBurst> GseRust::Context::flushAll()
{
	LOG(this->log, LEVEL_ERROR,
		"The %s flushAll method should never be called\n",
		this->getName().c_str());
	assert(false);
}

// this method must not be called
bool GseRust::PacketHandler::getSrc(const Rt::Data &data, tal_id_t &tal_id) const
{
	return this->plugin->getSrc(data, tal_id);
}

// this method must not be called
bool GseRust::PacketHandler::getDst(const Rt::Data &data, tal_id_t &tal_id) const
{

	LOG(this->log, LEVEL_ERROR,
		"The %s getDst method should never be called\n",
		this->getName().c_str());

	return true;
}

// this method must not be called
bool GseRust::PacketHandler::getQos(const Rt::Data &data, qos_t &qos) const
{
	LOG(this->log, LEVEL_ERROR,
		"The %s getQos method should never be called\n",
		this->getName().c_str());

	assert(false);
	return true;
}

// this method must not be called
bool GseRust::PacketHandler::checkPacketForHeaderExtensions(Rt::Ptr<NetPacket> &packet)
{
	LOG(this->log, LEVEL_WARNING,
		"The %s checkPacketForHeaderExtensions called and return true but didn't check for extensions\n",
		this->getName().c_str());
	return true;
}

// this method must not be called
bool GseRust::PacketHandler::setHeaderExtensions(Rt::Ptr<NetPacket> packet,
												 Rt::Ptr<NetPacket> &new_packet,
												 tal_id_t tal_id_src,
												 tal_id_t tal_id_dst,
												 std::string callback_name,
												 void *opaque)
{
	return this->plugin->setHeaderExtensions(std::move(packet),new_packet,tal_id_src,tal_id_dst,callback_name,opaque);
}

// this method must not be called
Rt::Ptr<NetPacket> GseRust::PacketHandler::build(const Rt::Data &data,
												 size_t data_length,
												 uint8_t qos,
												 uint8_t src_tal_id,
												 uint8_t dst_tal_id)
{
	LOG(this->log, LEVEL_ERROR,
		"The %s build method should never be called. Aborting",
		this->getName().c_str());
	assert(false);
	return Rt::make_ptr<NetPacket>(nullptr);
}

// this method must not be called
size_t GseRust::PacketHandler::getLength(unsigned char const *) const
{
	LOG(this->log, LEVEL_ERROR,
		"The %s getLength method should never be called. Aborting",
		this->getName().c_str());
	assert(false);
	return 0;
}
// ------------------------------------------------ END USELESS AND NOT CALLED METHOD ------------------------------------------------
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
 * @brief RLE encapsulation plugin implementation
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include <algorithm>
#include <cstring>
#include "SimpleRle.h"
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

Rle::Rle() : OpenSandPlugin(), EncapPlugin(NET_PROTO::RLE), SimpleRle()

{
	this->upper.push_back("ROHC");
	this->upper.push_back("Ethernet");
}

Rle::~Rle()
{
}

// pas touche
void Rle::generateConfiguration(const std::string &parent_path, const std::string &param_id, const std::string &plugin_name)
{
	return SimpleRle::generateConfiguration(parent_path, param_id, plugin_name);

}

bool Rle::init()
{
	if (!EncapPlugin::init())
	{
		return false;
	}
	if (!SimpleRle::init())
	{
		return false;
	}
	return true;
}

Rle::Context::Context(Rle &plugin) : EncapPlugin::EncapContext(plugin)
{
	this->plugin = dynamic_cast<Rle *>(&plugin);
	assert(this->plugin != nullptr && "dynamic cast on plugin failed");
}

Rle::Context::~Context()
{
}

bool Rle::Context::init()
{
	if (!EncapPlugin::EncapContext::init())
	{
		return false;
	}
	return true;
}

Rt::Ptr<NetBurst> Rle::Context::encapsulate(Rt::Ptr<NetBurst> burst, std::map<long, int> &)
{
	LOG(this->log, LEVEL_INFO, "Rle::Context::encapsulate called but not doing anything as expected", this->getName().c_str());
	return burst;
}

Rt::Ptr<NetBurst> Rle::Context::deencapsulate(Rt::Ptr<NetBurst> burst)
{
	return burst;
	
}

Rt::Ptr<NetBurst> Rle::Context::flush(int)
{
	LOG(this->log, LEVEL_INFO, "Rle::Context::flush called but not doing anything as expected", this->getName().c_str());
	return Rt::make_ptr<NetBurst>(nullptr);
}

Rt::Ptr<NetBurst> Rle::Context::flushAll()
{
	LOG(this->log, LEVEL_INFO, "Rle::Context::flushAll called but not doing anything as expected", this->getName().c_str());
	return Rt::make_ptr<NetBurst>(nullptr);
}

Rle::PacketHandler::PacketHandler(EncapPlugin &plugin) : EncapPlugin::EncapPacketHandler(plugin)
{
	this->plugin = dynamic_cast<Rle *>(&plugin);
	assert(this->plugin != nullptr && "dynamic cast on plugin failed");
}

Rle::PacketHandler::~PacketHandler()
{
}

bool Rle::PacketHandler::init()
{
	return true;
}

Rt::Ptr<NetPacket> Rle::PacketHandler::build(const Rt::Data &data,
											 std::size_t data_length,
											 uint8_t qos,
											 uint8_t src_tal_id,
											 uint8_t dst_tal_id)
{
	return this->plugin->build(data, data_length, qos, src_tal_id, dst_tal_id);
}

//USELESS
std::size_t Rle::PacketHandler::getLength(const unsigned char *) const
{
	LOG(this->log, LEVEL_ERROR,
		"The %s getLength method will never be called\n",
		this->getName().c_str());
	return 0;
}

bool Rle::PacketHandler::encapNextPacket(Rt::Ptr<NetPacket> packet,
										 std::size_t remaining_length,
										 bool new_burst,
										 Rt::Ptr<NetPacket> &encap_packet,
										 Rt::Ptr<NetPacket> &remaining_data)
{
	return this->plugin->encapNextPacket(std::move(packet), remaining_length, new_burst, encap_packet, remaining_data);
}

bool Rle::PacketHandler::getEncapsulatedPackets(Rt::Ptr<NetContainer> packet,
												bool &partial_decap,
												std::vector<Rt::Ptr<NetPacket>> &decap_packets,
												unsigned int)
{
	partial_decap = false; 	//UNUSED parameter
	return this->plugin->decapAllPackets(std::move(packet),decap_packets,0);	;
}

bool Rle::PacketHandler::getChunk(Rt::Ptr<NetPacket>,
								  std::size_t,
								  Rt::Ptr<NetPacket> &,
								  Rt::Ptr<NetPacket> &)
{
	LOG(this->log, LEVEL_INFO,
		"The %s getChunk called as expected but empty function\n",
		this->getName().c_str());
	return true;
}

bool Rle::PacketHandler::getSrc(const Rt::Data &data, tal_id_t &tal_id) const
{
	return this->plugin->getSrc(data, tal_id);
}

bool Rle::PacketHandler::getDst(const Rt::Data &data, tal_id_t &tal_id) const
{
	return this->plugin->getDst(data, tal_id);
}

bool Rle::PacketHandler::getQos(const Rt::Data &data, qos_t &qos) const
{
	return this->plugin->getQos(data, qos);
}

bool Rle::PacketHandler::checkPacketForHeaderExtensions(Rt::Ptr<NetPacket> &)
{ // NOT DOING ANYTHING
	assert(false);
	LOG(this->log, LEVEL_ERROR,
		"The %s getPacketForHeaderExtensions method should never be called\n",
		this->getName().c_str());
	return false;
}

bool Rle::PacketHandler::setHeaderExtensions(Rt::Ptr<NetPacket>,
											 Rt::Ptr<NetPacket> &,
											 tal_id_t,
											 tal_id_t,
											 std::string,
											 void *)
{
	// NOT DOING ANYTHING
	assert(false);
	LOG(this->log, LEVEL_ERROR,
		"The %s setHeaderExtensions method should never be called\n",
		this->getName().c_str());
	return false;
}

bool Rle::PacketHandler::getHeaderExtensions(const Rt::Ptr<NetPacket> &,
											 std::string,
											 void *)
{
	assert(false);
	LOG(this->log, LEVEL_ERROR,
		"The %s getHeaderExtensions method should never be called\n",
		this->getName().c_str());
	return false;
}

// Static methods

bool Rle::getLabel(const NetPacket &packet, uint8_t label[])
{
	return SimpleRle::getLabel(packet, label);
}

bool Rle::getLabel(const Rt::Data &data, uint8_t label[])
{
	return SimpleRle::getLabel(data, label);
}

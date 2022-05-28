/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
 * Copyright © 2019 TAS
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
 * @file EncapPlugin.cpp
 * @brief Generic encapsulation / deencapsulation plugin
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "EncapPlugin.h"
#include "NetBurst.h"
#include "NetContainer.h"
#include "NetPacket.h"

#include <opensand_output/Output.h>

#include <cassert>


EncapPlugin::EncapPlugin(uint16_t ether_type):
		StackPlugin(ether_type)
{
}

bool EncapPlugin::init()
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING,
	                                       "Encap.%s",
	                                       this->getName().c_str());
	return true;
}


EncapPlugin::EncapContext::EncapContext(EncapPlugin &pl):
		StackContext(pl)
{
	this->dst_tal_id = BROADCAST_TAL_ID;
}

void EncapPlugin::EncapContext::setFilterTalId(uint8_t tal_id)
{
	this->dst_tal_id = tal_id;
}

bool EncapPlugin::EncapContext::init()
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING,
	                                       "Encap.%s",
	                                       this->getName().c_str());
	return true;
}


EncapPlugin::EncapPacketHandler::EncapPacketHandler(EncapPlugin &pl):
		StackPacketHandler(pl)
{
}

EncapPlugin::EncapPacketHandler::~EncapPacketHandler()
{
	for(auto&& it : this->encap_packets)
	{
		delete it.first;
	}
	this->encap_packets.clear();
}

bool EncapPlugin::EncapPacketHandler::init()
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING,
	                                       "Encap.%s",
	                                       this->getName().c_str());
	return true;
}

/*
std::list<std::string> EncapPlugin::EncapPacketHandler::getCallback()
{
	return this->callback_name;
}
*/

bool EncapPlugin::EncapPacketHandler::encapNextPacket(NetPacket *packet,
                                                      std::size_t remaining_length,
                                                      bool UNUSED(new_burst),
                                                      bool &partial_encap,
                                                      NetPacket **encap_packet)
{
	// Set default returned values
	partial_encap = false;
	*encap_packet = nullptr;

	// Check there is a previous encapsulation of the packet
  std::unique_ptr<NetPacket> packet_to_encap;
	auto it = this->encap_packets.find(packet);
	if(it == this->encap_packets.end())
	{
		packet_to_encap = std::unique_ptr<NetPacket>{new NetPacket(packet)};
	}
	else
	{
		packet_to_encap = std::move(it->second);
	}

	// get the part of the packet to send
  std::unique_ptr<NetPacket> data;
  std::unique_ptr<NetPacket> remaining_data;
	bool success = this->getChunk(std::move(packet_to_encap),
                                remaining_length,
                                data, remaining_data);
	if(!success || (!data && !remaining_data))
	{
		return false;
	}

	// Set the returned encap packet
	if(data)
	{
		*encap_packet = data.release();
	}

	// Check the remaining data
	if(remaining_data)
	{
		partial_encap = true;
		if(it == this->encap_packets.end())
		{
			// Insert the remaining data
			this->encap_packets.insert({packet, std::move(remaining_data)});
		}
		else
		{
			// Modify the remaining data
			it->second = std::move(remaining_data);
		}
	}
	else if(it != this->encap_packets.end())
	{
		// Remove the remaining data
    delete it->first;
		this->encap_packets.erase(it);
	}

	return true;
}

bool EncapPlugin::EncapPacketHandler::getEncapsulatedPackets(NetContainer *packet,
                                                             bool &partial_decap,
                                                             std::vector<std::unique_ptr<NetPacket>> &decap_packets,
                                                             unsigned int decap_packets_count)
{
	std::vector<std::unique_ptr<NetPacket>> packets{};
	std::size_t previous_length = 0;

	// Set the default returned values
	partial_decap = false;

	// Sanity check
	if(decap_packets_count <= 0)
	{
		decap_packets = std::move(packets);
		LOG(this->log, LEVEL_INFO,
			"No packet to decapsulate\n");
		return true;
	}

	LOG(this->log, LEVEL_DEBUG,
		"%u packet(s) to decapsulate\n",
		decap_packets_count);
	for(unsigned int i = 0; i < decap_packets_count; ++i)
	{
		// Get the current packet length
    std::size_t current_length = this->getLength(packet->getPayload(previous_length).c_str());
		if(current_length <= 0)
		{
			LOG(this->log, LEVEL_ERROR,
				"cannot create one %s packet (no data)\n",
				this->getName().c_str(), current_length);
      return false;
		}

		// Get the current packet
    std::unique_ptr<NetPacket> current;
    try
    {
      current = this->build(packet->getPayload(previous_length), current_length,
                            0x00, BROADCAST_TAL_ID, BROADCAST_TAL_ID);
    }
    catch (const std::bad_alloc&)
		{
			LOG(this->log, LEVEL_ERROR,
				"cannot create one %s packet (length = %zu bytes)\n",
				this->getName().c_str(), current_length);
      return false;
		}

		// Add the current packet to decapsulated packets
		packets.push_back(std::move(current));
		previous_length += current_length;
	}

	// Set returned decapsulated packets
	decap_packets = std::move(packets);
	return true;
}

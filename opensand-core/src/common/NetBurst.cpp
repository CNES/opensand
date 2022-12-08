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
 * @file NetBurst.cpp
 * @brief Generic network burst
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <numeric>

#include <opensand_output/Output.h>
#include <opensand_rt/Types.h>

#include "NetBurst.h"
#include "NetPacket.h"


std::shared_ptr<OutputLog> NetBurst::log_net_burst = nullptr;


// max_packets = 0 => unlimited length
NetBurst::NetBurst(unsigned int max_packets): std::list<std::unique_ptr<NetPacket>>()
{
	this->max_packets = max_packets;

	LOG(log_net_burst, LEVEL_INFO,
	    "burst created (max length = %d)\n",
	    this->max_packets);
}


NetBurst::~NetBurst()
{
	this->clear();
}


unsigned int NetBurst::getMaxPackets() const
{
	return this->max_packets;
}


// max_packets = 0 => unlimited length
void NetBurst::setMaxPackets(unsigned int max_packets)
{
	this->max_packets = max_packets;
}


bool NetBurst::add(std::unique_ptr<NetPacket> packet)
{
	bool success = true;

	if(this->isFull() || packet == nullptr)
	{
		LOG(log_net_burst, LEVEL_INFO,
		    "cannot add packet to burst (%d/%d)\n",
		    this->length(), this->max_packets);
		success = false;
	}
	else
	{
		this->push_back(std::move(packet));
		LOG(log_net_burst, LEVEL_INFO,
		    "packet added to burst (%d/%d)\n",
		    this->length(), this->max_packets);
	}

	return success;
}


bool NetBurst::isFull() const
{
	// max_packets = 0 => unlimited length
	return this->max_packets != 0 && this->length() >= this->max_packets;
}


unsigned int NetBurst::length() const
{
	return this->size();
}


Rt::Data NetBurst::data() const
{
	Rt::Data data;

	// add the data of each network packet of the burst
	for(auto&& packet : *this)
	{
		data.append(packet->getData());
	}

	return data;
}


long NetBurst::bytes() const
{
	return std::accumulate(this->begin(), this->end(), 0L,
	                       [](long length, const std::unique_ptr<NetPacket>& packet){
	                          return length + packet->getTotalLength();
	                       });
}


NET_PROTO NetBurst::type() const
{
	if(!this->size())
	{
		// no packet in the burst, impossible to get the packet type
		LOG(log_net_burst, LEVEL_ERROR,
		    "failed to determine the burst type: "
		    "burst is empty\n");
		return NET_PROTO::ERROR;
	}
	else
	{
		// type of first packet in burst
		return this->front()->getType();
	}
}


std::string NetBurst::name() const
{
	if(!this->size())
	{
		// no packet in the burst, impossible to get the packet name
		return std::string{"unknown"};
	}
	else
	{
		// name of first packet in burst
		return this->front()->getName();
	}
}

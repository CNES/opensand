/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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

#include "NetBurst.h"

#include <opensand_output/Output.h>

OutputLog *NetBurst::log_net_burst = NULL;

// max_packets = 0 => unlimited length
NetBurst::NetBurst(unsigned int max_packets): list<NetPacket *>()
{
	this->max_packets = max_packets;

	LOG(log_net_burst, LEVEL_INFO,
	    "burst created (max length = %d)\n",
	    this->max_packets);
}

NetBurst::~NetBurst()
{
	list < NetPacket * >::iterator it;

	for(it = this->begin(); it != this->end(); it++)
	{
		if(*it != NULL)
			delete *it;
	}
}

int NetBurst::getMaxPackets()
{
	return this->max_packets;
}

// max_packets = 0 => unlimited length
void NetBurst::setMaxPackets(unsigned int max_packets)
{
	this->max_packets = max_packets;
}

bool NetBurst::add(NetPacket *packet)
{
	bool success = true;

	if(this->isFull() || packet == NULL)
	{
		LOG(log_net_burst, LEVEL_INFO,
		    "cannot add packet to burst (%d/%d)\n",
		    this->length(), this->max_packets);
		success = false;
	}
	else
	{
		this->push_back(packet);
		LOG(log_net_burst, LEVEL_INFO,
		    "packet added to burst (%d/%d)\n",
		    this->length(), this->max_packets);
	}

	return success;
}

bool NetBurst::isFull()
{
	// max_packets = 0 => unlimited length
	return (this->max_packets != 0 && this->length() >= this->max_packets);
}

unsigned int NetBurst::length()
{
	return this->size();
}

Data NetBurst::data()
{
	Data data;
	list<NetPacket *>::iterator it;

	// add the data of each network packet of the burst
	for(it = this->begin(); it != this->end(); it++)
	{
		data.append((*it)->getData());
	}

	return data;
}

long NetBurst::bytes()
{
	long len = 0;

	if(this->length() > 0)
	{
		list<NetPacket *>::iterator it;

		for(it = this->begin(); it != this->end(); it++)
		{
			len += (*it)->getTotalLength();
		}
	}

	return len;
}

uint16_t NetBurst::type()
{
	if(this->length() <= 0)
	{
		// no packet in the burst, impossible to get the packet type
		LOG(log_net_burst, LEVEL_ERROR,
		    "failed to determine the burst type: "
		    "burst is empty\n");
		return NET_PROTO_ERROR;
	}
	else
	{
		// type of first packet in burst
		return this->front()->getType();
	}
}

string NetBurst::name()
{
	if(this->length() <= 0)
	{
		// no packet in the burst, impossible to get the packet type
		return string("unknown");
	}
	else
	{
		// type of first packet in burst
		return (*(this->begin()))->getName();
	}
}

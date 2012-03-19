/**
 * @file NetBurst.cpp
 * @brief Generic network burst
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "NetBurst.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


// max_packets = 0 => unlimited length
NetBurst::NetBurst(unsigned int max_packets): std::list<NetPacket *>()
{
	this->max_packets = max_packets;

	UTI_DEBUG("[NetBurst::NetBurst] burst created (max length = %d)\n",
	          this->max_packets);
}

NetBurst::~NetBurst()
{
	std::list < NetPacket * >::iterator it;

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
		UTI_DEBUG("[NetBurst::add] cannot add packet to burst (%d/%d)\n",
		          this->length(), this->max_packets);
		success = false;
	}
	else
	{
		this->push_back(packet);
		UTI_DEBUG("[NetBurst::add] packet added to burst (%d/%d)\n",
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
	std::list<NetPacket *>::iterator it;

	// add the data of each network packet of the burst
	for(it = this->begin(); it != this->end(); it++)
	{
		data.append((*it)->data());
	}

	return data;
}

long NetBurst::bytes()
{
	long len = 0;

	if(this->length() > 0)
	{
		std::list<NetPacket *>::iterator it;

		for(it = this->begin(); it != this->end(); it++)
		{
			len += (*it)->totalLength();
		}
	}

	return len;
}

uint16_t NetBurst::type()
{
	if(this->length() <= 0)
	{
		// no packet in the burst, impossible to get the packet type
		UTI_ERROR("failed to determine the burst type: "
		          "burst is empty\n");
		return 0;
	}
	else
	{
		// type of first packet in burst
		return this->front()->type();
	}
}

std::string NetBurst::name()
{
	if(this->length() <= 0)
	{
		// no packet in the burst, impossible to get the packet type
		return std::string("unknown");
	}
	else
	{
		// type of first packet in burst
		return (*(this->begin()))->name();
	}
}


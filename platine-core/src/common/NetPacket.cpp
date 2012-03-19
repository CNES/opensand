/**
 * @file NetPacket.cpp
 * @brief Network-layer packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "NetPacket.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


mgl_memory_pool NetPacket::mempool(230, 100000, "net_packet");


NetPacket::NetPacket(Data data): _data(data), _name("unknown")
{
	this->_type = 0;
}

NetPacket::NetPacket(unsigned char *data, unsigned int length):
	_data(), _name("unknown")
{
	_data.append(data, length);

	this->_type = 0;
}

NetPacket::NetPacket(): _data(), _name("unknown")
{
	this->_type = 0;
}

NetPacket::~NetPacket()
{
}

void NetPacket::addTrace(std::string name_function)
{
	mempool.add_function(name_function, (char *) this);
}

std::string NetPacket::name()
{
	return this->_name;
}

uint16_t NetPacket::type()
{
	return this->_type;
}

Data NetPacket::data()
{
	if(!this->isValid())
	{
		UTI_ERROR("invalid packet\n");
		return Data();
	}

	return this->_data;
}

void NetPacket::setType(uint16_t type)
{
	return;
}


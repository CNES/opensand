/**
 * @file RohcPacket.cpp
 * @brief ROHC packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "RohcPacket.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


RohcPacket::RohcPacket(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "ROHC";
	this->_type = NET_PROTO_ROHC;
	this->_data.reserve(1500);
}

RohcPacket::RohcPacket(Data data): NetPacket(data)
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "ROHC";
	this->_type = NET_PROTO_ROHC;
	this->_data.reserve(1500);
}

RohcPacket::RohcPacket(): NetPacket()
{
	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_name = "ROHC";
	this->_type = NET_PROTO_ROHC;
	this->_data.reserve(1500);
}

RohcPacket::~RohcPacket()
{
}

int RohcPacket::qos()
{
	return this->_qos;
}

void RohcPacket::setQos(int qos)
{
	this->_qos = qos;
}

unsigned long RohcPacket::macId()
{
	return this->_macId;
}

void RohcPacket::setMacId(unsigned long macId)
{
	this->_macId = macId;
}

long RohcPacket::talId()
{
	return this->_talId;
}

void RohcPacket::setTalId(long talId)
{
	this->_talId = talId;
}

bool RohcPacket::isValid()
{
	// always return true, rohc library will test packet validity
	return true;
}

uint16_t RohcPacket::totalLength()
{
	return this->_data.length();
}

uint16_t RohcPacket::payloadLength()
{
	return this->totalLength();
}

Data RohcPacket::payload()
{
	return this->_data;
}

// static
NetPacket * RohcPacket::create(Data data)
{
	return new RohcPacket(data);
}

void RohcPacket::setType(uint16_t type)
{
	this->_type = type;
}

/**
 * @file IpPacket.cpp
 * @brief Generic IP packet, either IPv4 or IPv6
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "IpPacket.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


IpPacket::IpPacket(Data data): NetPacket(data)
{
	this->_data.reserve(1500);

	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_srcAddr = NULL;
	this->_destAddr = NULL;
}

IpPacket::IpPacket(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->_data.reserve(1500);

	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_srcAddr = NULL;
	this->_destAddr = NULL;
}

IpPacket::IpPacket(): NetPacket()
{
	this->_data.reserve(1500);

	this->_qos = -1;
	this->_talId = -1;
	this->_macId = 0;

	this->_srcAddr = NULL;
	this->_destAddr = NULL;
}

IpPacket::~IpPacket()
{
	if(this->_srcAddr != NULL)
		delete this->_srcAddr;
	if(this->_destAddr != NULL)
		delete this->_destAddr;
}

int IpPacket::qos()
{
	return this->_qos;
}

void IpPacket::setQos(int qos)
{
	this->_qos = qos;
}

unsigned long IpPacket::macId()
{
	return this->_macId;
}

void IpPacket::setMacId(unsigned long macId)
{
	this->_macId = macId;
}

long IpPacket::talId()
{
	return this->_talId;
}

void IpPacket::setTalId(long talId)
{
	this->_talId = talId;
}

Data IpPacket::payload()
{
	uint16_t payload_len, header_len;

	if(!this->isValid())
	{
		UTI_ERROR("[IpPacket::payload] invalid IP packet\n");
		return Data();
	}

	payload_len = this->payloadLength();
	header_len = this->totalLength() - payload_len;

	if(header_len <= 0 || payload_len <= 0)
	{
		UTI_ERROR("[IpPacket::payload] IP packet has a 0 length payload\n");
		return Data();
	}

	return this->_data.substr(header_len, payload_len);
}

// static
int IpPacket::version(Data data)
{
	if(data.length() < 4 * 5)
	{
		UTI_ERROR("[IpPacket::version(data)] invalid IP packet\n");
		return 0;
	}

	return ((data.at(0) & 0xf0) >> 4);
}

// static
int IpPacket::version(unsigned char *data, unsigned int length)
{
	if(length < 4 * 5)
	{
		UTI_ERROR("[IpPacket::version(data, length)] invalid IP packet\n");
		return 0;
	}

	return ((data[0] & 0xf0) >> 4);
}

int IpPacket::version()
{
	if(!this->isValid())
	{
		UTI_ERROR("[IpPacket::version] invalid IP packet\n");
		return 0;
	}

	return IpPacket::version(this->_data);
}


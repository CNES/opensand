/**
 * @file Ipv4Packet.cpp
 * @brief IPv4 packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "Ipv4Packet.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


Ipv4Packet::Ipv4Packet(Data data): IpPacket(data)
{
	this->_name = "IPv4";
	this->_type = NET_PROTO_IPV4;

	this->validityChecked = false;
	this->validityResult = false;
}

Ipv4Packet::Ipv4Packet(unsigned char *data,
                       unsigned int length):
	IpPacket(data, length)
{
	this->_name = "IPv4";
	this->_type = NET_PROTO_IPV4;

	this->validityChecked = false;
	this->validityResult = false;
}

Ipv4Packet::Ipv4Packet(): IpPacket()
{
	this->_name = "IPv4";
	this->_type = NET_PROTO_IPV4;

	this->validityChecked = false;
	this->validityResult = false;
}

Ipv4Packet::~Ipv4Packet()
{
}

bool Ipv4Packet::isValid()
{
	bool is_valid = false;
	uint16_t crc;
	uint16_t cur_crc;

	if(this->validityChecked)
		goto skip;

	// IPv4 packet length must be at least 20 byte long
	if(this->_data.length() < 4 * 5)
		goto invalid;

	// calculate the CRC
	crc = this->calcCrc();
	cur_crc = this->crc();

	if(crc != cur_crc)
	{
		UTI_DEBUG("[Ipv4Packet::isValid] CRC = %08x, should be %08x\n",
		          cur_crc, crc);
		goto invalid;
	}

	is_valid = true;

invalid:
	// cache the result
	this->validityResult = is_valid;
	this->validityChecked = true;

skip:
	return this->validityResult;
}

uint16_t Ipv4Packet::calcCrc()
{
	size_t nbytes;
	uint16_t *data;
	uint32_t sum;

	data = (uint16_t *) this->_data.c_str();
	nbytes = this->ihl() * 4;
	sum = 0;

	while(nbytes >= sizeof(uint16_t))
	{
		sum += *(data++);
		nbytes -= sizeof(uint16_t);
	}
	// Header Checksum field must be 0 for checksum,
	sum -= this->crc();

	sum = (sum >> 16) + (sum & 0xffff);
	sum = (sum >> 16) + (sum & 0xffff);

	return ((uint16_t) ~ sum);
}

uint16_t Ipv4Packet::crc()
{
	if(this->_data.length() < 4 * 5)
	{
		UTI_ERROR("[Ipv4Packet::crc] invalid IPv4 packet\n");
		return 0;
	}

	return (uint16_t) (((this->_data.at(10) & 0xff) << 8)
	                 + ((this->_data.at(11) & 0xff) << 0));
}

uint16_t Ipv4Packet::totalLength()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::totalLength] invalid IPv4 packet\n");
		return 0;
	}

	return (uint16_t) (((this->_data.at(2) & 0xff) << 8)
	                 + ((this->_data.at(3) & 0xff) << 0));
}

uint8_t Ipv4Packet::ihl()
{
	if(this->_data.length() < 4 * 5)
	{
		UTI_ERROR("[Ipv4Packet::ihl] invalid IPv4 packet\n");
		return 0;
	}

	return (uint8_t) (this->_data.at(0) & 0x0f);
}

uint16_t Ipv4Packet::payloadLength()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::payloadLength] invalid IPv4 packet\n");
		return 0;
	}

	return (uint16_t) (this->totalLength() - this->ihl() * 4);
}

IpAddress * Ipv4Packet::srcAddr()
{
	if(this->_srcAddr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv4Packet::srcAddr] invalid IPv4 packet\n");
			return NULL;
		}

		this->_srcAddr =
			new Ipv4Address(this->_data.at(12), this->_data.at(13),
			                this->_data.at(14), this->_data.at(15));
	}

	return this->_srcAddr;
}

IpAddress * Ipv4Packet::destAddr()
{
	if(this->_destAddr == NULL)
	{
		if(!this->isValid())
		{
			UTI_ERROR("[Ipv4Packet::destAddr] invalid IPv4 packet\n");
			return NULL;
		}

		this->_destAddr =
			new Ipv4Address(this->_data.at(16), this->_data.at(17),
			                this->_data.at(18), this->_data.at(19));
	}

	return this->_destAddr;
}

uint8_t Ipv4Packet::trafficClass()
{
	if(!this->isValid())
	{
		UTI_ERROR("[Ipv4Packet::trafficClass] invalid IPv4 packet\n");
		return 0;
	}

	return (uint8_t) this->_data.at(1);
}

// static
NetPacket * Ipv4Packet::create(Data data)
{
	return new Ipv4Packet(data);
}


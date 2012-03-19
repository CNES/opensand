/**
 * @file DvbFrame.cpp
 * @brief DVB frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "DvbFrame.h"
#include "lib_dvb_rcs.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


DvbFrame::DvbFrame(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->_name = "unknown DVB frame";
	this->_type = NET_PROTO_DVB_FRAME;
	this->max_size = 0;
	this->num_packets = 0;
	this->carrier_id = 0;
}

DvbFrame::DvbFrame(Data data):
	NetPacket(data)
{
	this->_name = "unknown DVB frame";
	this->_type = NET_PROTO_DVB_FRAME;
	this->max_size = 0;
	this->num_packets = 0;
	this->carrier_id = 0;
}

DvbFrame::DvbFrame(DvbFrame *frame):
	NetPacket(frame->data())
{
	this->_name = frame->name();
	this->_type = frame->type();
	this->max_size = frame->getMaxSize();
	this->num_packets = frame->getNumPackets();
	this->carrier_id = 0;
}

DvbFrame::DvbFrame():
	NetPacket()
{
	this->_name = "unknown DVB frame";
	this->_type = NET_PROTO_DVB_FRAME;
	this->max_size = 0;
	this->num_packets = 0;
	this->carrier_id = 0;
}

DvbFrame::~DvbFrame()
{
}

int DvbFrame::qos()
{
	return 0;
}

void DvbFrame::setQos(int qos)
{
}

unsigned long DvbFrame::macId()
{
	return 0;
}

void DvbFrame::setMacId(unsigned long macId)
{
}

long DvbFrame::talId()
{
	return 0;
}

void DvbFrame::setTalId(long talId)
{
}

bool DvbFrame::isValid()
{
	return true;
}

uint16_t DvbFrame::totalLength()
{
	return this->_data.length();
}

unsigned int DvbFrame::getMaxSize(void)
{
	return this->max_size;
}

void DvbFrame::setMaxSize(unsigned int size)
{
	this->max_size = size;
}

void DvbFrame::setCarrierId(long carrier_id)
{
	this->carrier_id = carrier_id;
}

unsigned int DvbFrame::getFreeSpace(void)
{
	return (this->max_size - this->totalLength());
}

bool DvbFrame::addPacket(NetPacket *packet)
{
	// is the frame large enough to contain the packet ?
	if(packet->totalLength() > this->getFreeSpace())
	{
		// too few free space in the frame
		return false;
	}

	this->_data.append(packet->data());
	this->num_packets++;

	return true;
}

unsigned int DvbFrame::getNumPackets(void)
{
	return this->num_packets;
}


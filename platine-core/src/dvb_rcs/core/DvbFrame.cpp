/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file DvbFrame.cpp
 * @brief DVB frame
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "DvbFrame.h"
#include "lib_dvb_rcs.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


DvbFrame::DvbFrame(unsigned char *data, unsigned int length):
	NetPacket(data, length)
{
	this->name = "unknown DVB frame";
	this->type = NET_PROTO_DVB_FRAME;
	this->max_size = 0;
	this->num_packets = 0;
	this->carrier_id = 0;
}

DvbFrame::DvbFrame(Data data):
	NetPacket(data)
{
	this->name = "unknown DVB frame";
	this->type = NET_PROTO_DVB_FRAME;
	this->max_size = 0;
	this->num_packets = 0;
	this->carrier_id = 0;
}

DvbFrame::DvbFrame(DvbFrame *frame):
	NetPacket(frame->getData())
{
	this->name = frame->getName();
	this->type = frame->getType();
	this->max_size = frame->getMaxSize();
	this->num_packets = frame->getNumPackets();
	this->carrier_id = 0;
}

DvbFrame::DvbFrame():
	NetPacket()
{
	this->name = "unknown DVB frame";
	this->type = NET_PROTO_DVB_FRAME;
	this->max_size = 0;
	this->num_packets = 0;
	this->carrier_id = 0;
}

DvbFrame::~DvbFrame()
{
}

uint16_t DvbFrame::getTotalLength()
{
	return this->data.length();
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
	return (this->max_size - this->getTotalLength());
}

bool DvbFrame::addPacket(NetPacket *packet)
{
	// is the frame large enough to contain the packet ?
	if(packet->getTotalLength() > this->getFreeSpace())
	{
		// too few free space in the frame
		return false;
	}

	this->data.append(packet->getData());
	this->num_packets++;

	return true;
}

unsigned int DvbFrame::getNumPackets(void)
{
	return this->num_packets;
}

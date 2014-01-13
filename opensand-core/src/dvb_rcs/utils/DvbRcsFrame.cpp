/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file DvbRcsFrame.cpp
 * @brief DVB-RCS frame
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "DvbRcsFrame.h"
#include "OpenSandFrames.h"

#define DBG_PACKAGE PKG_DEFAULT
#include <opensand_conf/uti_debug.h>

#include <string.h>


DvbRcsFrame::DvbRcsFrame(const unsigned char *data, size_t length):
	DvbFrameTpl<T_DVB_ENCAP_BURST>(data, length)
{
	this->name = "DVB-RCS frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);
	this->num_packets = ntohl(this->frame()->qty_element);
}

DvbRcsFrame::DvbRcsFrame(const Data &data):
	DvbFrameTpl<T_DVB_ENCAP_BURST>(data)
{
	this->name = "DVB-RCS frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);
	this->num_packets = ntohl(this->frame()->qty_element);
}

DvbRcsFrame::DvbRcsFrame(const Data &data, size_t length):
	DvbFrameTpl<T_DVB_ENCAP_BURST>(data, length)
{
	this->name = "DVB-RCS frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);
	this->num_packets = ntohl(this->frame()->qty_element);
}

DvbRcsFrame::DvbRcsFrame(DvbFrame *frame):
	DvbFrameTpl<T_DVB_ENCAP_BURST>(frame)
{
}

DvbRcsFrame::DvbRcsFrame():
	DvbFrameTpl<T_DVB_ENCAP_BURST>()
{
	this->name = "DVB-RCS frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);

	// no data given as input, so create the DVB-RCS header
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);
	this->setMessageLength(sizeof(T_DVB_ENCAP_BURST));
	this->setMessageType(MSG_TYPE_DVB_BURST);
	this->frame()->qty_element = 0; // no encapsulation packet at the beginning
}

DvbRcsFrame::~DvbRcsFrame()
{
}

bool DvbRcsFrame::addPacket(NetPacket *packet)
{
	bool is_added;

	is_added = DvbFrameTpl<T_DVB_ENCAP_BURST>::addPacket(packet);
	if(is_added)
	{
		this->setMessageLength(this->getMessageLength() + packet->getTotalLength());
		this->frame()->qty_element = htons(this->num_packets);
	}

	return is_added;
}

// TODO not used => remove ?!
void DvbRcsFrame::empty(void)
{
	// remove the payload
	this->data.erase(sizeof(T_DVB_ENCAP_BURST));
	this->num_packets = 0;

	// update the DVB-RCS frame header
	this->setMessageLength(sizeof(T_DVB_ENCAP_BURST));
	this->frame()->qty_element = 0; // no encapsulation packet at the beginning
}


uint16_t DvbRcsFrame::getNumPackets(void) const
{
	return ntohs(this->frame()->qty_element);
}

void DvbRcsFrame::setModcodId(uint8_t modcod_id)
{
	this->frame()->modcod = modcod_id;
}
 
uint8_t DvbRcsFrame::getModcodId(void) const
{
	return this->frame()->modcod;
}

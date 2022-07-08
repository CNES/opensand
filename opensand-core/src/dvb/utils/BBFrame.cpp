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
 * @file BBFrame.cpp
 * @brief BB frame
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "BBFrame.h"
#include "OpenSandFrames.h"

#include <opensand_output/Output.h>

#include <string.h>


std::shared_ptr<OutputLog> BBFrame::bbframe_log = nullptr;


BBFrame::BBFrame(const unsigned char *data, size_t length):
	DvbFrameTpl<T_DVB_BBFRAME>(data, length)
{
	this->name = "BB frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);
	this->num_packets = this->getDataLength();
	this->header_length = this->getOffsetForPayload();
}


BBFrame::BBFrame(const Data &data):
	DvbFrameTpl<T_DVB_BBFRAME>(data)
{
	this->name = "BB frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);
	this->num_packets = this->getDataLength();
	this->header_length = this->getOffsetForPayload();
}


BBFrame::BBFrame(const Data &data, size_t length):
	DvbFrameTpl<T_DVB_BBFRAME>(data, length)
{
	this->name = "BB frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);
	this->num_packets = this->getDataLength();
	this->header_length = this->getOffsetForPayload();
}


BBFrame::BBFrame():
	DvbFrameTpl<T_DVB_BBFRAME>()
{
	this->name = "BB frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);

	// no data given as input, so create the BB header
	this->setMessageLength(sizeof(T_DVB_BBFRAME));
	this->setMessageType(EmulatedMessageType::BbFrame);
	this->frame()->data_length = 0; // no encapsulation packet at the beginning
	this->frame()->used_modcod = 0; // by default, may be changed
}

BBFrame::~BBFrame()
{
}

bool BBFrame::addPacket(NetPacket *packet)
{
	bool is_added;

	is_added = DvbFrameTpl<T_DVB_BBFRAME>::addPacket(packet);
	if(is_added)
	{
		this->setMessageLength(this->getMessageLength() + packet->getTotalLength());
		this->frame()->data_length = htons(this->num_packets);
	}

	return is_added;
}

// TODO not used => remove ?!
void BBFrame::empty(void)
{
	// remove the payload
	this->data.erase(sizeof(T_DVB_BBFRAME));
	this->num_packets = 0;

	// update the BB frame header
	this->setMessageLength(sizeof(T_DVB_BBFRAME));
	this->frame()->data_length = 0; // no encapsulation packet at the beginning
	this->frame()->used_modcod = 0; // by default, may be changed
}

void BBFrame::setModcodId(uint8_t modcod_id)
{
	this->frame()->used_modcod = modcod_id;
}

uint8_t BBFrame::getModcodId(void) const
{
	return this->frame()->used_modcod;
}

uint16_t BBFrame::getDataLength(void) const
{
	return ntohs(this->frame()->data_length);
}


size_t BBFrame::getOffsetForPayload(void)
{
	return sizeof(T_DVB_BBFRAME);
}


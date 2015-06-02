/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file SlottedAlohaFrame.cpp
 * @brief The Slotted Aloha frame
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
 */

#include "SlottedAlohaFrame.h"
#include "OpenSandFrames.h"

#include <string.h>

// TODO SALOHA is not compatible with physical layer, regenerative, ??

SlottedAlohaFrame::SlottedAlohaFrame(const unsigned char *data, size_t length):
	DvbFrameTpl<T_DVB_SALOHA>(data, length)
{
	this->name = "Slotted Aloha frame";
	this->setMaxSize(MSG_SALOHA_SIZE_MAX);
	this->num_packets = this->getDataLength();
}

SlottedAlohaFrame::SlottedAlohaFrame(const Data &data):
	DvbFrameTpl<T_DVB_SALOHA>(data)
{
	this->name = "Slotted Aloha frame";
	this->setMaxSize(MSG_SALOHA_SIZE_MAX);
	this->num_packets = this->getDataLength();
}

SlottedAlohaFrame::SlottedAlohaFrame(const Data &data, size_t length):
	DvbFrameTpl<T_DVB_SALOHA>(data, length)
{
	this->name = "Slotted Aloha frame";
	this->setMaxSize(MSG_SALOHA_SIZE_MAX);
	this->num_packets = this->getDataLength();
}

SlottedAlohaFrame::SlottedAlohaFrame(DvbFrame *frame):
	DvbFrameTpl<T_DVB_SALOHA>(frame)
{
}

SlottedAlohaFrame::SlottedAlohaFrame():
	DvbFrameTpl<T_DVB_SALOHA>()
{
	this->name = "Slotted Aloha frame";
	this->setMaxSize(MSG_SALOHA_SIZE_MAX);

	//No data given as input, so create the Slotted Aloha header
	this->setMessageLength(sizeof(T_DVB_SALOHA));
	this->frame()->data_length = 0;
}


SlottedAlohaFrameCtrl::SlottedAlohaFrameCtrl():
	SlottedAlohaFrame()
{
	this->setMessageType(MSG_TYPE_SALOHA_CTRL);
}


SlottedAlohaFrameData::SlottedAlohaFrameData():
	SlottedAlohaFrame()
{
	this->setMessageType(MSG_TYPE_SALOHA_DATA);
}

SlottedAlohaFrame::~SlottedAlohaFrame()
{
}

bool SlottedAlohaFrame::addPacket(NetPacket *packet)
{
	bool is_added;
	
	is_added = DvbFrameTpl<T_DVB_SALOHA>::addPacket(packet);
	if(is_added)
	{
		this->setMessageLength(this->getMessageLength() + packet->getTotalLength());
		this->frame()->data_length = htons(this->num_packets);
	}

	return is_added;
}

void SlottedAlohaFrame::empty()
{
	// remove the payload
	this->data.erase(sizeof(T_DVB_SALOHA));
	this->num_packets = 0;

	// update the DVB-RCS frame header
	this->setMessageLength(sizeof(T_DVB_SALOHA));
	this->frame()->data_length = 0; // no encapsulation packet at the beginning
}

uint16_t SlottedAlohaFrame::getDataLength(void) const
{
	return ntohs(this->frame()->data_length);
}


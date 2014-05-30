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
 * @file SlottedAlohaPacketCtrl.cpp
 * @brief The Slotted Aloha control signal packets
 *
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
 */

#include "SlottedAlohaPacketCtrl.h"

#include <arpa/inet.h>


SlottedAlohaPacketCtrl::SlottedAlohaPacketCtrl(const Data &data,
                                               uint8_t ctrl_type):
	SlottedAlohaPacket(data)
{
	saloha_ctrl_hdr_t header;
	this->name = "Slotted Aloha control";
	this->header_length = sizeof(saloha_ctrl_hdr_t);

	header.type = ctrl_type;
	header.total_length = htons(this->header_length + this->data.length());
	this->data.insert(0, (unsigned char*)&header, this->header_length);
}

SlottedAlohaPacketCtrl::SlottedAlohaPacketCtrl(const unsigned char *data,
                                               size_t length):
	SlottedAlohaPacket(data, length)
{
	this->name = "Slotted Aloha control";
	this->header_length = sizeof(saloha_ctrl_hdr_t);
}

SlottedAlohaPacketCtrl::~SlottedAlohaPacketCtrl()
{
}

uint8_t SlottedAlohaPacketCtrl::getCtrlType() const
{
	saloha_ctrl_hdr_t *header;
	
	header = (saloha_ctrl_hdr_t *)this->data.c_str();
	return header->type;
}

saloha_id_t SlottedAlohaPacketCtrl::getId() const
{
	return this->data.substr(sizeof(saloha_ctrl_hdr_t),
	                         this->getTotalLength() - sizeof(saloha_ctrl_hdr_t));
}

saloha_id_t SlottedAlohaPacketCtrl::getUniqueId(void) const
{
	return this->getId();
}

size_t SlottedAlohaPacketCtrl::getTotalLength() const
{
	saloha_ctrl_hdr_t *header;
	
	header = (saloha_ctrl_hdr_t *)this->data.c_str();
	return ntohs(header->total_length);
}


size_t SlottedAlohaPacketCtrl::getPacketLength(const Data &data)
{
	saloha_ctrl_hdr_t *header;
	
	header = (saloha_ctrl_hdr_t *)data.c_str();
	return ntohs(header->total_length);
}

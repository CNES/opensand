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
 * @file BBFrame.cpp
 * @brief BB frame
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "BBFrame.h"
#include "OpenSandFrames.h"

#define DBG_PACKAGE PKG_DEFAULT
#include <opensand_conf/uti_debug.h>

#include <string.h>

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

BBFrame::BBFrame(DvbFrame *frame):
	DvbFrameTpl<T_DVB_BBFRAME>(frame)
{
}

BBFrame::BBFrame():
	DvbFrameTpl<T_DVB_BBFRAME>()
{
	this->name = "BB frame";
	this->setMaxSize(MSG_BBFRAME_SIZE_MAX);

	// no data given as input, so create the BB header
	this->setMessageLength(sizeof(T_DVB_BBFRAME));
	this->setMessageType(MSG_TYPE_BBFRAME);
	this->frame->data_length = 0; // no encapsulation packet at the beginning
	this->frame->used_modcod = 0; // by default, may be changed
	this->frame->real_modcod_nbr = 0; // no MODCOD option at the beginning
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
		this->frame->data_length = htons(this->num_packets);
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
	this->frame->data_length = 0; // no encapsulation packet at the beginning
	this->frame->used_modcod = 0; // by default, may be changed
	this->frame->real_modcod_nbr = 0; // no MODCOD option at the beginning
}

void BBFrame::setModcodId(unsigned int modcod_id)
{
	this->frame->used_modcod = modcod_id;
}

uint8_t BBFrame::getModcodId(void) const
{
	return this->frame->used_modcod;
}

uint16_t BBFrame::getDataLength(void) const
{
	return ntohs(this->frame->data_length);
}

void BBFrame::addModcodOption(tal_id_t tal_id, unsigned int modcod_id)
{
	T_DVB_REAL_MODCOD option;

	option.terminal_id = htons(tal_id);
	option.real_modcod = modcod_id;
	this->data.insert(sizeof(T_DVB_BBFRAME), (unsigned char *)(&option),
	                  sizeof(T_DVB_REAL_MODCOD));

	this->frame->real_modcod_nbr += 1;
}

void BBFrame::getRealModcod(tal_id_t tal_id, uint8_t &modcod_id) const
{
	unsigned int i;
	T_DVB_REAL_MODCOD *real_modcod_option;
	real_modcod_option = (T_DVB_REAL_MODCOD *)(this->frame + sizeof(T_DVB_BBFRAME));

	for(i = 0; i < this->frame->real_modcod_nbr; i++)
	{
		// is the option for us ?
		if(ntohs(real_modcod_option->terminal_id) == tal_id)
		{
			UTI_DEBUG("update real MODCOD to %d\n",
			          real_modcod_option->real_modcod);
			// check if the value is not outside the values of the file
			modcod_id = real_modcod_option->real_modcod;
			return;
		}
		// retrieve one real MODCOD option
		real_modcod_option += 1;
	}
	// let modcod_id unchanged if there is no option
}

size_t BBFrame::getOffsetForPayload(void)
{
	return sizeof(T_DVB_BBFRAME) + this->frame->real_modcod_nbr * sizeof(T_DVB_REAL_MODCOD);
}


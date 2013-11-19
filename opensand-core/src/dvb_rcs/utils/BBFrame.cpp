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

BBFrame::BBFrame(unsigned char *data, unsigned int length):
	DvbFrame(data, length)
{
	this->name = "BB frame";
	this->max_size = MSG_BBFRAME_SIZE_MAX;
	this->data.reserve(this->max_size);
}

BBFrame::BBFrame(Data data):
	DvbFrame(data)
{
	this->name = "BB frame";
	this->max_size = MSG_BBFRAME_SIZE_MAX;
	this->data.reserve(this->max_size);
}

BBFrame::BBFrame(BBFrame *frame):
	DvbFrame(frame)
{
	this->data.reserve(this->max_size);
}

BBFrame::BBFrame():
	DvbFrame()
{
	T_DVB_BBFRAME header;

	this->name = "BB frame";
	this->max_size = MSG_BBFRAME_SIZE_MAX;
	this->data.reserve(this->max_size);

	// no data given as input, so create the BB header
	header.hdr.msg_length = sizeof(T_DVB_BBFRAME);
	header.hdr.msg_type = MSG_TYPE_BBFRAME;
	header.data_length = 0; // no encapsulation packet at the beginning
	header.used_modcod = 0; // by default, may be changed
	header.real_modcod_nbr = 0; // no MODCOD option at the beginning
	this->data.append((unsigned char *) &header, sizeof(T_DVB_BBFRAME));
}

BBFrame::~BBFrame()
{
}

uint16_t BBFrame::getPayloadLength()
{
	return (this->getTotalLength() - this->getOffsetForPayload());
}

Data BBFrame::getPayload()
{
	return Data(this->data, this->getOffsetForPayload(), this->getPayloadLength());
}

bool BBFrame::addPacket(NetPacket *packet)
{
	bool is_added;

	is_added = DvbFrame::addPacket(packet);
	if(is_added)
	{
		T_DVB_BBFRAME *bb_header = (T_DVB_BBFRAME *)this->data.c_str();

		bb_header->hdr.msg_length += packet->getTotalLength();
		bb_header->data_length++;
	}

	return is_added;
}

void BBFrame::empty(void)
{
	T_DVB_BBFRAME *bb_header = (T_DVB_BBFRAME *)this->data.c_str();

	// remove the payload
	this->data.erase(sizeof(T_DVB_BBFRAME));
	this->num_packets = 0;

	// update the BB frame header
	bb_header->hdr.msg_length = sizeof(T_DVB_BBFRAME);
	bb_header->data_length = 0; // no encapsulation packet at the beginning
	bb_header->real_modcod_nbr = 0; // no MODCOD option at the beginning
}

void BBFrame::setModcodId(unsigned int modcod_id)
{
	T_DVB_BBFRAME *bb_header = (T_DVB_BBFRAME *)this->data.c_str();

	bb_header->used_modcod = modcod_id;
}

void BBFrame::setEncapPacketEtherType(uint16_t type)
{
	T_DVB_BBFRAME *bbframe_burst = (T_DVB_BBFRAME *)this->data.c_str();

	bbframe_burst->pkt_type = type;
}

// TODO endianess

uint8_t BBFrame::getModcodId(void) const
{
	T_DVB_BBFRAME *bb_header = (T_DVB_BBFRAME *)this->data.c_str();

	return bb_header->used_modcod;
}


uint16_t BBFrame::getEncapPacketEtherType(void) const
{
	T_DVB_BBFRAME *bbframe_burst = (T_DVB_BBFRAME *)this->data.c_str();

	return bbframe_burst->pkt_type;
}


uint16_t BBFrame::getDataLength(void) const
{
	T_DVB_BBFRAME *bbframe_burst = (T_DVB_BBFRAME *)this->data.c_str();

	return bbframe_burst->data_length;
}

void BBFrame::addModcodOption(tal_id_t tal_id, unsigned int modcod_id)
{
	T_DVB_BBFRAME *bb_header = (T_DVB_BBFRAME *)this->data.c_str();
	T_DVB_REAL_MODCOD option;

	option.terminal_id = htons(tal_id);
	option.real_modcod = modcod_id;
	this->data.insert(sizeof(T_DVB_BBFRAME), (unsigned char *)(&option),
	                  sizeof(T_DVB_REAL_MODCOD));

	bb_header->real_modcod_nbr += 1;
}

void BBFrame::getRealModcod(tal_id_t tal_id, uint8_t &modcod_id) const
{
	unsigned int i;
	T_DVB_BBFRAME *bb_header = (T_DVB_BBFRAME *)this->data.c_str();
	T_DVB_REAL_MODCOD *real_modcod_option;
	real_modcod_option = (T_DVB_REAL_MODCOD *)(this->data.c_str() + sizeof(T_DVB_BBFRAME));

	for(i = 0; i < bb_header->real_modcod_nbr; i++)
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
	T_DVB_BBFRAME *bb_header = (T_DVB_BBFRAME *)this->data.c_str();
	return sizeof(T_DVB_BBFRAME) + bb_header->real_modcod_nbr * sizeof(T_DVB_REAL_MODCOD);
}

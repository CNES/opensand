/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file BBFrame.cpp
 * @brief BB frame
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "BBFrame.h"
#include "msg_dvb_rcs.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


BBFrame::BBFrame(unsigned char *data, unsigned int length):
	DvbFrame(data, length)
{
	this->_name = "BB frame";
	this->max_size = MSG_BBFRAME_SIZE_MAX;
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;
}

BBFrame::BBFrame(Data data):
	DvbFrame(data)
{
	this->_name = "BB frame";
	this->max_size = MSG_BBFRAME_SIZE_MAX;
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;
}

BBFrame::BBFrame(BBFrame *frame):
	DvbFrame(frame)
{
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;
}

BBFrame::BBFrame():
	DvbFrame()
{
	T_DVB_BBFRAME header;

	this->_name = "BB frame";
	this->max_size = MSG_BBFRAME_SIZE_MAX;
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;

	// no data given as input, so create the BB header
	header.hdr.msg_length = sizeof(T_DVB_BBFRAME);
	header.hdr.msg_type = MSG_TYPE_BBFRAME;
	header.dataLength = 0; // no encapsulation packet at the beginning
	header.usedModcod = 0; // by default, may be changed
	header.list_realModcod_size = 0; // no MODCOD option at the beginning
	this->_data.append((unsigned char *) &header, sizeof(T_DVB_BBFRAME));
}

BBFrame::~BBFrame()
{
}

uint16_t BBFrame::payloadLength()
{
	// TODO: substract the size of the MODCOD options here ?
	return (this->totalLength() - sizeof(T_DVB_BBFRAME));
}

Data BBFrame::payload()
{
	// TODO: handle the size of the MODCOD options here ?
	return Data(this->_data, sizeof(T_DVB_BBFRAME), this->payloadLength());
}

bool BBFrame::addPacket(NetPacket *packet)
{
	bool is_added;

	is_added = DvbFrame::addPacket(packet);
	if(is_added)
	{
		T_DVB_BBFRAME bb_header;

		memcpy(&bb_header, this->_data.c_str(), sizeof(T_DVB_BBFRAME));
		bb_header.hdr.msg_length += packet->totalLength();
		bb_header.dataLength++;
		this->_data.replace(0, sizeof(T_DVB_BBFRAME),
		                    (unsigned char *) &bb_header,
		                    sizeof(T_DVB_BBFRAME));
	}

	return is_added;
}

void BBFrame::empty(void)
{
	T_DVB_BBFRAME bb_header;

	// remove the payload
	this->_data.erase(sizeof(T_DVB_BBFRAME));
	this->num_packets = 0;

	// update the BB frame header
	memcpy(&bb_header, this->_data.c_str(), sizeof(T_DVB_BBFRAME));
	bb_header.hdr.msg_length = sizeof(T_DVB_BBFRAME);
	bb_header.dataLength = 0; // no encapsulation packet at the beginning
	bb_header.list_realModcod_size = 0; // no MODCOD option at the beginning
	this->_data.replace(0, sizeof(T_DVB_BBFRAME),
	                    (unsigned char *) &bb_header,
	                    sizeof(T_DVB_BBFRAME));
}

unsigned int BBFrame::getModcodId(void)
{
	T_DVB_BBFRAME bb_header;

	memcpy(&bb_header, this->_data.c_str(), sizeof(T_DVB_BBFRAME));

	return bb_header.usedModcod;
}

void BBFrame::setModcodId(unsigned int modcod_id)
{
	T_DVB_BBFRAME bb_header;

	memcpy(&bb_header, this->_data.c_str(), sizeof(T_DVB_BBFRAME));
	bb_header.usedModcod = modcod_id;
	this->_data.replace(0, sizeof(T_DVB_BBFRAME),
	                    (unsigned char *) &bb_header,
	                    sizeof(T_DVB_BBFRAME));
}

void BBFrame::setEncapPacketType(int type)
{
	T_DVB_BBFRAME bbframe_burst;

	this->_packet_type = (t_pkt_type)type;

	memcpy(&bbframe_burst, this->_data.c_str(), sizeof(T_DVB_BBFRAME));
	bbframe_burst.pkt_type = type;
	this->_data.replace(0, sizeof(T_DVB_BBFRAME),
	                    (unsigned char *) &bbframe_burst,
	                    sizeof(T_DVB_BBFRAME));
}

t_pkt_type BBFrame::getEncapPacketType()
{
	return this->_packet_type;
}

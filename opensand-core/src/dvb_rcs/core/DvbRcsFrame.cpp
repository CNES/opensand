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
#include "msg_dvb_rcs.h"

#define DBG_PACKAGE PKG_DEFAULT
#include <opensand_conf/uti_debug.h>

#include <string.h>


DvbRcsFrame::DvbRcsFrame(unsigned char *data, unsigned int length):
	DvbFrame(data, length)
{
	this->name = "DVB-RCS frame";
	this->max_size = MSG_DVB_RCS_SIZE_MAX;
	this->data.reserve(this->max_size);
}

DvbRcsFrame::DvbRcsFrame(Data data):
	DvbFrame(data)
{
	this->name = "DVB-RCS frame";
	this->max_size = MSG_DVB_RCS_SIZE_MAX;
	this->data.reserve(this->max_size);
}

DvbRcsFrame::DvbRcsFrame(DvbRcsFrame *frame):
	DvbFrame(frame)
{
	this->data.reserve(this->max_size);
	this->num_packets = frame->getNumPackets();
}

DvbRcsFrame::DvbRcsFrame():
	DvbFrame()
{
	T_DVB_ENCAP_BURST header;

	this->name = "DVB-RCS frame";
	this->max_size = MSG_DVB_RCS_SIZE_MAX;
	this->data.reserve(this->max_size);

	// no data given as input, so create the DVB-RCS header
	header.hdr.msg_length = sizeof(T_DVB_ENCAP_BURST);
	header.hdr.msg_type = MSG_TYPE_DVB_BURST;
	header.qty_element = 0; // no encapsulation packet at the beginning
	this->data.append((unsigned char *) &header, sizeof(T_DVB_ENCAP_BURST));
}

DvbRcsFrame::~DvbRcsFrame()
{
}

uint16_t DvbRcsFrame::getPayloadLength()
{
	return (this->getTotalLength() - sizeof(T_DVB_ENCAP_BURST));
}

Data DvbRcsFrame::getPayload()
{
	return Data(this->data, sizeof(T_DVB_ENCAP_BURST), this->getPayloadLength());
}

bool DvbRcsFrame::addPacket(NetPacket *packet)
{
	bool is_added;

	is_added = DvbFrame::addPacket(packet);
	if(is_added)
	{
		T_DVB_ENCAP_BURST dvb_header;

		memcpy(&dvb_header, this->data.c_str(), sizeof(T_DVB_ENCAP_BURST));
		dvb_header.hdr.msg_length += packet->getTotalLength();
		dvb_header.qty_element++;
		this->data.replace(0, sizeof(T_DVB_ENCAP_BURST),
		                    (unsigned char *) &dvb_header,
		                    sizeof(T_DVB_ENCAP_BURST));
	}

	return is_added;
}

void DvbRcsFrame::empty(void)
{
	T_DVB_ENCAP_BURST *dvb_header = (T_DVB_ENCAP_BURST *)this->data.c_str();

	// remove the payload
	this->data.erase(sizeof(T_DVB_ENCAP_BURST));
	this->num_packets = 0;

	// update the DVB-RCS frame header
	dvb_header->hdr.msg_length = sizeof(T_DVB_ENCAP_BURST);
	dvb_header->qty_element = 0; // no encapsulation packet at the beginning
}

void DvbRcsFrame::setEncapPacketEtherType(uint16_t type)
{
	T_DVB_ENCAP_BURST *dvb_burst = (T_DVB_ENCAP_BURST *)this->data.c_str();

	dvb_burst->pkt_type = type;
}



/**
 * @file DvbRcsFrame.cpp
 * @brief DVB-RCS frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "DvbRcsFrame.h"
#include "msg_dvb_rcs.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


DvbRcsFrame::DvbRcsFrame(unsigned char *data, unsigned int length):
	DvbFrame(data, length)
{
	this->_name = "DVB-RCS frame";
	this->max_size = MSG_DVB_RCS_SIZE_MAX;
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;
}

DvbRcsFrame::DvbRcsFrame(Data data):
	DvbFrame(data)
{
	this->_name = "DVB-RCS frame";
	this->max_size = MSG_DVB_RCS_SIZE_MAX;
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;
}

DvbRcsFrame::DvbRcsFrame(DvbRcsFrame *frame):
	DvbFrame(frame)
{
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;
	this->num_packets = frame->getNumPackets();
}

DvbRcsFrame::DvbRcsFrame():
	DvbFrame()
{
	T_DVB_ENCAP_BURST header;

	this->_name = "DVB-RCS frame";
	this->max_size = MSG_DVB_RCS_SIZE_MAX;
	this->_data.reserve(this->max_size);
	this->_packet_type = PKT_TYPE_INVALID;

	// no data given as input, so create the DVB-RCS header
	header.hdr.msg_length = sizeof(T_DVB_ENCAP_BURST);
	header.hdr.msg_type = MSG_TYPE_DVB_BURST;
	header.qty_element = 0; // no encapsulation packet at the beginning
	this->_data.append((unsigned char *) &header, sizeof(T_DVB_ENCAP_BURST));
}

DvbRcsFrame::~DvbRcsFrame()
{
}

uint16_t DvbRcsFrame::payloadLength()
{
	return (this->totalLength() - sizeof(T_DVB_ENCAP_BURST));
}

Data DvbRcsFrame::payload()
{
	return Data(this->_data, sizeof(T_DVB_ENCAP_BURST), this->payloadLength());
}

bool DvbRcsFrame::addPacket(NetPacket *packet)
{
	bool is_added;

	is_added = DvbFrame::addPacket(packet);
	if(is_added)
	{
		T_DVB_ENCAP_BURST dvb_header;

		memcpy(&dvb_header, this->_data.c_str(), sizeof(T_DVB_ENCAP_BURST));
		dvb_header.hdr.msg_length += packet->totalLength();
		dvb_header.qty_element++;
		this->_data.replace(0, sizeof(T_DVB_ENCAP_BURST),
		                    (unsigned char *) &dvb_header,
		                    sizeof(T_DVB_ENCAP_BURST));
	}

	return is_added;
}

void DvbRcsFrame::empty(void)
{
	T_DVB_ENCAP_BURST dvb_header;

	// remove the payload
	this->_data.erase(sizeof(T_DVB_ENCAP_BURST));
	this->num_packets = 0;

	// update the DVB-RCS frame header
	memcpy(&dvb_header, this->_data.c_str(), sizeof(T_DVB_ENCAP_BURST));
	dvb_header.hdr.msg_length = sizeof(T_DVB_ENCAP_BURST);
	dvb_header.qty_element = 0; // no encapsulation packet at the beginning
	this->_data.replace(0, sizeof(T_DVB_ENCAP_BURST),
	                    (unsigned char *) &dvb_header,
	                    sizeof(T_DVB_ENCAP_BURST));
}

void DvbRcsFrame::setEncapPacketType(int type)
{
	T_DVB_ENCAP_BURST dvb_burst;

	this->_packet_type = (t_pkt_type)type;

	memcpy(&dvb_burst, this->_data.c_str(), sizeof(T_DVB_ENCAP_BURST));
	dvb_burst.pkt_type = type;
	this->_data.replace(0, sizeof(T_DVB_ENCAP_BURST),
	                    (unsigned char *) &dvb_burst,
	                    sizeof(T_DVB_ENCAP_BURST));
}

t_pkt_type DvbRcsFrame::getEncapPacketType()
{
	return this->_packet_type;
}

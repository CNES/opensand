/**
 * @file GseSwitch.cpp
 * @brief GSE switch for Satellite Emulator (SE)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "GseSwitch.h"


GseSwitch::GseSwitch()
{
}

GseSwitch::~GseSwitch()
{
}

long GseSwitch::find(NetPacket *packet)
{
	GsePacket *gse_packet;
	std::map < long, long >::iterator it_switch;
	std::map < uint8_t, long >::iterator it_frag_id;
	long spot_id = 0;

	if(packet == NULL)
		goto error;

	if(packet->type() != NET_PROTO_GSE)
		goto error;

	gse_packet = (GsePacket *) packet;

	// if this is a subsequent fragment of PDU (no label field in packet)
	if(gse_packet->start_indicator() == 0)
	{
		it_frag_id = this->frag_id_table.find(gse_packet->fragId());

		if(it_frag_id != this->frag_id_table.end())
			spot_id = (*it_frag_id).second;
	}
	else // there is a label field in packet
	{
		// get terminal id contained in GSE packet label field
		it_switch = this->switch_table.find(gse_packet->talId());

		if(it_switch != this->switch_table.end())
			spot_id = (*it_switch).second;

		// if this is a first fragment store the spot id corresponding to the frag id
		if(gse_packet->end_indicator() == 0)
		{
			// remove the entry in map if the key already exists
			it_frag_id = this->frag_id_table.find(gse_packet->fragId());

			if(it_frag_id != this->frag_id_table.end())
				frag_id_table.erase(gse_packet->fragId());

			frag_id_table.insert(std::pair < uint8_t, long > (gse_packet->fragId(), spot_id));
		}
	}

error:
	return spot_id;
}


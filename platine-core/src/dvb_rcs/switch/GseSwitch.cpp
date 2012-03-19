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
 * @file GseSwitch.cpp
 * @brief GSE switch for Satellite Emulator (SE)
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
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

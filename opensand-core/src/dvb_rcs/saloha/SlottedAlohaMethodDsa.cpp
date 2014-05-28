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
 * @file SlottedAlohaDsa.cpp
 * @brief The DSA method
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
*/

#include "SlottedAlohaMethodDsa.h"
#include "SlottedAlohaTypes.h"


SlottedAlohaMethodDsa::SlottedAlohaMethodDsa():
	SlottedAlohaMethod()
{
}

SlottedAlohaMethodDsa::~SlottedAlohaMethodDsa()
{
}

void SlottedAlohaMethodDsa::removeCollisions(map<unsigned int, Slot *> &slots,
                                             saloha_packets_t *accepted_packets)
{
	map<unsigned int, Slot *>::iterator slot_it;

	// cf: DSA algorithm
	for(map<unsigned int, Slot *>::iterator slot_it = slots.begin();
	    slot_it != slots.end(); ++slot_it)
	{
		Slot *slot = (*slot_it).second;
		saloha_packets_t::iterator pkt_it;
		saloha_packets_t &packets = slot->getPackets();
		if(!slot->getNbrPackets())
		{
			continue;
		}
		packets = slot->getPackets();

		LOG(this->log_saloha, LEVEL_DEBUG,
		    "Remove collisions on slot %u, containing %zu packets\n",
		    slot->getId(), packets.size());

		if((packets.size() == 1) &&
		   (std::find(accepted_packets->begin(),
		              accepted_packets->end(),
		              packets.front()) == accepted_packets->end()))
		{
			accepted_packets->push_back(packets.front());
			LOG(this->log_saloha, LEVEL_DEBUG,
			    "No collision, keep packet from terminal %u\n",
			    packets.front()->getSrcTalId());
		}
		else
		{
			// TODO NOTICE
			LOG(this->log_saloha, LEVEL_WARNING,
			    "Collision on slot %u, remove packets\n", slot->getId());
			for(pkt_it = packets.begin();
			    pkt_it != packets.end();
			    ++pkt_it)
			{
				// delete packets on the slot, except those that
				// was accepted on another slot, to avoid releasing
				// data that will be used later
				if(std::find(accepted_packets->begin(),
				             accepted_packets->end(),
				             *pkt_it) == accepted_packets->end())
				{
				   delete *pkt_it;
				}
			}
		}
		slot->clear();
	}
}


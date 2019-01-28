/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @brief The DSA algo
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#include "SlottedAlohaAlgoDsa.h"


SlottedAlohaAlgoDsa::SlottedAlohaAlgoDsa():
	SlottedAlohaAlgo()
{
}

SlottedAlohaAlgoDsa::~SlottedAlohaAlgoDsa()
{
}

uint16_t SlottedAlohaAlgoDsa::removeCollisions(map<unsigned int, Slot *> &slots,
                                                 saloha_packets_data_t *accepted_packets)
{
	map<unsigned int, Slot *>::iterator slot_it;
	map<tal_id_t, vector<saloha_id_t> > accepted_ids;
	uint16_t nbr_collisions = 0;

	// cf: DSA algorithm
	for(map<unsigned int, Slot *>::iterator slot_it = slots.begin();
	    slot_it != slots.end(); ++slot_it)
	{
		Slot *slot = (*slot_it).second;
		saloha_packets_data_t::iterator pkt_it;
		if(!slot->size())
		{
			continue;
		}

		LOG(this->log_saloha, LEVEL_DEBUG,
		    "Remove collisions on slot %u, containing %zu packets\n",
		    slot->getId(), slot->size());

		if(slot->size() == 1)
		{
			SlottedAlohaPacketData *packet;
			packet = dynamic_cast<SlottedAlohaPacketData *>(slot->front());
			tal_id_t tal_id = packet->getSrcTalId();

			// create accepted_ids for this terminal if it does not exist
			if(accepted_ids.find(tal_id) == accepted_ids.end())
			{
				accepted_ids[tal_id] = vector<saloha_id_t>();
			}

			if(std::find(accepted_ids[tal_id].begin(),
			             accepted_ids[tal_id].end(),
			             packet->getUniqueId()) == accepted_ids[tal_id].end())
			{
				accepted_packets->push_back(packet);
				accepted_ids[tal_id].push_back(packet->getUniqueId());
				LOG(this->log_saloha, LEVEL_DEBUG,
				    "No collision, keep packet from terminal %u\n",
				    tal_id);
			}
			else
			{
				// packet was already received on another slot
				delete packet;
			}
		}
		else
		{
			LOG(this->log_saloha, LEVEL_NOTICE,
			    "Collision on slot %u, remove packets\n", slot->getId());
			nbr_collisions += slot->size();
			for(pkt_it = slot->begin();
			    pkt_it != slot->end();
			    ++pkt_it)
			{
			   delete *pkt_it;
			}
		}
		slot->clear();
	}
	return nbr_collisions;
}


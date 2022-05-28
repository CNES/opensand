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
 * @file SlottedAlohaCrdsa.cpp
 * @brief The CRDSA algo
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#include "SlottedAlohaAlgoCrdsa.h"


SlottedAlohaAlgoCrdsa::SlottedAlohaAlgoCrdsa():
	SlottedAlohaAlgo()
{
}

SlottedAlohaAlgoCrdsa::~SlottedAlohaAlgoCrdsa()
{
}

uint16_t SlottedAlohaAlgoCrdsa::removeCollisions(std::map<unsigned int, Slot *> &slots,
                                                 saloha_packets_data_t *accepted_packets)
{
  std::map<tal_id_t, std::vector<saloha_id_t> > accepted_ids;
	uint16_t nbr_collisions = 0;
	bool stop;
	
	//cf: CRDSA algorithm, but here, each packet is searched in accepted_packets
	//    for each iteration
	LOG(this->log_saloha, LEVEL_DEBUG,
	    "Start removing collisions\n");
	do
	{
		stop = true;
		for(auto&& slot_it : slots)
		{
			Slot *slot = slot_it.second;
			if(!slot->size())
			{
				continue;
			}
			LOG(this->log_saloha, LEVEL_DEBUG,
			    "Remove collisions on slot %u, containing %zu packets\n",
			    slot->getId(), slot->size());

			auto pkt_it = slot->begin();
			// remove packets that were accepted on another slot from this slot
			// (i.e. signal suppression of this packet)
			while(pkt_it != slot->end())
			{
				auto& packet = *pkt_it;
				tal_id_t tal_id = packet->getSrcTalId();

				// create accepted_ids for this terminal if it does not exist
				if(accepted_ids.find(tal_id) == accepted_ids.end())
				{
					accepted_ids[tal_id] = std::vector<saloha_id_t>();
				}

				if(std::find(accepted_ids[tal_id].begin(),
				             accepted_ids[tal_id].end(),
				             packet->getUniqueId()) != accepted_ids[tal_id].end())
				{
					slot->erase(pkt_it);
					// avoid removing an accepted packet
          /*
					if(std::find(accepted_packets->begin(),
					             accepted_packets->end(),
					             packet) == accepted_packets->end())
					{
						delete packet;
					}
          */
					// erase goes to next iterator
					continue;
				}
				pkt_it++;
			}
			LOG(this->log_saloha, LEVEL_DEBUG,
			    "Slot %u contains %zu packets after signal suppression\n",
			    slot->getId(), slot->size());
			if(slot->size() == 1)
			{
				auto& packet = slot->front();
				tal_id_t tal_id = packet->getSrcTalId();

				// create accepted_ids for this terminal if it does not exist
				if(accepted_ids.find(tal_id) == accepted_ids.end())
				{
					accepted_ids[tal_id] = std::vector<saloha_id_t>();
				}

				accepted_ids[tal_id].push_back(packet->getUniqueId());
				accepted_packets->push_back(std::move(packet));
				// packet is decoded, we need to restart the check all slots
				// to remove the signal of this packet when a duplicate was found
				stop = false;
				LOG(this->log_saloha, LEVEL_DEBUG,
				    "No collision on slot %u, keep packet from terminal %u\n",
				    slot->getId(), tal_id);
			}
			else if(slot->size())
			{
				LOG(this->log_saloha, LEVEL_DEBUG,
				    "Collision on slot %u at the moment\n",
				    slot->getId());
			}
		}
	}
	while(!stop);

	for(auto&& slot_it : slots)
	{
		Slot *slot = slot_it.second;
		// check for collisions here, we do not count collisions that were avoided
		if(slot->size() > 1)
		{
			LOG(this->log_saloha, LEVEL_NOTICE,
			    "There is still collision on slot %u, remove packets\n",
			    slot->getId());
			nbr_collisions += slot->size();
		}
		slot->clear();
	}
	return nbr_collisions;
}


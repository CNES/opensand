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
 * @file SlottedAlohaCrdsa.cpp
 * @brief The CRDSA method
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
*/

#include "SlottedAlohaMethodCrdsa.h"


SlottedAlohaMethodCrdsa::SlottedAlohaMethodCrdsa():
	SlottedAlohaMethod()
{
}

SlottedAlohaMethodCrdsa::~SlottedAlohaMethodCrdsa()
{
}

uint16_t SlottedAlohaMethodCrdsa::removeCollisions(map<unsigned int, Slot *> &slots,
                                                   saloha_packets_t *accepted_packets)
{
	map<unsigned int, Slot *>::iterator slot_it;
	vector<saloha_id_t> accepted_ids;
	saloha_packets_t::iterator pkt_it;
	uint16_t nbr_collisions = 0;
	bool stop;
	
	//cf: CRDSA algorithm, but here, each packet is searched in accepted_packets
	//    for each iteration
	do
	{
		stop = true;
		for(map<unsigned int, Slot *>::iterator slot_it = slots.begin();
			slot_it != slots.end(); ++slot_it)
		{
			Slot *slot = (*slot_it).second;
			SlottedAlohaPacketData *packet;
			if(!slot->size())
			{
				continue;
			}

			pkt_it = slot->begin();
			// remove packets that were accepted on another slot from this slot
			// (i.e. signal suppression of this packet)
			while(pkt_it != slot->end())
			{
				packet = dynamic_cast<SlottedAlohaPacketData *>(*pkt_it);
				if(std::find(accepted_ids.begin(),
				             accepted_ids.end(),
				             packet->getUniqueId()) != accepted_ids.end())
				{
					slot->erase(pkt_it);
					// avoid removing an accepted packet
					if(std::find(accepted_packets->begin(),
					             accepted_packets->end(),
					             packet) == accepted_packets->end())
					{
						delete packet;
					}
					// erase goes to next iterator
					continue;
				}
				pkt_it++;
			}
			if(slot->size() == 1)
			{
				packet = dynamic_cast<SlottedAlohaPacketData *>(slot->front());
				accepted_packets->push_back(packet);
				accepted_ids.push_back(packet->getUniqueId());
				// packet is decoded, we need to restart the check all slots
				// to remove the signal of this packet when a duplicate was found
				stop = false;
			}
		}
	}
	while(!stop);
	for(map<unsigned int, Slot *>::iterator slot_it = slots.begin();
		slot_it != slots.end(); ++slot_it)
	{
		Slot *slot = (*slot_it).second;
		// check for collisions here, we do not count collisions that were avoided
		if(slot->size() > 1)
		{
			nbr_collisions++;
			for(pkt_it = slot->begin();
			    pkt_it != slot->end();
			    ++pkt_it)
			{
				// remove collisionned packets
			   delete *pkt_it;
			}
		}
		(*slot_it).second->clear();
	}
	return 0;
}


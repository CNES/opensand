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
#include "SlottedAlohaTypes.h"

SlottedAlohaMethodCrdsa::SlottedAlohaMethodCrdsa():
	SlottedAlohaMethod()
{
}

SlottedAlohaMethodCrdsa::~SlottedAlohaMethodCrdsa()
{
}

void SlottedAlohaMethodCrdsa::removeCollisions(map<unsigned int, Slot *> &slots,
                                               saloha_packets_t *accepted_packets)
{
	map<unsigned int, Slot *>::iterator slot_it;
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
			saloha_packets_t::iterator pkt_it;
			saloha_packets_t &packets = slot->getPackets();
			if(!slot->getNbrPackets())
			{
				continue;
			}

			pkt_it = packets.begin();
			while(pkt_it != packets.end())
			{
				if(std::find(accepted_packets->begin(),
				             accepted_packets->end(),
				             *pkt_it) != accepted_packets->end())
				{
					// erase goes to next iterator
					packets.erase(pkt_it);
					continue;
				}
				pkt_it++;
			}
			if(packets.size() == 1)
			{
				accepted_packets->push_back(packets.front());
				stop = false;
			}
		}
	}
	while(!stop);
	for(map<unsigned int, Slot *>::iterator slot_it = slots.begin();
		slot_it != slots.end(); ++slot_it)
	{
		(*slot_it).second->clear();
	}
}


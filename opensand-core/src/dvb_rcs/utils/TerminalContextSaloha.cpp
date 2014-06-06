/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file    TerminalContextSaloha.cpp
 * @brief   The terminal context for Slotted Aloha
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#include "TerminalContextSaloha.h"

#include <opensand_output/Output.h>

#define MAX_OLD_COUNTER 65535


static bool sortSalohaPacketData(const SlottedAlohaPacketData *pkt1,
                                 const SlottedAlohaPacketData *pkt2)
{
	return pkt1->getSeq() < pkt2->getSeq();
}

TerminalContextSaloha::TerminalContextSaloha(tal_id_t tal_id):
	TerminalContext(tal_id),
	wait_propagation(),
	oldest_id(),
	old_count(0)
{
	this->log_saloha = Output::registerLog(LEVEL_WARNING, "Dvb.SlottedAloha");
}

TerminalContextSaloha::~TerminalContextSaloha()
{
}

prop_state_t TerminalContextSaloha::addPacket(SlottedAlohaPacketData *packet,
                                              saloha_packets_data_t &pdu)
{
	saloha_pdu_id_t pdu_id = packet->getId();
	saloha_id_t pkt_id = packet->getUniqueId();
	qos_t qos = packet->getQos();
	saloha_packets_data_t sa_packets;
	map<qos_t, pdus_t>::iterator wait_it;
	pdus_t pdus;
	pdus_t::iterator pdu_it;
	uint16_t id[4];

	SlottedAlohaPacketData::convertPacketId(pkt_id, id);
	
	wait_it = this->wait_propagation.find(qos);
	if(wait_it != this->wait_propagation.end())
	{
		// the entry for this qos already exists
		pdus = (*wait_it).second;
		pdu_it = pdus.find(pdu_id);
		// the entry for this PDU already exists
		if(pdu_it != pdus.end())
		{
			sa_packets = this->wait_propagation[qos][pdu_id];
		}
	}
	// add packet in list
	sa_packets.push_back(packet);

	pdus[pdu_id] = sa_packets;
	this->wait_propagation[qos] = pdus;

	// check if PDU is complete
	if(id[SALOHA_ID_PDU_NB] == sa_packets.size())
	{
		// this makes a copy
		pdu = sa_packets;
		// packets should be received in correct order, but
		// in case of loss this order is not ensured
		// => need to sort on seq (see operator in SlottedAlohaPacketData)
		sort(pdu.begin(), pdu.end(), sortSalohaPacketData);

		// clear element
		sa_packets.clear();
		pdu_it = this->wait_propagation[qos].find(pdu_id);
		this->wait_propagation[qos].erase(pdu_it);

		// new pdu, increase old counter
		this->old_count++;

		// handle oldest ID
		this->handleOldest(qos, pdu_id);
		return prop;
	}
	if(this->oldest_id.find(qos) == this->oldest_id.end())
	{
		// no oldest packet, update it
		this->oldest_id[qos] = pdu_id;
		this->old_count = 0;
	}

	return no_prop;
}

void TerminalContextSaloha::handleOldest(qos_t qos, saloha_pdu_id_t current_id)
{
	pdus_t::iterator pdu_it;
	saloha_pdu_id_t oldest = this->oldest_id[qos];

	if(oldest == current_id)
	{
		this->findOldest(qos);
		return;
	}

	// see comment at EOF for this test
	if(this->old_count > MAX_OLD_COUNTER)
	{
		// delete this data
		tal_id_t tal_id = 0;
		pdu_it = this->wait_propagation[qos].find(oldest);
		saloha_packets_data_t packets = (*pdu_it).second;
		for(saloha_packets_data_t::iterator it = packets.begin();
		    it != packets.end(); ++it)
		{
			tal_id = (*it)->getSrcTalId();
			delete *it;
		}
		LOG(this->log_saloha, LEVEL_WARNING,
		    "We may have lost at least a packet from PDU %u on ST%u, "
		    "drop pending content (current_id %u)\n",
		    oldest, tal_id, current_id);

		this->wait_propagation[qos].erase(pdu_it);

		// find next oldest
		this->findOldest(qos);
	}
}

void TerminalContextSaloha::findOldest(qos_t qos)
{
	saloha_pdu_id_t diff;
	saloha_pdu_id_t oldest;
	map<qos_t, saloha_pdu_id_t>::iterator id_it;
	pdus_t::iterator pdu_it;
	saloha_pdu_id_t min_diff = (saloha_pdu_id_t)pow(2.0, 8 * sizeof(saloha_pdu_id_t));

	oldest = this->oldest_id[qos];
	id_it = this->oldest_id.find(qos);
	// no next oldest
	if(!this->wait_propagation[qos].size())
	{
		this->oldest_id.erase(id_it);
		return;
	}

	// to avoid problems because of max, we try each id until we get one
	for(pdu_it = this->wait_propagation[qos].begin();
	    pdu_it != this->wait_propagation[qos].end();
	    ++pdu_it)
	{
		// whether oldest is greater or smaller than pdu_id
		// ha no importance has we are on unsigned with modulo
		diff = (*pdu_it).first - oldest;
		if(diff < min_diff)
		{
			this->oldest_id[qos] = (*pdu_it).first;
			this->old_count = 0;
			min_diff = diff;
		}
	}
}


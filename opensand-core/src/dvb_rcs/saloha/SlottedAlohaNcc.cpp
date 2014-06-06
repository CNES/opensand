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
 * @file SlottedAlohaNcc.cpp
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien BERNARD / Viveris Technologies
 */

#include "SlottedAlohaNcc.h"
#include "SlottedAlohaFrame.h"
#include "SlottedAlohaAlgo.h"
#include "SlottedAlohaPacketCtrl.h"
#include "SlottedAlohaAlgoDsa.h"
#include "SlottedAlohaAlgoCrdsa.h"

#include <stdlib.h>
#include <math.h>

#include <opensand_conf/conf.h>


// functor for SlottedAlohaPacket comparison
//

SlottedAlohaNcc::SlottedAlohaNcc():
	SlottedAloha(),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	terminals(),
	algo(NULL),
	simulation_traffic(0)
{
}

SlottedAlohaNcc::~SlottedAlohaNcc()
{
	for(saloha_terminals_t::iterator it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		delete it->second;
	}
	this->terminals.clear();

	TerminalCategories<TerminalCategorySaloha>::iterator it;

	for(it = this->categories.begin();
	    it != this->categories.end(); ++it)
	{
		delete (*it).second;
	}
	this->categories.clear();

	this->terminal_affectation.clear();

	delete this->algo;
}

bool SlottedAlohaNcc::init(TerminalCategories<TerminalCategorySaloha> &categories,
                           TerminalMapping<TerminalCategorySaloha> terminal_affectation,
                           TerminalCategorySaloha *default_category)

{
	string algo_name;
	TerminalCategories<TerminalCategorySaloha>::const_iterator cat_iter;

	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Parent 'init()' method must be called first.\n");
		return false;
	}

	this->categories = categories;
	// we keep terminal affectation and default category but these affectations
	// and the default category can concern non Slotted Aloha categories
	// so be careful when adding a new terminal
	this->terminal_affectation = terminal_affectation;
	this->default_category = default_category;
	if(!this->default_category)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "No default terminal affectation defined, "
		    "some terminals may not be able to log in\n");
	}

	for(cat_iter = this->categories.begin(); cat_iter != this->categories.end();
	    ++cat_iter)
	{
		TerminalCategorySaloha *cat = (*cat_iter).second;
		cat->setSlotsNumber(this->frame_duration_ms,
		                    this->pkt_hdl->getFixedLength());
		Probe<int> *probe_coll;

		probe_coll = Output::registerProbe<int>(true, SAMPLE_SUM,
		                                       "Aloha.collisions.%s",
		                                       (*cat_iter).first.c_str());
		this->probe_collisions.insert(
		            pair<string, Probe<int> *>((*cat_iter).first,
		                                       probe_coll));
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_ALGO, algo_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_ALGO);
		return false;
	}
	// TODO add more parameters (nb_replicas, max_pkt, ...)
/*	if(!Conf::getValue(SALOHA_SECTION, SALOHA_SIMU_TRAFFIC,
	                   this->simulation_traffic))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_SIMU_TRAFFIC);
		return false;
	}*/

	if (algo_name == "DSA")
	{
		this->algo = new SlottedAlohaAlgoDsa();
	}
	else if (algo_name == "CRDSA")
	{
		this->algo = new SlottedAlohaAlgoCrdsa();
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to set Slotted Aloha '%s' algorithm\n", algo_name.c_str());
		return false;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "initialize Slotted Aloha with %s algorithm\n",
	    algo_name.c_str());

	return true;
}

bool SlottedAlohaNcc::onRcvFrame(DvbFrame *dvb_frame)
{
	SlottedAlohaFrame *frame;
	size_t previous_length;

	// TODO static cast
	frame = dvb_frame->operator SlottedAlohaFrame*();
	
	if(frame->getDataLength() <= 0)
	{
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "skip Slotted Aloha frame with no packet");
		goto skip;
	}	

	LOG(this->log_saloha, LEVEL_INFO,
	    "Receive Slotted Aloha frame containing %u packets\n",
	    frame->getDataLength());

	previous_length = 0;
	for(unsigned int cpt = 0; cpt < frame->getDataLength(); cpt++)
	{
		SlottedAlohaPacketData *sa_packet;
		map<unsigned int, Slot *> slots;
		map<unsigned int, Slot *>::iterator slot_it;
		TerminalContextSaloha *terminal;
		saloha_terminals_t::iterator st;
		TerminalCategorySaloha *category;
		tal_id_t src_tal_id;
		qos_t qos;
		Data encap;
		Data payload = frame->getPayload(previous_length);
		size_t current_length =
			SlottedAlohaPacketData::getPacketLength(payload);

		sa_packet = new SlottedAlohaPacketData(payload,
		                                       current_length);
		previous_length += current_length;
		if(!sa_packet)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "cannot create a Slotted Aloha data packet\n");
			continue;
		}
		// we need to keep qos and src_tal_id of inner encapsulated packet
		encap = sa_packet->getPayload();
		this->pkt_hdl->getSrc(encap, src_tal_id); 
		this->pkt_hdl->getQos(encap, qos); 
		sa_packet->setSrcTalId(src_tal_id);
		sa_packet->setQos(qos);

		// find the associated terminal category
		st = this->terminals.find(src_tal_id);
		if(st == this->terminals.end())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Slotted Aloha packet received from unknown terminal %u\n",
			    src_tal_id);
			delete sa_packet;
			continue;
		}
		terminal = st->second;
		category = this->categories[terminal->getCurrentCategory()];

		// Add replicas in the corresponding slots
		slots = category->getSlots();
		slot_it = slots.find(sa_packet->getTs());
		if(slot_it == slots.end())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "packet received on a slot that does not exist\n");
			delete sa_packet;
			continue;
		}
		(*slot_it).second->push_back(sa_packet);
		category->increaseReceivedPacketsNbr();
	}

skip:
	delete dvb_frame;
	return true;
}


bool SlottedAlohaNcc::schedule(NetBurst **burst,
                               list<DvbFrame *> &complete_dvb_frames,
                               time_sf_t superframe_counter)
{
	TerminalCategories<TerminalCategorySaloha>::const_iterator cat_iter;
	
	if(!this->isSalohaFrameTick(superframe_counter))
	{
		return true;
	}
	// TODO at the moment we simulate trafic on all categories
	for(cat_iter = this->categories.begin(); cat_iter != this->categories.end();
	    ++cat_iter)
	{
		TerminalCategorySaloha *category = (*cat_iter).second;
		if(!this->scheduleCategory(category, burst, complete_dvb_frames))
		{
			return false;
		}
	}
	return true;
}

bool SlottedAlohaNcc::scheduleCategory(TerminalCategorySaloha *category,
                                       NetBurst **burst,
                                       list<DvbFrame *> &complete_dvb_frames)
{
	SlottedAlohaFrameCtrl *frame;
	saloha_packets_data_t *accepted_packets;
	saloha_packets_data_t::iterator pkt_it;
	// refresh the probe in case of no traffic
	this->probe_collisions[category->getLabel()]->put(0);
	if(!category->getReceivedPacketsNbr())
	{
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "No packet to schedule in category %s\n",
		    category->getLabel().c_str());
		return true;
		// TODO check wait propagation packets here ?
	}

	*burst = new NetBurst();
	if (!(*burst))
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to create a Slotted Aloha burst");
		return false;
	}

	//this->simulateTraffic(category); // Simulate traffic to get performance statistics
	category->resetReceivedPacketsNbr();
	LOG(this->log_saloha, LEVEL_DEBUG,
	    "Remove collisions on category %s\n",
	    category->getLabel().c_str());
	this->removeCollisions(category); // Call specific algorithm to remove collisions

	// create the Slotted Aloha control frame
	frame = new SlottedAlohaFrameCtrl();
	if(!frame)
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to create a Slotted Aloha signal control frame");
		return false;
	}

	LOG(this->log_saloha, LEVEL_DEBUG,
	    "Schedule Slotted Aloha packets\n");

	// Propagate if possible all packets received to encap block
	accepted_packets = category->getAcceptedPackets();
	pkt_it = accepted_packets->begin();
	while(pkt_it != accepted_packets->end())
	{
		SlottedAlohaPacketData *sa_packet = *pkt_it;
		SlottedAlohaPacketCtrl *ack;
		saloha_packets_data_t pdu;
		TerminalContextSaloha *terminal;
		saloha_terminals_t::iterator st;
		saloha_pdu_id_t id_pdu;
		saloha_id_t id_packet;
		tal_id_t tal_id;
		prop_state_t state;

		// erase goes to next iterator
		accepted_packets->erase(pkt_it);
		id_packet = sa_packet->getUniqueId();
		id_pdu = sa_packet->getId();
		tal_id = sa_packet->getSrcTalId();

		if(tal_id == GW_TAL_ID)
		{
			LOG(this->log_saloha, LEVEL_DEBUG,
			    "drop Slotted Aloha simulation packet\n");
			delete sa_packet;
			continue;
		}

		st = this->terminals.find(tal_id);
		if(st == this->terminals.end())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Cannot find terminal %u associated with packet\n",
			    tal_id);
			delete sa_packet;
			continue;
		}
		terminal = st->second;
		if(terminal->getCurrentCategory() != category->getLabel())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Wrong category %s for packet with source terminal ID %u\n",
			    category->getLabel().c_str(), tal_id);
			delete sa_packet;
			continue;
		}

		// Send an ACK
		ack = new SlottedAlohaPacketCtrl(id_packet, SALOHA_CTRL_ACK, tal_id);
		if(!ack)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "failed to create a Slotted Aloha signal control "
			    "packet");
			delete sa_packet;
			continue;
		}

		if(frame->getFreeSpace() < ack->getTotalLength())
		{
			// add the previous frame in complete frames
			complete_dvb_frames.push_back((DvbFrame *)frame);
			// create a new Slotted Aloha control frame
			frame = new SlottedAlohaFrameCtrl();
			if(!frame)
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to create a Slotted Aloha signal control frame");
				delete sa_packet;
				return false;
			}
		}
		if(!frame->addPacket(ack))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "failed to add a Slotted Aloha packet in "
			    "signal control frame");
			delete ack;
			delete sa_packet;
			continue;
		}
		LOG(this->log_saloha, LEVEL_INFO,
		    "Ack packet %s on ST%u\n", id_packet.c_str(), tal_id);
		delete ack;

		state = terminal->addPacket(sa_packet, pdu);
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "New Slotted Aloha packet with ID %s received from terminal %u\n", 
		    id_packet.c_str(), tal_id);

		if(state == no_prop)
		{
			LOG(this->log_saloha, LEVEL_INFO,
			    "Received packet %s from ST%u, no complete PDU to propagate\n",
			    id_packet.c_str(), tal_id);
		}
		else
		{
			LOG(this->log_saloha, LEVEL_INFO,
			    "Complete PDU received from ST%u with ID %u\n",
			    tal_id, id_pdu);

			for(saloha_packets_data_t::iterator pdu_it = pdu.begin();
			    pdu_it != pdu.end(); ++pdu_it)
			{
				(*burst)->add(this->removeSalohaHeader(*pdu_it));
			}
		}
	}
	// NB: if a pdu is never completed, it will be overwritten once
	//     PDU id would have looped
	// add last frame in complete frames
	if(frame->getDataLength())
	{
		complete_dvb_frames.push_back((DvbFrame *)frame);
	}
	else
	{
		delete frame;
	}
	LOG(this->log_saloha, LEVEL_INFO,
	    "Slotted Aloha scheduled, there is now %zu complete frames to send\n",
	    complete_dvb_frames.size());
	return true;
}


NetPacket* SlottedAlohaNcc::removeSalohaHeader(SlottedAlohaPacketData *sa_packet)
{
	NetPacket* encap_packet;
	size_t length = sa_packet->getPayloadLength();
	
	encap_packet = this->pkt_hdl->build(sa_packet->getPayload(),
	                                    length,
	                                    0, 0, 0);

	delete sa_packet;
	return encap_packet;
}

void SlottedAlohaNcc::removeCollisions(TerminalCategorySaloha *category)
{
	// we remove collision per category as in the same category
	// we do as if there was only one big carrier
	uint16_t nbr;
	unsigned int slots_per_carrier = floor(category->getSlotsNumber() /
	                                       category->getCarriersNumber());
	map<unsigned int, Slot *> slots = category->getSlots();
	AlohaPacketComparator comparator(slots_per_carrier);
	saloha_packets_data_t *accepted_packets = category->getAcceptedPackets();

	nbr = this->algo->removeCollisions(slots,
	                                   accepted_packets);
	this->probe_collisions[category->getLabel()]->put(nbr);
	// Because of CRDSA algorithm for example, need to sort packets
	sort(accepted_packets->begin(), accepted_packets->end(), comparator);
}

void SlottedAlohaNcc::simulateTraffic(TerminalCategorySaloha *category)
{
	if(!this->simulation_traffic)
	{
		return;
	}

	unsigned int nb_packets;
	unsigned int nb_packets_per_tal;
	unsigned int nb_slots;
	unsigned int slot;
	unsigned int nb_tal;
	unsigned int slots_per_carrier = floor(category->getSlotsNumber() /
	                                       category->getCarriersNumber());
	// TODO choose mean nb_max_packet in conf
	uint16_t nb_max_packets = 10;

	nb_slots = round((category->getSlotsNumber() * this->simulation_traffic) / 100);
	nb_packets = nb_slots * this->nb_replicas;
	nb_tal = nb_packets / nb_max_packets;
	nb_packets_per_tal = nb_packets / nb_tal;
	LOG(this->log_saloha, LEVEL_ERROR,
	    "category %s, simulate %du%% = (%u slots * %u replicas) "
	    "= %u packets (%u / tal * %u)",
	    category->getLabel().c_str(), this->simulation_traffic,
	    nb_slots, this->nb_replicas, nb_packets,
	    nb_packets_per_tal, nb_tal);
	for(unsigned int cpt = 0; cpt < nb_tal; cpt++)
	{
		set<unsigned int> tmp;
		set<unsigned int> time_slots;
		set<unsigned int>::iterator it;

		// see SlottedAlohaTal
		tmp.clear();
		time_slots.clear();
		while(tmp.size() < nb_packets_per_tal)
		{
			slot = (rand() / (double)RAND_MAX) * slots_per_carrier;
			tmp.insert(slot);
		}
		for(it = tmp.begin(); it != tmp.end(); ++it)
		{
			unsigned int slot_id = *it;
			slot = (int)((rand() / (double)RAND_MAX) * category->getCarriersNumber()) *
			       slots_per_carrier + slot_id;
			time_slots.insert(slot);
		}
		for(it = time_slots.begin(); it != time_slots.end(); ++it)
		{
			unsigned int slot_id = *it;
			map<unsigned int, Slot *> slots = category->getSlots();
//			NetPacket encap_packet(Data(""), 0, "", 0, 0, 0, 0, 0);
			SlottedAlohaPacketData *sa_packet;
			sa_packet = new SlottedAlohaPacketData(Data(),
			                                       (saloha_pdu_id_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (time_ms_t)0);
			// no need to check here if id exists as we directly
			// get info from the map itself to get IDs
			slots[slot_id]->push_back(sa_packet);
		}
	}
}

bool SlottedAlohaNcc::addTerminal(tal_id_t tal_id)
{
	saloha_terminals_t::iterator it;
	it = this->terminals.find(tal_id);
	if(it == this->terminals.end())
	{
		TerminalContextSaloha *terminal;
		TerminalMapping<TerminalCategorySaloha>::const_iterator it;
		TerminalCategories<TerminalCategorySaloha>::const_iterator category_it;
		TerminalCategorySaloha *category;
		const FmtDefinitionTable modcod_def;

		if(tal_id >= BROADCAST_TAL_ID)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Try to add Slotted Aloha terminal context "
			    "for simulated terminal\n");
			return false;
		}

		// Find the associated category
		it = this->terminal_affectation.find(tal_id);
		if(it == this->terminal_affectation.end())
		{
			if(!this->default_category)
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "ST #%u cannot be handled by Slotted Aloha context, "
				    "there is no default category\n", tal_id);
				return false;
			}

			LOG(this->log_saloha, LEVEL_INFO,
			    "ST #%d is not affected to a category, using "
			    "default: %s\n", tal_id, 
			    this->default_category->getLabel().c_str());
			category = this->default_category;
		}
		else
		{
			category = (*it).second;
		}
		// check if the category is concerned by Slotted Aloha
		if(this->categories.find(category->getLabel()) == this->categories.end())
		{
			LOG(this->log_saloha, LEVEL_INFO,
			    "Terminal %u is not concerned by Slotted Aloha category\n", tal_id);
			return true;
		}

		terminal = new TerminalContextSaloha(tal_id);
		if(!terminal)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Cannot create terminal context for ST #%d\n",
			    tal_id);
			return false;
		}

		// Add the new terminal to the list
		this->terminals.insert(
			pair<unsigned int, TerminalContextSaloha *>(tal_id, terminal));

		// add terminal in category and inform terminal of its category
		category->addTerminal(terminal);
		terminal->setCurrentCategory(category->getLabel());
		LOG(this->log_saloha, LEVEL_NOTICE,
		    "Add terminal %u in category %s\n",
		    tal_id, category->getLabel().c_str());
	}
	else
	{
		// terminal already exists, consider it rebooted
		LOG(this->log_saloha, LEVEL_WARNING,
		    "Duplicate ST received with ID #%u\n", tal_id);
		return true;
	}

	return true;
}


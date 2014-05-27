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
 * @file SlottedAlohaNcc.cpp
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien BERNARD / Viveris Technologies
 */

#include "SlottedAlohaNcc.h"
#include "SlottedAlohaFrame.h"
#include "SlottedAlohaMethod.h"
#include "SlottedAlohaPacketCtrl.h"
#include "SlottedAlohaMethodDsa.h"
#include "SlottedAlohaMethodCrdsa.h"

#include <stdlib.h>
#include <math.h>

#include <opensand_conf/conf.h>


// functor for SlottedAlohaPacket comparison
//

SlottedAlohaNcc::SlottedAlohaNcc():
	SlottedAloha()
{
}

SlottedAlohaNcc::~SlottedAlohaNcc()
{
	for(SalohaTerminalList::iterator it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		delete it->second;
	}
	this->terminals.clear();

	delete this->method;
}

bool SlottedAlohaNcc::init(void)
{
	string method_name;

	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Parent 'init()' method must be called first.\n");
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_METHOD, method_name)) // TODO RENAME !
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_METHOD);
		return false;
	}
	if(!Conf::getValue(SALOHA_SECTION, SALOHA_SIMU_TRAFFIC,
	                          this->simulation_traffic))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_SIMU_TRAFFIC);
		return false;
	}

	if (method_name == "DSA")
	{
		this->method = new SlottedAlohaMethodDsa();
	}
	else if (method_name == "CRDSA")
	{
		this->method = new SlottedAlohaMethodCrdsa();
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to set Slotted Aloha '%s' algorithm\n", method_name.c_str());
		return false;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "initialize Slotted Aloha with %s algorithm\n",
	    method_name.c_str());

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
		SalohaTerminalList::iterator st;
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
		if (!sa_packet)
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
		// TODO
//		this->debug("< RCVD", sa_packet);
		// TODO move nb_packet _received in category ?
		this->nb_packets_received_total++;

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
		(*slot_it).second->addPacket(sa_packet);
		category->increaseReceivedPacketsNbr();
/*		for(unsigned int i = 0; i < sa_packet->getNbReplicas(); i++)
		{
			unsigned int slot_id = sa_packet->getReplica(i);

			slot_it = slots.find(slot_id);
			if(slot_it == slots.end())
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "replica received in slot %u that does not exist\n",
				    slot_id);
				continue;
			}
//			this->nb_packets_received_total++;
//			category->increaseReceivedPacketsNbr();
			(*slot_it).second->addPacket(sa_packet);
		}*/
	}
	// TODO stat nb_packets_received_per_frame
	//      nb_packet_received_total

skip:
	delete dvb_frame;
	return true;
}


bool SlottedAlohaNcc::schedule(NetBurst **burst,
                               list<DvbFrame *> &complete_dvb_frames,
                               time_sf_t superframe_counter)
{
	TerminalCategories<TerminalCategorySaloha>::const_iterator cat_iter;
	
	if(!this->isSuperFrameTick(superframe_counter))
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
//	this->debugFifo("after scheduling");
	return true;
}

bool SlottedAlohaNcc::scheduleCategory(TerminalCategorySaloha *category,
                                       NetBurst **burst,
                                       list<DvbFrame *> &complete_dvb_frames)
{
	SlottedAlohaFrameCtrl *frame;
	saloha_packets_t *accepted_packets;
	saloha_packets_t::iterator pkt_it;
	if(!category->getReceivedPacketsNbr())
	{
		LOG(this->log_saloha, LEVEL_INFO,
		    "No packet to schedule in category %s\n",
		    category->getLabel().c_str());
		return true;
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
	// TODO
//	this->debugFifo("before scheduling");
	LOG(this->log_saloha, LEVEL_DEBUG,
	    "Remove collisions on category %s\n",
	    category->getLabel().c_str());
	this->removeCollisions(category); // Call specific algorithm to remove collisions
	// TODO
//	this->debugFifo("after collisions removing");

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
		SlottedAlohaPacketData *sa_packet;
		SlottedAlohaPacketCtrl *ack;
		TerminalContextSaloha *terminal;
		SalohaTerminalList::iterator st;
		saloha_packets_t *wait_propagation_packets;
		saloha_id_t last_propagated_id;
		saloha_id_t id_packet;
		tal_id_t tal_id;
		qos_t qos;
		prop_state_t state;

		sa_packet = dynamic_cast<SlottedAlohaPacketData *>(*pkt_it);
		// erase goes to next iterator
		accepted_packets->erase(pkt_it);
		id_packet = this->buildPacketId(sa_packet);
		tal_id = sa_packet->getSrcTalId();
		qos = sa_packet->getQos();

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

		// TODO ENUM for state !
		last_propagated_id = terminal->getLastPropagatedIds(qos);
		state = this->canPropagate(last_propagated_id, id_packet, sa_packet);
		if(!sa_packet->getSrcTalId())
		{
			LOG(this->log_saloha, LEVEL_DEBUG,
			    "drop Slotted Aloha simulation packet\n");
			delete sa_packet;
			continue;
		}

		if(state == dup)
		{
			LOG(this->log_saloha, LEVEL_NOTICE,
			    "drop Slotted Aloha packet because of duplication or error\n");
			delete sa_packet;
			continue;
		}
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "New Slotted Aloha packet with ID %s received from terminal %u\n", 
		    id_packet.c_str(), sa_packet->getSrcTalId());

		// Send an ACK
		ack = new SlottedAlohaPacketCtrl(id_packet, SALOHA_CTRL_ACK);
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
		delete ack;
//		this->debug("SEND >", ack);
		wait_propagation_packets = terminal->getWaitPropagationPackets(qos);
		if(state == no_prop)
		{
			LOG(this->log_saloha, LEVEL_NOTICE,
			    "Packet %s, previous packet is missing, wait\n",
			    id_packet.c_str());
			// Store packet in waiting list because of previous packet missing
			wait_propagation_packets->push_back(sa_packet);
		}
		else
		{

			saloha_packets_t::iterator wait_pkt_it;
			terminal->setLastPropagatedIds(qos, id_packet);
//			this->debug("PROPAGATE", sa_packet);
			(*burst)->add(this->removeHeader(sa_packet));

			LOG(this->log_saloha, LEVEL_INFO,
			    "Propagate packet with ID %s\n", id_packet.c_str());
			//Propagate all waiting packet after receiving the missing packet
			/// TODO function, this is quite the same as the upper algo on accepted_packets
			wait_pkt_it = wait_propagation_packets->begin();
			while(wait_pkt_it != wait_propagation_packets->end())
			{
				sa_packet = dynamic_cast<SlottedAlohaPacketData *>(*wait_pkt_it);
				id_packet = this->buildPacketId(sa_packet);
				if((sa_packet->getSrcTalId() != terminal->getTerminalId()) ||
				   (sa_packet->getQos() != qos))
				{
					LOG(this->log_saloha, LEVEL_ERROR,
					    "Wrong packet data waiting for propagation\n");
					// erase goes to next packet
					wait_propagation_packets->erase(wait_pkt_it);
					continue;
				}

				state = this->canPropagate(last_propagated_id,
				                           id_packet,
				                           sa_packet);
				if(state == prop)
				{
					LOG(this->log_saloha, LEVEL_ERROR,
					    "Waiting packet with ID %s can also be propagated\n",
					    id_packet.c_str());
//					this->debug("PROPAGATE_bis", sa_packet);
					(*burst)->add(this->removeHeader(sa_packet));
					terminal->setLastPropagatedIds(qos, id_packet);
					// erase goes to next packet
					wait_propagation_packets->erase(wait_pkt_it);
					continue;
				}
				wait_pkt_it++;
			}
		}
	}
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


// dup     = packet duplicated or error (no propagation, no ACK)
// no_prop = packet cannot be propagated (no propagation but ACK)
// prop    = packet can be propagated (propagation and ACK)
SlottedAlohaNcc::prop_state_t
SlottedAlohaNcc::canPropagate(saloha_id_t last_propagated_id,
                              saloha_id_t id_packet,
                              SlottedAlohaPacketData *sa_packet)
{
	uint16_t id_last[4];
	uint16_t id;
	uint16_t seq;
	
	id = sa_packet->getId();
	seq = sa_packet->getSeq();
	this->convertPacketId(last_propagated_id, id_last);
	if(last_propagated_id.empty() && (!id) && (!seq))
	{
		goto success;
	}
	else if(last_propagated_id == id_packet)
	{
		goto drop;
	}
	else if((id_last[SALOHA_ID_ID] == id) &&
	        (id_last[SALOHA_ID_SEQ] == (seq - 1)) &&
	        (id_last[SALOHA_ID_PDU_NB] == (seq + 1)))
	{
		goto success;
	}
	else if((id_last[SALOHA_ID_ID] == (id - 1)) &&
	        (id_last[SALOHA_ID_PDU_NB] == (id_last[SALOHA_ID_SEQ] + 1)) &&
	        (seq == 0))
	{
		goto success;
	}
	else
	{
		goto error;
	}
drop:
	return dup;
error:
	return no_prop;
success:
	return prop;
}

NetPacket* SlottedAlohaNcc::removeHeader(SlottedAlohaPacketData *sa_packet)
{
	NetPacket* encap_packet;
	size_t length = sa_packet->getPayloadLength();
/*	qos_t qos = sa_packet->getQos();
	tal_id_t src_tal_id*/
	
	sa_packet->removeHeader();
	// TODO why not use EncapPktHdl to create the correct pkt type
	encap_packet = this->pkt_hdl->build(sa_packet->getData(),
	                                    length,
	                                    0, 0, 0);

	                                    
/*	encap_packet = new NetPacket(sa_packet->getData(),
	                             sa_packet->getTotalLength(),
	                             sa_packet->getName(),
	                             sa_packet->getType(),
	                             sa_packet->getQos(),
	                             sa_packet->getSrcTalId(),
	                             sa_packet->getDstTalId(),
	                             sa_packet->getHeaderLength());*/
	delete sa_packet;
	return encap_packet;
}

void SlottedAlohaNcc::removeCollisions(TerminalCategorySaloha *category)
{
	// we remove collision per category as in the same category
	// we do as if there was only one big carrier
/*	unsigned int slots_per_carrier = floor(category->getSlotsNumber() /
	                                       category->getCarriersNumber());*/
	map<unsigned int, Slot *> slots = category->getSlots();
//	AlohaPacketComparator comparator(slots_per_carrier);
	saloha_packets_t *accepted_packets = category->getAcceptedPackets();

	this->method->removeCollisions(slots,
	                               accepted_packets);
	// Because of CRDSA algorithm for example, need to sort packets
//	sort(accepted_packets.begin(), accepted_packets.end(), comparator);
	// TODO stat collision
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
			                                       (uint64_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t)0,
			                                       (uint16_t*)NULL);
			// no need to check here if id exists as we directly
			// get info from the map itself to get IDs
			slots[slot_id]->addPacket(sa_packet);
		}
	}
}

/*void SlottedAlohaNcc::debugFifo(const char* title)
{
	sa_vector_vector_data_t::iterator i1;
	sa_vector_data_t::iterator i2;
	sa_map_map_vector_data_t::iterator i3;
	sa_map_vector_data_t::iterator i4;
	sa_map_map_id_t::iterator i5;
	sa_map_id_t::iterator i6;
	SlottedAlohaPacketData* sa_packet;
	int cpt;
	
	if (!SALOHA_DEBUG)
		return;
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL , ---------------- %s ----------------------", title);
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL | fifos_from_input.size() = %d",
		(int)this->fifos_from_input.size());
	cpt = 0;
	for(i1 = this->fifos_from_input.begin();
		i1 != this->fifos_from_input.end();
		++i1)
	{
		if (i1->size())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "WKL |     fifos_from_input[Slot = %d].size() = %d",
				cpt, (int)i1->size());
			for(i2 = i1->begin();
				i2 != i1->end();
				++i2)
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "WKL |         Pkt[#%d]", (int)*i2);
			}
		}
		cpt++;
	}
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL | fifo_to_encap.size() = %d",
		(int)this->fifo_to_encap.size());
	for(i2 = this->fifo_to_encap.begin();
		i2 != fifo_to_encap.end();
		++i2)
	{
		sa_packet = dynamic_cast<SlottedAlohaPacketData*>(*i2);
		if (sa_packet->getSrcTalId())
			LOG(this->log_saloha, LEVEL_ERROR,
			    "WKL |     Pkt[#%d]", (int)*i2);
	}
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL | last_propagated.size() = %d",
		(int)last_propagated_ids.size());
	for(i5 = last_propagated_ids.begin();
		i5 != last_propagated_ids.end();
		++i5)
	{
		if (i5->second.size())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "WKL |     last_propagated[ST = %d].size() = %d",
				i5->first, (int)i5->second.size());
			for(i6 = i5->second.begin();
				i6 != i5->second.end();
				++i6)
			{
				if (i6->second.size())
				{
					LOG(this->log_saloha, LEVEL_ERROR,
					    "WKL |         last_propagated[ST = %d][QoS = %d] = '%s'",
						i5->first, i6->first, i6->second.c_str());
				}
			}
		}
	}
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL | fifos_wait_propagation.size() = %d",
		(int)wait_propagation_packets.size());
	for(i3 = wait_propagation_packets.begin();
		i3 != wait_propagation_packets.end();
		++i3)
	{
		if (i3->second.size())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "WKL |     fifos_wait_propagation[ST = %d].size() = %d",
				i3->first, (int)i3->second.size());
			for(i4 = i3->second.begin();
				i4 != i3->second.end();
				++i4)
			{
				if (i4->second.size())
				{
					LOG(this->log_saloha, LEVEL_ERROR,
					    "WKL |         fifos_wait_propagation"
						"[ST = %d][QoS = %d].size() = %d",
						i3->first, i4->first, (int)i4->second.size());
					for(i2 = i4->second.begin();
						i2 != i4->second.end();
						++i2)
					{
						LOG(this->log_saloha, LEVEL_ERROR,
						    "WKL |             Pkt[#%d]", (int)*i2);
					}
				}
			}
		}
	}
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL '--------------------------------------------------------");
}
*/

bool SlottedAlohaNcc::addTerminal(tal_id_t tal_id)
{
	SalohaTerminalList::iterator it;
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

		// TODO register probes ?

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


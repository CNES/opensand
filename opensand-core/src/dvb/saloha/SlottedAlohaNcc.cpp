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
#include "OpenSandModelConf.h"

#include <stdlib.h>
#include <math.h>


// functor for SlottedAlohaPacket comparison
//

SlottedAlohaNcc::SlottedAlohaNcc():
	SlottedAloha(),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	spot_id(0),
	terminals(),
	algo(NULL),
	simu()
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

	for(std::vector<SlottedAlohaSimu *>::iterator it = this->simu.begin();
	    it != this->simu.end(); ++it)
	{
		delete *it;
	}

	delete this->algo;
}

void SlottedAlohaNcc::generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane)
{
	auto Conf = OpenSandModelConf::Get();

	auto types = Conf->getModelTypesDefinition();
	types->addEnumType("saloha_algo", "Slotted Aloha Algorithm", {"DSA", "CRDSA"});
	types->addEnumType("traffic_type", "Simulated Slotted Aloha Traffic", {"Standard", "Premium", "Professional", "SVNO1", "SVNO2", "SVNO3", "SNO"});

	auto conf = Conf->getOrCreateComponent("access", "Access");
	Conf->setProfileReference(conf, disable_ctrl_plane, false);
	auto saloha = conf->getOrCreateComponent("random_access", "Random Access");
	saloha->addParameter("saloha_algo", "Slotted Aloha Algorithm", types->getType("saloha_algo"));
	auto simu_list = conf->addList("simulations", "Simulated traffic", "simulation")->getPattern();
	simu_list->addParameter("category", "Category", types->getType("traffic_type"));
	simu_list->addParameter("max_packets", "Max Packets", types->getType("int"))->setUnit("packets");
	simu_list->addParameter("replicas", "Replicas", types->getType("int"))->setUnit("packets");
	simu_list->addParameter("ratio", "Ratio", types->getType("int"));
}

bool SlottedAlohaNcc::init(TerminalCategories<TerminalCategorySaloha> &categories,
                           TerminalMapping<TerminalCategorySaloha> terminal_affectation,
                           TerminalCategorySaloha *default_category,
                           spot_id_t spot_id,
                           UnitConverter *converter)

{
  std::string algo_name;
	TerminalCategories<TerminalCategorySaloha>::const_iterator cat_iter;
	auto conf = OpenSandModelConf::Get()->getProfileData()->getComponent("access");

	// set spot id
	if(spot_id == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
			"wrong spot id = %u", spot_id);
	}
	this->spot_id = spot_id;

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

	auto output = Output::Get();
	for(cat_iter = this->categories.begin(); cat_iter != this->categories.end();
	    ++cat_iter)
	{
		TerminalCategorySaloha *cat = (*cat_iter).second;
		cat->computeSlotsNumber(converter);
		std::shared_ptr<Probe<int>> probe_coll;
		std::shared_ptr<Probe<int>> probe_coll_before;
		std::shared_ptr<Probe<int>> probe_coll_ratio;
		char probe_name[128];

		snprintf(probe_name, sizeof(probe_name), "Aloha.collisions.%s", (*cat_iter).first.c_str());
		probe_coll = output->registerProbe<int>(probe_name, true, SAMPLE_SUM);
		// disable by default
		snprintf(probe_name, sizeof(probe_name), "Aloha.collisions.before_algo.%s", (*cat_iter).first.c_str());
		probe_coll_before = output->registerProbe<int>(probe_name, false, SAMPLE_SUM);
		// disable by default
		snprintf(probe_name, sizeof(probe_name), "Aloha.collisions_ratio.%s", (*cat_iter).first.c_str());
		probe_coll_ratio = output->registerProbe<int>(probe_name, "%", false, SAMPLE_AVG);

		this->probe_collisions_before.emplace(cat_iter->first, probe_coll_before);
		this->probe_collisions.emplace(cat_iter->first, probe_coll);
		this->probe_collisions_ratio.emplace(cat_iter->first, probe_coll_ratio);
	}

	if(!OpenSandModelConf::extractParameterData(conf->getComponent("random_access")->getParameter("saloha_algo"),
	                                            algo_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'slotted aloha algorithm'\n");
		return false;
	}

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

	// load Slotted Aloha traffic simulation parameters
	for(auto& item : conf->getList("simulations")->getItems())
	{
		auto simulated_traffic = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);

		std::string label;
		if(!OpenSandModelConf::extractParameterData(simulated_traffic->getParameter("category"), label))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get category from section 'access, simulated traffic'\n");
			return false;
		}

		int max_packets;
		if(!OpenSandModelConf::extractParameterData(simulated_traffic->getParameter("max_packets"), max_packets))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get max packets from section 'access, simulated traffic'\n");
			return false;
		}
		uint16_t nb_max_packets = max_packets;

		int replicas;
		if(!OpenSandModelConf::extractParameterData(simulated_traffic->getParameter("replicas"), replicas))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get replicas count from section 'access, simulated traffic'\n");
			return false;
		}
		uint16_t nb_replicas = replicas;

		int raw_ratio;
		if(!OpenSandModelConf::extractParameterData(simulated_traffic->getParameter("ratio"), raw_ratio))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get ratio from section 'access, simulated traffic'\n");
			return false;
		}
		uint8_t ratio = raw_ratio;

		// FIXME: as in manager we need at least one element in a table
		//        to add a new line, we will have at least one line here.
		//        So this is a way to ignore it
		// TODO: This is fixed with the new libconf, should we get rid of this check?
		if(nb_max_packets == 0)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "Slotted Aloha simulation parameters for category %s "
			    "with 0 maximum packets: ignored\n", label.c_str());
			continue;
		}

		cat_iter = this->categories.find(label);
		if(cat_iter == this->categories.end())
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Slotted Aloha simulation parameters for category %s "
			    "that does not contain Slotted Aloha carriers\n",
			    label.c_str());
			continue;
		}
		
		SlottedAlohaSimu *simulation = new SlottedAlohaSimu(cat_iter->second,
		                                                    nb_max_packets,
		                                                    nb_replicas,
		                                                    ratio);
		this->simu.push_back(simulation);
	}

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
    delete dvb_frame;
    return true;
	}	

	LOG(this->log_saloha, LEVEL_INFO,
	    "Receive Slotted Aloha frame containing %u packets\n",
	    frame->getDataLength());

	previous_length = 0;
	for(unsigned int cpt = 0; cpt < frame->getDataLength(); cpt++)
	{
		Data payload = frame->getPayload(previous_length);
		size_t current_length =
			SlottedAlohaPacketData::getPacketLength(payload);

		previous_length += current_length;
    std::unique_ptr<SlottedAlohaPacketData> sa_packet;
    try
    {
      sa_packet = std::unique_ptr<SlottedAlohaPacketData>(new SlottedAlohaPacketData{payload, current_length});
    }
		catch (const std::bad_alloc&)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "cannot create a Slotted Aloha data packet\n");
			continue;
		}
		// we need to keep qos and src_tal_id of inner encapsulated packet
		qos_t qos;
		tal_id_t src_tal_id;
		Data encap = sa_packet->getPayload();
		this->pkt_hdl->getSrc(encap, src_tal_id); 
		this->pkt_hdl->getQos(encap, qos); 
		sa_packet->setSrcTalId(src_tal_id);
		sa_packet->setQos(qos);

		// find the associated terminal category
		auto st = this->terminals.find(src_tal_id);
		if(st == this->terminals.end())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Slotted Aloha packet received from unknown terminal %u\n",
			    src_tal_id);
			continue;
		}
		auto terminal = st->second;
		auto category = this->categories[terminal->getCurrentCategory()];

		// Add replicas in the corresponding slots
		auto slots = category->getSlots();
		auto slot_it = slots.find(sa_packet->getTs());
		if(slot_it == slots.end())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "packet received on a slot that does not exist\n");
			continue;
		}
		slot_it->second->push_back(std::move(sa_packet));
		category->increaseReceivedPacketsNbr();
	}

	delete dvb_frame;
	return true;
}


bool SlottedAlohaNcc::schedule(NetBurst **burst,
                               std::list<DvbFrame *> &complete_dvb_frames,
                               time_sf_t superframe_counter)
{
	TerminalCategories<TerminalCategorySaloha>::const_iterator cat_iter;
	
	if(!this->isSalohaFrameTick(superframe_counter))
	{
		return true;
	}
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
                                       std::list<DvbFrame *> &complete_dvb_frames)
{
	SlottedAlohaFrameCtrl *frame;
	saloha_packets_data_t *accepted_packets;
	saloha_packets_data_t::iterator pkt_it;
	// refresh the probe in case of no traffic
	this->probe_collisions[category->getLabel()]->put(0);
	this->probe_collisions_before[category->getLabel()]->put(0);
	this->probe_collisions_ratio[category->getLabel()]->put(0);
	if(!category->getReceivedPacketsNbr())
	{
		LOG(this->log_saloha, LEVEL_DEBUG,
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

	for(std::vector<SlottedAlohaSimu *>::iterator it = this->simu.begin();
	    it != this->simu.end(); ++it)
	{
		if((*it)->getCategory() == category->getLabel())
		{
			this->simulateTraffic(category, *it);
		}
	}

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
	frame->setSpot(this->spot_id);

	LOG(this->log_saloha, LEVEL_DEBUG,
	    "Schedule Slotted Aloha packets\n");

	// Propagate if possible all packets received to encap block
	accepted_packets = category->getAcceptedPackets();
	pkt_it = accepted_packets->begin();
	while(pkt_it != accepted_packets->end())
	{
    std::unique_ptr<SlottedAlohaPacketData> sa_packet = std::move(*pkt_it);
		SlottedAlohaPacketCtrl *ack;
		TerminalContextSaloha *terminal;
		saloha_terminals_t::iterator st;
		saloha_pdu_id_t id_pdu;
		saloha_id_t id_packet;
		tal_id_t tal_id;

		// erase goes to next iterator
		accepted_packets->erase(pkt_it);
		id_packet = sa_packet->getUniqueId();
		id_pdu = sa_packet->getId();
		tal_id = sa_packet->getSrcTalId();

		if(tal_id > BROADCAST_TAL_ID)
		{
			LOG(this->log_saloha, LEVEL_DEBUG,
			    "drop Slotted Aloha simulation packet\n");
			continue;
		}

		st = this->terminals.find(tal_id);
		if(st == this->terminals.end())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Cannot find terminal %u associated with packet\n",
			    tal_id);
			continue;
		}
		terminal = st->second;
		if(terminal->getCurrentCategory() != category->getLabel())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Wrong category %s for packet with source terminal ID %u\n",
			    category->getLabel().c_str(), tal_id);
			continue;
		}

		// Send an ACK
		ack = new SlottedAlohaPacketCtrl(id_packet, SALOHA_CTRL_ACK, tal_id);
		if(!ack)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "failed to create a Slotted Aloha signal control "
			    "packet");
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
			frame->setSpot(this->spot_id);
		}
		if(!frame->addPacket(ack))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "failed to add a Slotted Aloha packet in "
			    "signal control frame");
			delete ack;
			continue;
		}
		LOG(this->log_saloha, LEVEL_INFO,
		    "Ack packet %s on ST%u\n", id_packet.c_str(), tal_id);
		delete ack;

		saloha_packets_data_t pdu;
		auto state = terminal->addPacket(std::move(sa_packet), pdu);
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "New Slotted Aloha packet with ID %s received from terminal %u\n", 
		    id_packet.c_str(), tal_id);

		if(state == PropagateState::NoPropagation)
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

			for(auto&& packet_in_pdu : pdu)
			{
				(*burst)->add(this->removeSalohaHeader(std::move(packet_in_pdu)));
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


std::unique_ptr<NetPacket> SlottedAlohaNcc::removeSalohaHeader(std::unique_ptr<SlottedAlohaPacketData> sa_packet)
{
  std::size_t length = sa_packet->getPayloadLength();
	return this->pkt_hdl->build(sa_packet->getPayload(), length, 0, 0, 0);
}


void SlottedAlohaNcc::removeCollisions(TerminalCategorySaloha *category)
{
	// we remove collision per category as in the same category
	// we do as if there was only one big carrier
	uint16_t nbr;
	unsigned int slots_per_carrier = floor(category->getSlotsNumber() /
	                                       category->getCarriersNumber());
  std::map<unsigned int, Slot *> slots = category->getSlots();
	AlohaPacketComparator comparator(slots_per_carrier);
	saloha_packets_data_t *accepted_packets = category->getAcceptedPackets();

	if(this->probe_collisions_before[category->getLabel()]->isEnabled())
	{
		uint16_t coll = 0;
		for(std::map<unsigned int, Slot *>::iterator slot_it = slots.begin();
		    slot_it != slots.end(); ++slot_it)
		{
			Slot *slot = (*slot_it).second;
			if(slot->size() > 1)
			{
				coll += slot->size();
			}
		}
		this->probe_collisions_before[category->getLabel()]->put(coll);
	}
	nbr = this->algo->removeCollisions(slots, accepted_packets);
	this->probe_collisions[category->getLabel()]->put(nbr);
	this->probe_collisions_ratio[category->getLabel()]->put(nbr * 100 /
	                                                        category->getSlotsNumber());
	// Because of CRDSA algorithm for example, need to sort packets
	sort(accepted_packets->begin(), accepted_packets->end(), comparator);
}

void SlottedAlohaNcc::simulateTraffic(TerminalCategorySaloha *category,
                                      const SlottedAlohaSimu *simulation)
{
	for(unsigned int cpt = 0; cpt < simulation->getNbTal(); cpt++)
	{
		saloha_ts_list_t tmp;
		saloha_ts_list_t time_slots;
		saloha_ts_list_t::iterator it;
		uint16_t slots_per_carrier;
		uint16_t slot;
		uint16_t pdu_id;

		slots_per_carrier = floor(category->getSlotsNumber() /
		                          category->getCarriersNumber());

		// see SlottedAlohaTal
		tmp.clear();
		time_slots.clear();
		while(tmp.size() < simulation->getNbPacketsPerTal())
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


		pdu_id = 0;
		it = time_slots.begin();
		while(it != time_slots.end())
		{
      std::map<unsigned int, Slot *> slots = category->getSlots();
			uint16_t nb_replicas = simulation->getNbReplicas();
			uint16_t replicas[nb_replicas];
			for(uint16_t rep_cpt = 0; rep_cpt < nb_replicas; rep_cpt++)
			{
				replicas[rep_cpt] = *it;
				it++;
			}
 
			for(uint16_t rep_cpt = 0; rep_cpt < nb_replicas; rep_cpt++)
			{
				uint16_t slot_id = replicas[rep_cpt];
				// we need a PDU ID else removeCollision will consider all
				// packets the same, this will mislead CRDSA algorithm
				auto sa_packet = std::unique_ptr<SlottedAlohaPacketData>(
            new SlottedAlohaPacketData(Data(),
				                               (saloha_pdu_id_t)pdu_id,
				                               (uint16_t)0,
				                               (uint16_t)0,
				                               (uint16_t)0,
				                               nb_replicas,
				                               (time_ms_t)0));
				// as for request simulation use tal id > BROADCAST_TAL_ID
				// used for filtering
				sa_packet->setSrcTalId(BROADCAST_TAL_ID + 1 + cpt);
				sa_packet->setReplicas(replicas, nb_replicas);
				sa_packet->setTs(slot_id);
				// no need to check here if id exists as we directly
				// get info from the map itself to get IDs
				slots[slot_id]->push_back(std::move(sa_packet));
			}
			pdu_id++;
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
			if(category == NULL)
			{
				LOG(this->log_saloha,LEVEL_INFO,
				    "Terminal %d do not use SALOHA", tal_id);
				return true;
			}
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
			std::pair<unsigned int, TerminalContextSaloha *>(tal_id, terminal));

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


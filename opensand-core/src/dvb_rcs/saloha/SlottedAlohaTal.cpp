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
 * @file SlottedAlohaTal.cpp
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#include "SlottedAlohaTal.h"
#include "SlottedAlohaBackoffBeb.h"
#include "SlottedAlohaBackoffEied.h"
#include "SlottedAlohaBackoffMimd.h"
#include "SlottedAlohaPacketCtrl.h"

#include <opensand_conf/conf.h>


SlottedAlohaTal::SlottedAlohaTal():
	SlottedAloha(),
	tal_id(),
	timeout_saf(),
	packets_wait_ack(),
	nb_success(0),
	nb_max_packets(0),
	nb_max_retransmissions(0),
	base_id(0),
	backoff(NULL),
	category(NULL),
	dvb_fifos()
{
}

bool SlottedAlohaTal::init(tal_id_t tal_id,
                           TerminalCategorySaloha *category,
                           const fifos_t &dvb_fifos)
{
	uint16_t max;
	uint16_t multiple;
	time_ms_t sat_delay_ms;
	time_ms_t timeout_ms;
	time_ms_t min_timeout_ms;
	string backoff_name;

	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Parent 'init()' method must be called first.\n");
		return false;
	}

	this->tal_id = tal_id;
	this->category = category;
    this->category->setSlotsNumber(this->frame_duration_ms,
                                   this->pkt_hdl->getFixedLength());

	this->dvb_fifos = dvb_fifos;

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_NB_MAX_PACKETS, this->nb_max_packets))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_NB_MAX_PACKETS);
		return false;
	}
	if(!Conf::getValue(SALOHA_SECTION, SALOHA_NB_REPLICAS, this->nb_replicas))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_NB_REPLICAS);
		return false;
	}

	/// check nb_max_packets
	//  we limit maximum to the number of slots per carrier to avoid two packets to
	//  be sent on the same slot but at different frequencies
	//  (we may have different slots depending on band and carriers but here we
	//   do as if all carriers and slots were the same, this is a convenient approx)
	if((unsigned)(this->nb_max_packets * this->nb_replicas) >
	   (this->category->getSlotsNumber() / this->category->getCarriersNumber()))
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "Maximum packet per Slotted Aloha frames is bigger than "
		    "slots number !\n");
		this->nb_max_packets = this->category->getSlotsNumber() /
		                       (this->nb_replicas * this->category->getCarriersNumber());
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_TIMEOUT, this->timeout_saf))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_TIMEOUT);
		return false;
	}
	if(!Conf::getValue(GLOBAL_SECTION, SAT_DELAY, sat_delay_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, SAT_DELAY);
		return false;
	}

	timeout_ms = this->timeout_saf * this->frame_duration_ms * this->sf_per_saframe;
	min_timeout_ms = 2 * sat_delay_ms + this->sf_per_saframe * this->frame_duration_ms;
	if (timeout_ms <= min_timeout_ms)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Slotted Aloha timeout too low ! (%u < %u)",
		    timeout_ms, min_timeout_ms);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_NB_MAX_RETRANSMISSIONS,
	                   this->nb_max_retransmissions))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_NB_MAX_RETRANSMISSIONS);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_BACKOFF_ALGORITHM, backoff_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_BACKOFF_ALGORITHM);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_CW_MAX, max))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_CW_MAX);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_BACKOFF_MULTIPLE, multiple))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_BACKOFF_MULTIPLE);
		return false;
	}

	if(backoff_name == "BEB")
	{
		this->backoff = new SlottedAlohaBackoffBeb(max, multiple);
	}
	else if(backoff_name == "EIED")
	{
		this->backoff = new SlottedAlohaBackoffEied(max, multiple);
	}
	else if(backoff_name == "MIMD")
	{
		this->backoff = new SlottedAlohaBackoffMimd(max, multiple);
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize Slotted Aloha '%s' backoff",
		    backoff_name.c_str());
		return false;
	}

	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() != cr_saloha)
		{
			continue;
		}
		Probe<int> *probe_ret;
		Probe<int> *probe_wait;
		Probe<int> *probe_nb_drop;

		probe_ret = Output::registerProbe<int>(true, SAMPLE_SUM,
		                                       "Aloha.retransmissions.%s",
		                                       (*it).second->getName().c_str());
		probe_wait = Output::registerProbe<int>(true, SAMPLE_LAST,
		                                       "Aloha.wait.%s",
		                                       (*it).second->getName().c_str());
		probe_nb_drop = Output::registerProbe<int>(true, SAMPLE_SUM,
		                                       "Aloha.drops.%s",
		                                       (*it).second->getName().c_str());
		this->probe_retransmission.insert(
		            pair<qos_t, Probe<int> *>((*it).first, probe_ret));
		this->probe_wait_ack.insert(
		            pair<qos_t, Probe<int> *>((*it).first, probe_wait));
		this->probe_drop.insert(
		            pair<qos_t, Probe<int> *>((*it).first, probe_nb_drop));
	}
	this->probe_backoff = Output::registerProbe<int>(true, SAMPLE_MAX,
	                                                 "Aloha.backoff");

	return true;
}

SlottedAlohaTal::~SlottedAlohaTal()
{
	delete this->backoff;
}

SlottedAlohaPacketData *SlottedAlohaTal::addSalohaHeader(NetPacket *encap_packet,
                                                         uint16_t offset,
                                                         uint16_t burst_size)
{
	SlottedAlohaPacketData *sa_packet;

	sa_packet = new SlottedAlohaPacketData(encap_packet->getData(),
	                                       this->base_id,     // id
	                                       0,                 // ts - set after initialization
	                                       offset,            // seq
	                                       burst_size,        // pdu_nb
	                                       this->nb_replicas, // nb_replicas
	                                       this->timeout_saf);
	sa_packet->setSrcTalId(encap_packet->getSrcTalId());
	sa_packet->setQos(encap_packet->getQos());
	LOG(this->log_saloha, LEVEL_DEBUG,
	    "New Slotted ALoha packet of size %zu, source terminal = %u, qos = %u\n",
	    sa_packet->getTotalLength(), sa_packet->getSrcTalId(), sa_packet->getQos());
	delete encap_packet;
	if(offset == (burst_size - 1))
	{
		this->base_id++;
	}
	return sa_packet;
}

bool SlottedAlohaTal::onRcvFrame(DvbFrame *dvb_frame)
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
	    "New Slotted Aloha frame containing %u packets\n",
	    frame->getDataLength());

	previous_length = 0;
	for(unsigned int cpt = 0; cpt < frame->getDataLength(); cpt++)
	{
		SlottedAlohaPacketCtrl *ctrl_pkt;
		Data payload = frame->getPayload(previous_length);
		size_t current_length =
			SlottedAlohaPacketCtrl::getPacketLength(payload);

		ctrl_pkt = new SlottedAlohaPacketCtrl(payload.c_str(),
		                                      current_length);
		previous_length += current_length;
		if(!ctrl_pkt)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "cannot create a Slotted Aloha control packet\n");
			continue;
		}
		if(ctrl_pkt->getTerminalId() != this->tal_id)
		{
			// control packet for another terminal
			delete ctrl_pkt;
			continue;
		}

		switch(ctrl_pkt->getCtrlType())
		{
			case SALOHA_CTRL_ACK:
			{
				uint16_t ids[4];
				bool dup = true;
				saloha_packets_data_t::iterator packet;
				saloha_id_t id = ctrl_pkt->getId();
				delete ctrl_pkt;

				SlottedAlohaPacket::convertPacketId(id, ids);
				packet = this->packets_wait_ack[ids[SALOHA_ID_QOS]].begin();
				LOG(this->log_saloha, LEVEL_DEBUG,
				    "ACK received for packet with ID %s\n",
				    id.c_str());
				while(packet != this->packets_wait_ack[ids[SALOHA_ID_QOS]].end())
				{
					SlottedAlohaPacketData *data_pkt = *packet;
					saloha_id_t data_id;
					data_id = data_pkt->getUniqueId();
					if(id == data_id)
					{
						uint16_t cw;
						LOG(this->log_saloha, LEVEL_DEBUG,
						    "Packet with ID %s found in packets waiting for ack "
						    "and removed\n", data_id.c_str());
						delete data_pkt;
						this->nb_success++;
						cw = this->backoff->setReady();
						this->probe_backoff->put(cw);
						this->packets_wait_ack[ids[SALOHA_ID_QOS]].erase(packet);
						dup = false;
						break;
					}
					packet++;
				}
				if(dup)
				{
					LOG(this->log_saloha, LEVEL_NOTICE,
					    "Potentially duplicated ACK received for ID %s\n",
					    id.c_str());
				}
				break;
			}
			//NB: Possibility to add new control signals
			default:
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to get a Slotted Aloha signal control packet "
				    "(unknown type %u)", ctrl_pkt->getCtrlType());
			}
		}
	}

skip:
	delete dvb_frame;
	return true;
}

bool SlottedAlohaTal::schedule(list<DvbFrame *> &complete_dvb_frames,
                               time_sf_t sf_counter)
{
	uint16_t nb_retransmissions;
	SlottedAlohaPacketData *sa_packet;
	map<qos_t, saloha_packets_data_t>::iterator wack_it;
	saloha_packets_data_t::iterator packet;
	SlottedAlohaFrame *frame;
	saloha_ts_list_t ts;
	saloha_ts_list_t::iterator i_ts;
	uint16_t nbr_packets = 0;
	uint16_t nbr_packets_total = 0;

	if(!this->isSalohaFrameTick(sf_counter))
	{
		goto skip;
	}
	this->backoff->tick();
	nb_retransmissions = 0;
	// Decrease timeout of waiting packets
	// We do that here because we may skip depending on backoff
	for(wack_it = this->packets_wait_ack.begin();
	    wack_it != this->packets_wait_ack.end();
	    ++wack_it)
	{
		for(packet = wack_it->second.begin();
		    packet != wack_it->second.end();
		    ++packet)
		{
			sa_packet = *packet;
			sa_packet->decTimeout();
		}
	}

	if(!this->backoff->isReady())
	{
		goto skip;
	}

	if(complete_dvb_frames.size())
	{
		LOG(this->log_saloha, LEVEL_INFO,
		    "Schedule Slotted Aloha packets, %zu complete frames at the moment\n",
		    complete_dvb_frames.size());
	}

	// If waiting packets can be retransmitted, store them in retransmission_packets
	for(wack_it = this->packets_wait_ack.begin();
	    wack_it != this->packets_wait_ack.end();
	    ++wack_it)
	{
		packet = (*wack_it).second.begin();
		while(packet != (*wack_it).second.end())
		{
			sa_packet = *packet;
			if(sa_packet->isTimeout())
			{
				if(sa_packet->canBeRetransmitted(this->nb_max_retransmissions))
				{
					LOG(this->log_saloha, LEVEL_NOTICE,
					    "Packet %s not acked, will be retransmitted\n",
					    sa_packet->getUniqueId().c_str());
					sa_packet->incNbRetransmissions();
					sa_packet->setTimeout(this->timeout_saf);
					this->retransmission_packets.insert(
						this->retransmission_packets.begin() + nb_retransmissions,
						sa_packet);
					nb_retransmissions++;
				}
				else
				{
					uint16_t cw;
					LOG(this->log_saloha, LEVEL_WARNING,
					    "Packet %s lost\n",
					    sa_packet->getUniqueId().c_str());
					this->probe_drop[sa_packet->getQos()]->put(1);
					delete sa_packet;
					cw = this->backoff->setCollision();
					this->probe_backoff->put(cw);
				}
				// erase goes to next iterator
				wack_it->second.erase(packet);
				continue;
			}
			packet++;
		}
	}

	if(nb_retransmissions)
	{
		LOG(this->log_saloha, LEVEL_NOTICE,
		    "%u packets added in retransmission FIFOs\n",
		    nb_retransmissions);
		this->nb_success = 0;
	}

	frame = new SlottedAlohaFrameData();
	if(!frame)
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to create a Slotted Aloha data frame");
		goto error;
	}
	ts = this->getTimeSlots(); // Get random unique time slots
	i_ts = ts.begin();
	// Send packets which can be retransmitted (high priority)
	packet = this->retransmission_packets.begin();
	while(packet != this->retransmission_packets.end() &&
	      nbr_packets_total < ts.size())
	{
		sa_packet = *packet;
		qos_t qos = sa_packet->getQos();

		if(!this->addPacketInFrames(complete_dvb_frames,
		                            &frame, sa_packet,
		                            i_ts, qos))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "failed to add a Slotted Aloha packet in data frame");
			packet++;
			continue;
		}
		this->probe_retransmission[qos]->put(1);
		// erase goes to next iterator
		this->retransmission_packets.erase(packet);
		nbr_packets++;
		nbr_packets_total++;
	}
	if(nbr_packets)
	{
		LOG(this->log_saloha, LEVEL_INFO,
		    "%u retransmission packets added to Slotted Aloha frames\n",
		    nbr_packets);
		nbr_packets = 0;
	}

	// Send new packets (low priority)
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		// the allocated slot limits the capacity
		if(nbr_packets_total >= ts.size())
		{
			break;
		}

		qos_t qos = (*it).first;
		DvbFifo *fifo = (*it).second;
		if(fifo->getCrType() != cr_saloha)
		{
			continue;
		}
		while(fifo->getCurrentSize() &&
		      nbr_packets_total < ts.size())
		{
			MacFifoElement *elem;
			elem = fifo->pop();
			sa_packet = elem->getElem<SlottedAlohaPacketData>();

			if(!this->addPacketInFrames(complete_dvb_frames,
			                            &frame, sa_packet,
			                            i_ts, qos))
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to add a Slotted Aloha packet in data frame");
				continue;
			}
			delete elem;
			nbr_packets++;
			nbr_packets_total++;
		}
		if(nbr_packets)
		{
			LOG(this->log_saloha, LEVEL_INFO,
			    "%u packets added to Slotted Aloha frames from %s fifo\n",
			    nbr_packets, fifo->getName().c_str());
			nbr_packets = 0;
		}
	}
	// add last frame in complete_dvb_frames
	if(frame->getDataLength())
	{
		complete_dvb_frames.push_back((DvbFrame *)frame);
	}
	else
	{
		delete frame;
	}
	if(complete_dvb_frames.size())
	{
		LOG(this->log_saloha, LEVEL_INFO,
		    "Slotted Aloha scheduled, there is now %zu complete frames to send\n",
		    complete_dvb_frames.size());
	}

skip:
	for(wack_it = this->packets_wait_ack.begin();
	    wack_it != this->packets_wait_ack.end();
	    ++wack_it)
	{
		this->probe_wait_ack[(*wack_it).first]->put((*wack_it).second.size());
	}

	// keep the probes refreshing
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() != cr_saloha)
		{
			continue;
		}
		this->probe_retransmission[(*it).first]->put(0);
		this->probe_drop[(*it).first]->put(0);
	}
	return true;
error:
	return false;
}

saloha_ts_list_t SlottedAlohaTal::getTimeSlots(void)
{
	saloha_ts_list_t tmp;
	saloha_ts_list_t time_slots;
	uint16_t max;
	uint16_t nb_packets;
	uint16_t slot;
	saloha_ts_list_t::iterator id;
	// slots per carrier is a mean because we may have carriers groups
	// with different parameters
	unsigned int slots_per_carrier = floor(this->category->getSlotsNumber() /
	                                       this->category->getCarriersNumber());

	nb_packets = this->retransmission_packets.size();
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_saloha)
		{
			nb_packets += (*it).second->getCurrentSize();
		}
	}
	max = min(nb_packets, this->nb_max_packets) * this->nb_replicas;

	if(max)
	{
		LOG(this->log_saloha, LEVEL_INFO,
		    "Compute timeslots, %u packets to send\n", max / this->nb_replicas);
	}

	// First step: generate random unique time slots about number of slots for
	//             one carrier (to keep concept of chronology)
	while(tmp.size() < max)
	{
		slot = (rand() / (double)RAND_MAX) * slots_per_carrier;
		tmp.insert(slot);
	}
	// Second step: calculate a random position between carriers, to simulate
	//              frequency changes
	for(id = tmp.begin(); id != tmp.end(); ++id)
	{
		slot = (int)((rand() / (double)RAND_MAX) * this->category->getCarriersNumber()) *
		       slots_per_carrier + (*id);
		time_slots.insert(slot);
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "Add random time slot %u\n", slot);
	}
	// time slots is a ordonned set
	return time_slots;
}

bool SlottedAlohaTal::addPacketInFrames(list<DvbFrame *> &complete_dvb_frames,
                                        SlottedAlohaFrame **frame,
                                        SlottedAlohaPacketData *packet,
                                        saloha_ts_list_t::iterator &slot,
                                        qos_t qos)
{
	uint16_t nbr_replicas = packet->getNbReplicas();
	uint16_t replicas[nbr_replicas];
	
	// in this function, the iterator on slots can be increased by nbr_replicas
	// because slots has been computed accordingly
	for(unsigned int cpt = 0; cpt < nbr_replicas; cpt++)
	{
		replicas[cpt] = *slot;
		slot++;
	}
	packet->setReplicas(replicas, nbr_replicas);

	// add each replicas in the frame
	for(unsigned int cpt = 0; cpt < nbr_replicas; cpt++)
	{
		if((*frame)->getFreeSpace() < packet->getTotalLength())
		{
			complete_dvb_frames.push_back((DvbFrame *)(*frame));
			*frame = new SlottedAlohaFrameData();
			if(!(*frame))
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to create a Slotted Aloha data frame");
				return false;
			}
		}
		packet->setTs(replicas[cpt]);
		if(!(*frame)->addPacket(packet))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Failed to add packet into Slotted Aloha frame\n");
			return false;
		}
	}
	this->packets_wait_ack[qos].push_back(packet);

	return true;
}


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
 * @file SlottedAlohaTal.cpp
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
*/

#include "SlottedAlohaTal.h"
#include "SlottedAlohaBackoffBeb.h"
#include "SlottedAlohaBackoffEied.h"
#include "SlottedAlohaBackoffMimd.h"

#include <opensand_conf/conf.h>


SlottedAlohaTal::SlottedAlohaTal():
	SlottedAloha(),
	tal_id(255),
	packets_wait_ack(),
	nb_success(0),
	nb_max_packets(0),
	nb_max_retransmissions(0),
	base_id(0),
	backoff(NULL),
	category(NULL)
{
}

bool SlottedAlohaTal::init(tal_id_t tal_id,
                           unsigned int frames_per_superframe)
{
	uint16_t max;
	uint16_t multiple;
	uint16_t delay;
	uint16_t t;
	uint16_t d;
	string backoff_name;

	TerminalCategories<TerminalCategorySaloha>::const_iterator cat_iter;
	TerminalMapping<TerminalCategorySaloha>::const_iterator it;

	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Parent 'init()' method must be called first.\n");
		return false;
	}

	this->tal_id = tal_id;
	// set category
	if(this->categories.size() > 1)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Too many terminal categories for Slotted Aloha\n");
		return false;
	}
	this->category = (*(this->categories.begin())).second;

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_NB_MAX_PACKETS, this->nb_max_packets))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_NB_MAX_PACKETS);
		return false;
	}

	/// check nb_max_packets
	if((unsigned)(this->nb_max_packets * this->nb_replicas) >
	   this->category->getSlotsNumber())
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "Maximum packet per Slotted Aloha frames is bigger than "
		    "slots number !\n");
		this->nb_max_packets = this->category->getSlotsNumber() / this->nb_replicas;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_TIMEOUT, this->timeout))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_TIMEOUT);
		return false;
	}
	if(!Conf::getValue(GLOBAL_SECTION, SAT_DELAY, delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, SAT_DELAY);
		return false;
	}

	// TODO RENAME t and d
	// TODO check if we need frame_duration or superframe_duration
	t = this->timeout * frame_duration_ms * this->sf_per_saframe;
	d = 2 * delay + (1 + frames_per_superframe) * frame_duration_ms;
	if (t <= d)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Slotted Aloha timeout too low ! (%u < %u)", t, d);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_NB_MAX_RETRANSMISSIONS,
	                   this->nb_max_retransmissions))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_TIMEOUT);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_BACKOFF_ALGORITHM, backoff_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_TIMEOUT);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_CW_MAX, max))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_TIMEOUT);
		return false;
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_BACKOFF_MULTIPLE, multiple))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_TIMEOUT);
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

	return true;
}

SlottedAlohaTal::~SlottedAlohaTal()
{
	delete this->backoff;
}

// TODO rename into addHeader or encapToSaloha ?!
SlottedAlohaPacketData *SlottedAlohaTal::onRcvEncapPacket(NetPacket *encap_packet,
                                                          uint16_t offset,
                                                          uint16_t burst_size)
{
	SlottedAlohaPacketData *sa_packet;

	sa_packet = new SlottedAlohaPacketData(encap_packet->getData(),
	                                       this->base_id,     // id
	                                       0,                 // ts - set after initialization
	                                       offset,            // seq
	                                       burst_size,        // pdu_nb
	                                       this->timeout,     // timeout
	                                       0,                 // nb_retransmissions
	                                       this->nb_replicas, // nb_replicas
	                                       NULL);             // replicas
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
//		this->debug("< RCVD", ctrl_pkt);
//		TODO useful ?
		this->nb_packets_received_per_frame++;
		this->nb_packets_received_total++;

		switch(ctrl_pkt->getCtrlType())
		{
			case SALOHA_CTRL_ACK:
			{
				uint16_t ids[4];
				saloha_packets_t::iterator packet;
				saloha_id_t id = ctrl_pkt->getId();

				this->convertPacketId(ctrl_pkt->getId(), ids);
				packet = this->packets_wait_ack[ids[SALOHA_ID_QOS]].begin();
				LOG(this->log_saloha, LEVEL_DEBUG,
				    "ACK received for packet with ID %s\n",
				    ctrl_pkt->getId().c_str());
				while(packet != this->packets_wait_ack[ids[SALOHA_ID_QOS]].end())
				{
					SlottedAlohaPacketData *data_pkt;
					saloha_id_t data_id;
					data_pkt = (SlottedAlohaPacketData *)(*packet);
					data_id = this->buildPacketId(data_pkt);
					if(id == data_id)
					{
//						this->debug("remove", data_pkt);
						LOG(this->log_saloha, LEVEL_INFO,
						    "Packet with ID %s found in packet waiting for ack "
						    "and removed\n", data_id.c_str());
						delete data_pkt;
						this->nb_success++;
						this->backoff->setOk();
						// erase goes to next iterator
						this->packets_wait_ack[ids[SALOHA_ID_QOS]].erase(packet);
						continue;
					}
					packet++;
				}
				break;
			}
			//TODO Possibility to add new control signals
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

bool SlottedAlohaTal::schedule(fifos_t &dvb_fifos,
                               list<DvbFrame *> &complete_dvb_frames,
                               uint64_t counter)
{
	uint16_t nb_retransmissions;
	SlottedAlohaPacketData *sa_packet;
	map<qos_t, saloha_packets_t>::iterator wack_it;
	saloha_packets_t::iterator packet;
	SlottedAlohaFrame *frame;
	saloha_ts_list_t ts;
	saloha_ts_list_t::iterator i_ts;
	uint16_t nbr_packets = 0;
	uint16_t nbr_packets_total = 0;

	if(!this->isSuperFrameTick(counter))
	{
		goto skip;
	}
	this->backoff->tick();
	nb_retransmissions = 0;
	// Decrease timeout of waiting packets
	// TODO this has been moved in the other loop, check that this is ok
/*	for(wack_it = this->packets_wait_ack.begin();
	    wack_it != this->packets_wait_ack.end();
	    ++wack_it)
	{
		for(packet = wack_it->second.begin();
		    packet != wack_it->second.end();
		    ++packet)
		{
			sa_packet = (SlottedAlohaPacketData *)(*packet);
			sa_packet->decTimeout();
//			this->debug("decTimeout", sa_packet);
		}
	}*/

	if(!this->backoff->isOk())
	{
		goto skip;
	}
	this->nb_packets_received_per_frame = 0;

	LOG(this->log_saloha, LEVEL_INFO,
	    "Schedule Slotted Aloha packets, %zu complete frames at the moment\n",
	    complete_dvb_frames.size());

	// If waiting packets can be retransmitted, store them in retransmission_packets
	for(wack_it = this->packets_wait_ack.begin();
	    wack_it != this->packets_wait_ack.end();
	    ++wack_it)
	{
		packet = (*wack_it).second.begin();
		while(packet != (*wack_it).second.end())
		{
			sa_packet = (SlottedAlohaPacketData *)(*packet);
			sa_packet->decTimeout();
			if(sa_packet->isTimeout())
			{
				if(sa_packet->canBeRetransmitted(this->nb_max_retransmissions))
				{
//					this->debug("timeout_OK", sa_packet);
					sa_packet->incNbRetransmissions();
					sa_packet->setTimeout(this->timeout);
					this->retransmission_packets.insert(
						this->retransmission_packets.begin() + nb_retransmissions,
						sa_packet);
					nb_retransmissions++;
				}
				else
				{
					LOG(this->log_saloha, LEVEL_DEBUG,
					    "packet lost\n");
					delete sa_packet;
//					this->debug("timeout_NOK", sa_packet);
					this->backoff->setNok();
				}
				// erase goes to next iterator
				wack_it->second.erase(packet);
				continue;
			}
			packet++;
		}
	}

	// TODO MORE DEBUG !!!!
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
	ts = this->getTimeSlots(dvb_fifos); // Get random unique time slots
	i_ts = ts.begin();
	// Send packets which can be retransmitted (high priority)
	packet = this->retransmission_packets.begin();
	while(packet != this->retransmission_packets.end() &&
	      nbr_packets_total < ts.size())
	{
		sa_packet = (SlottedAlohaPacketData *)(*packet);

		if(!this->sendPacketData(complete_dvb_frames,
		                         &frame, sa_packet,
		                         i_ts, sa_packet->getQos()))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "failed to add a Slotted Aloha packet in data frame");
			packet++;
			continue;
		}
//		this->debug("> SEND_bis", sa_packet);
		// erase goes to next iterator
		this->retransmission_packets.erase(packet);
		nbr_packets++;
		nbr_packets_total++;
	}
	if(nbr_packets)
	{
		LOG(this->log_saloha, LEVEL_NOTICE,
		    "%u retransmission packets added to Slotted Aloha frames\n",
		    nbr_packets);
		nbr_packets = 0;
	}

	// Send new packets (low priority)
	for(fifos_t::const_iterator it = dvb_fifos.begin();
	    it != dvb_fifos.end(); ++it)
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

			if(!this->sendPacketData(complete_dvb_frames,
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
//			this->debug("> SEND", sa_packet);
		}
		if(nbr_packets)
		{
			LOG(this->log_saloha, LEVEL_NOTICE,
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
	LOG(this->log_saloha, LEVEL_INFO,
	    "Slotted Aloha scheduled, there is now %zu complete frames to send\n",
	    complete_dvb_frames.size());

//	this->debugFifo("end");
skip:
	return true;
error:
	return false;
}

saloha_ts_list_t SlottedAlohaTal::getTimeSlots(fifos_t &dvb_fifos)
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

	// TODO we could add these fifos as an attribute of this class ?
	nb_packets = this->retransmission_packets.size();
	for(fifos_t::const_iterator it = dvb_fifos.begin();
	    it != dvb_fifos.end(); ++it)
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

/*void SlottedAlohaTal::debugFifo(const char* title)
{
	sa_map_vector_data_t::iterator i1;
	sa_vector_data_t::iterator i2;

	if (!SALOHA_DEBUG)
		return;
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL , ---------------- %s ----------------------", title);
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL | packets_wait_ack.size() = %d",
		(int)this->packets_wait_ack.size());
	for(i1 = this->packets_wait_ack.begin();
		i1 != this->packets_wait_ack.end();
		++i1)
	{
		if (i1->second.size())
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "WKL |     packets_wait_ack[QoS = %d].size() = %d",
				(int)i1->first, (int)i1->second.size());
			for(i2 = i1->second.begin();
				i2 != i1->second.end();
				++i2)
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "WKL |         Pkt[#%d]", (int)*i2);
			}
		}
	}
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL | retransmission_packets.size() = %d",
		(int)this->retransmission_packets.size());
	for(i2 = this->retransmission_packets.begin();
		i2 != this->retransmission_packets.end();
		++i2)
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "WKL |     Pkt[#%d]", (int)*i2);
	}
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL '--------------------------------------------------------");
}*/


// TODO rename
bool SlottedAlohaTal::sendPacketData(list<DvbFrame *> &complete_dvb_frames,
                                     SlottedAlohaFrame **frame,
                                     SlottedAlohaPacketData *packet,
                                     saloha_ts_list_t::iterator &slot,
                                     qos_t qos)
{
	uint16_t nbr_replicas = packet->getNbReplicas();
	uint16_t replicas[nbr_replicas];
	
/*	replicas = (uint16_t *)calloc(nbr_replicas, sizeof(uint16_t));
	if (!replicas)
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "Error when allocating replicas\n");
		return false;
	}*/

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


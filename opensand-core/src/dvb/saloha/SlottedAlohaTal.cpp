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
 * @file SlottedAlohaTal.cpp
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com> / Viveris technologies
*/

#include <opensand_rt/Types.h>

#include "SlottedAlohaTal.h"
#include "SlottedAlohaBackoffBeb.h"
#include "SlottedAlohaBackoffEied.h"
#include "SlottedAlohaBackoffMimd.h"
#include "SlottedAlohaPacketCtrl.h"

#include "OpenSandModelConf.h"
#include "PhysicalLayerPlugin.h"
#include "DvbFifo.h"
#include "FifoElement.h"


SlottedAlohaTal::SlottedAlohaTal():
	SlottedAloha(),
	tal_id(),
	timeout_saf(),
	packets_wait_ack(),
	nb_success(0),
	nb_max_packets(0),
	nb_replicas(0),
	nb_max_retransmissions(0),
	base_id(0),
	backoff(nullptr),
	category(NULL),
	dvb_fifos()
{
}

void SlottedAlohaTal::generateConfiguration()
{
	auto Conf = OpenSandModelConf::Get();

	auto types = Conf->getModelTypesDefinition();
	types->addEnumType("backoff_algo", "Randow Access Back Off Algorithm", {"BEB", "EIED", "MIMD"});

	auto conf = Conf->getOrCreateComponent("access", "Access", "MAC layer configuration");
	auto settings = Conf->getOrCreateComponent("settings", "Settings", conf);
	auto enabled = settings->addParameter("ra_enabled", "Enable CRDSA", types->getType("bool"));

	auto ra = conf->getOrCreateComponent("random_access", "Random Access");
	Conf->setProfileReference(ra, enabled, true);
	ra->getOrCreateParameter("timeout", "Timeout", types->getType("ushort"))->setUnit("slotted aloha frames");
	ra->getOrCreateParameter("replicas", "Replicas", types->getType("ushort"))->setUnit("packets");
	ra->getOrCreateParameter("max_packets", "Max Packets", types->getType("ushort"))->setUnit("packets");
	ra->getOrCreateParameter("max_retry", "Max Retransmissions", types->getType("ushort"))->setUnit("packets");
	ra->getOrCreateParameter("backoff_algo", "Back Off Algorithm", types->getType("backoff_algo"));
	ra->getOrCreateParameter("max_cw", "Max Cw", types->getType("ushort"))->setUnit("slotted aloha frames");
	ra->getOrCreateParameter("backoff_multiple", "Back Off Multiple", types->getType("ushort"));
}

bool SlottedAlohaTal::init(tal_id_t tal_id,
                           std::shared_ptr<TerminalCategorySaloha> category,
                           std::shared_ptr<fifos_t> dvb_fifos,
                           UnitConverter &converter)
{
	time_ms_t sat_delay_ms;
	time_ms_t timeout_ms;
	time_ms_t min_timeout_ms;
	std::string backoff_name;
	
	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Parent 'init()' method must be called first.\n");
		return false;
	}

	this->tal_id = tal_id;
	this->category = category;
	this->category->computeSlotsNumber(converter);

	this->dvb_fifos = dvb_fifos;

	//******************************************************
	// get all value into saloha section and common section
	//******************************************************

	auto Conf = OpenSandModelConf::Get();
	auto saloha_section = Conf->getProfileData()->getComponent("access")->getComponent("random_access");

	if(!OpenSandModelConf::extractParameterData(saloha_section->getParameter("max_packets"), this->nb_max_packets))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'max packets'\n");
		return false;
	}

	if(!OpenSandModelConf::extractParameterData(saloha_section->getParameter("replicas"), this->nb_replicas))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'replicas'\n");
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

	if(!OpenSandModelConf::extractParameterData(saloha_section->getParameter("timeout"), this->timeout_saf))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'timeout'\n");
		return false;
	}

	// Get the max delay
	if(!Conf->getCrdsaMaxSatelliteDelay(sat_delay_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'schedulers': missing parameter 'crdsa max delay'\n");
		return false;
	}

	auto sf_duration = std::chrono::duration_cast<time_ms_t>(this->frame_duration * this->sf_per_saframe);
	timeout_ms = this->timeout_saf * sf_duration;
	min_timeout_ms = 2 * sat_delay_ms + sf_duration;
	if (timeout_ms <= min_timeout_ms)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Slotted Aloha timeout too low ! (%u < %u)",
		    timeout_ms, min_timeout_ms);
		return false;
	}

	if(!OpenSandModelConf::extractParameterData(saloha_section->getParameter("max_retry"), this->nb_max_retransmissions))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'maximum retransmissions'\n");
		return false;
	}

	if(!OpenSandModelConf::extractParameterData(saloha_section->getParameter("backoff_algo"), backoff_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'backoff algorithm'\n");
		return false;
	}

	uint16_t max;
	if(!OpenSandModelConf::extractParameterData(saloha_section->getParameter("max_cw"), max))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'max CW'\n");
		return false;
	}

	uint16_t multiple;
	if(!OpenSandModelConf::extractParameterData(saloha_section->getParameter("backoff_multiple"), multiple))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'random_access': missing parameter 'backoff multiple'\n");
		return false;
	}

	if(backoff_name == "BEB")
	{
		this->backoff = std::make_unique<SlottedAlohaBackoffBeb>(max, multiple);
	}
	else if(backoff_name == "EIED")
	{
		this->backoff = std::make_unique<SlottedAlohaBackoffEied>(max, multiple);
	}
	else if(backoff_name == "MIMD")
	{
		this->backoff = std::make_unique<SlottedAlohaBackoffMimd>(max, multiple);
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize Slotted Aloha '%s' backoff",
		    backoff_name.c_str());
		return false;
	}

	auto output = Output::Get();
	for(auto &&[qos, fifo]: *(this->dvb_fifos))
	{
		if(fifo->getAccessType() != ReturnAccessType::saloha)
		{
			continue;
		}
		std::shared_ptr<Probe<int>> probe_ret;
		std::shared_ptr<Probe<int>> probe_wait;
		std::shared_ptr<Probe<int>> probe_nb_drop;

		probe_ret = output->registerProbe<int>("Aloha.retransmissions." + fifo->getName(), true, SAMPLE_SUM);
		probe_wait = output->registerProbe<int>("Aloha.wait." + fifo->getName(), true, SAMPLE_LAST);
		probe_nb_drop = output->registerProbe<int>("Aloha.drops." + fifo->getName(), true, SAMPLE_SUM);
		this->probe_retransmission.emplace(qos, probe_ret);
		this->probe_wait_ack.emplace(qos, probe_wait);
		this->probe_drop.emplace(qos, probe_nb_drop);
	}
	this->probe_backoff = output->registerProbe<int>("Aloha.backoff", true, SAMPLE_MAX);

	return true;
}


Rt::Ptr<SlottedAlohaPacketData> SlottedAlohaTal::addSalohaHeader(Rt::Ptr<NetPacket> encap_packet,
                                                                 uint16_t offset,
                                                                 uint16_t burst_size)
{
	auto sa_packet = Rt::make_ptr<SlottedAlohaPacketData>(encap_packet->getData(),
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
	if(offset == (burst_size - 1))
	{
		this->base_id++;
	}
	return sa_packet;
}


bool SlottedAlohaTal::onRcvFrame(Rt::Ptr<DvbFrame> dvb_frame)
{
	auto frame = dvb_frame_upcast<SlottedAlohaFrame>(std::move(dvb_frame));
	if(frame->getDataLength() <= 0)
	{
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "skip Slotted Aloha frame with no packet");
		return true;
	}

	LOG(this->log_saloha, LEVEL_INFO,
	    "New Slotted Aloha frame containing %u packets\n",
	    frame->getDataLength());

	std::size_t previous_length = 0;
	for(unsigned int cpt = 0; cpt < frame->getDataLength(); cpt++)
	{
		std::unique_ptr<SlottedAlohaPacketCtrl> ctrl_pkt;
		Rt::Data payload = frame->getPayload(previous_length);
		std::size_t current_length = SlottedAlohaPacketCtrl::getPacketLength(payload);

		try
		{
			ctrl_pkt = std::make_unique<SlottedAlohaPacketCtrl>(payload.c_str(), current_length);
			previous_length += current_length;
		}
		catch (const std::bad_alloc&)
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "cannot create a Slotted Aloha control packet\n");
			continue;
		}
		if(ctrl_pkt->getTerminalId() != this->tal_id)
		{
			// control packet for another terminal
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

				SlottedAlohaPacket::convertPacketId(id, ids);
				packet = this->packets_wait_ack[ids[SALOHA_ID_QOS]].begin();
				LOG(this->log_saloha, LEVEL_DEBUG,
				    "ACK received for packet with ID %s\n",
				    id.c_str());
				while(packet != this->packets_wait_ack[ids[SALOHA_ID_QOS]].end())
				{
					saloha_id_t data_id = (*packet)->getUniqueId();
					if(id == data_id)
					{
						uint16_t cw;
						LOG(this->log_saloha, LEVEL_DEBUG,
						    "Packet with ID %s found in packets waiting for ack "
						    "and removed\n", data_id.c_str());
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

	return true;
}

bool SlottedAlohaTal::schedule(std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                               time_sf_t sf_counter)
{
	uint16_t nb_retransmissions;
	std::map<qos_t, saloha_packets_data_t>::iterator wack_it;
	saloha_packets_data_t::iterator packet;
	Rt::Ptr<SlottedAlohaFrame> frame = Rt::make_ptr<SlottedAlohaFrame>(nullptr);
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
		for(auto&& packet : wack_it->second)
		{
			packet->decTimeout();
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
		packet = wack_it->second.begin();
		while(packet != wack_it->second.end())
		{
			auto& sa_packet = *packet;
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
						std::move(sa_packet));
					nb_retransmissions++;
				}
				else
				{
					uint16_t cw;
					LOG(this->log_saloha, LEVEL_WARNING,
					    "Packet %s lost\n",
					    sa_packet->getUniqueId().c_str());
					this->probe_drop[sa_packet->getQos()]->put(1);
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

	try
	{
		frame = Rt::make_ptr<SlottedAlohaFrameData>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to create a Slotted Aloha data frame");
		return false;
	}
	ts = this->getTimeSlots(); // Get random unique time slots

	i_ts = ts.begin();
	// Send packets which can be retransmitted (high priority)
	packet = this->retransmission_packets.begin();
	while(packet != this->retransmission_packets.end() &&
	      nbr_packets_total + (*packet)->getNbReplicas() <= ts.size())
	{
		auto& sa_packet = *packet;
		qos_t qos = sa_packet->getQos();
		auto replicas = sa_packet->getNbReplicas();

		if(!this->addPacketInFrames(complete_dvb_frames,
		                            frame, std::move(sa_packet),
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
		nbr_packets_total += replicas;
	}

	if(nbr_packets)
	{
		LOG(this->log_saloha, LEVEL_INFO,
		    "%u retransmission packets added to Slotted Aloha frames\n",
		    nbr_packets);
		nbr_packets = 0;
	}

	// Send new packets (low priority)
	for(auto &&[qos, fifo]: *(this->dvb_fifos))
	{
		// the allocated slot limits the capacity
		if(nbr_packets_total >= ts.size())
		{
			break;
		}

		if(fifo->getAccessType() != ReturnAccessType::saloha)
		{
			continue;
		}
		for (auto &&elem: *fifo)
		{
			Rt::Ptr<SlottedAlohaPacketData> sa_packet = elem->releaseElem<SlottedAlohaPacketData>();
			auto replicas = sa_packet->getNbReplicas();

			if(!this->addPacketInFrames(complete_dvb_frames,
			                            frame, std::move(sa_packet),
			                            i_ts, qos))
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to add a Slotted Aloha packet in data frame");
				continue;
			}
			nbr_packets++;
			nbr_packets_total += replicas;

			if (nbr_packets_total + this->nb_replicas > ts.size())
			{
				break;
			}
		}
		if(nbr_packets)
		{
			LOG(this->log_saloha, LEVEL_INFO,
			    "%u packets added to Slotted Aloha frames from %s fifo\n",
			    nbr_packets, fifo->getName());
			nbr_packets = 0;
		}
	}
	// add last frame in complete_dvb_frames
	if(frame->getDataLength())
	{
		complete_dvb_frames.push_back(dvb_frame_downcast(std::move(frame)));
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
	for(auto &&[qos, fifo]: *(this->dvb_fifos))
	{
		if(fifo->getAccessType() != ReturnAccessType::saloha)
		{
			continue;
		}
		this->probe_retransmission[qos]->put(0);
		this->probe_drop[qos]->put(0);
	}
	return true;
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
	for(auto &&[qos, fifo]: *(this->dvb_fifos))
	{
		if(fifo->getAccessType() == ReturnAccessType::saloha)
		{
			nb_packets += fifo->getCurrentSize();
		}
	}
	max = std::min(nb_packets, this->nb_max_packets) * this->nb_replicas;

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

bool SlottedAlohaTal::addPacketInFrames(std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                        Rt::Ptr<SlottedAlohaFrame>& frame,
                                        Rt::Ptr<SlottedAlohaPacketData> packet,
                                        saloha_ts_list_t::iterator &slot,
                                        qos_t qos)
{
	uint16_t nbr_replicas = packet->getNbReplicas();
	uint16_t replicas[nbr_replicas];
	
	// in this function, the iterator on slots can be increased by nbr_replicas
	// because slots has been computed accordingly
	for(uint16_t cpt = 0; cpt < nbr_replicas; cpt++)
	{
		replicas[cpt] = *slot;
		slot++;
	}
	packet->setReplicas(replicas, nbr_replicas);

	// add each replicas in the frame
	for(uint16_t cpt = 0; cpt < nbr_replicas; cpt++)
	{
		if(frame->getFreeSpace() < packet->getTotalLength())
		{
			complete_dvb_frames.push_back(dvb_frame_downcast(std::move(frame)));
			try
			{
				frame = Rt::make_ptr<SlottedAlohaFrameData>();
			}
			catch (const std::bad_alloc&)
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to create a Slotted Aloha data frame");
				return false;
			}
		}
		packet->setTs(replicas[cpt]);
		// This copies packet->data() so we can do with bare pointers for now
		if(!frame->addPacket(*packet))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "Failed to add packet into Slotted Aloha frame\n");
			return false;
		}
	}

	this->packets_wait_ack[qos].push_back(std::move(packet));

	return true;
}


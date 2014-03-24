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
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */


/**
 * @file     ForwardScheduling.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-S2 forward or downlink
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 */



#include "ForwardSchedulingS2.h"
#include "MacFifoElement.h"

#include <opensand_output/Output.h>

#include <cassert>


/**
 * @brief Get the payload size in Bytes according to coding rate
 *
 * @param coding_rate  The coding rate
 * @return the payload size in Bytes
 */
static size_t getPayloadSize(string coding_rate)
{
	size_t payload;

	// see ESTI EN 302 307 v1.2.1 Table 5a
	if(!coding_rate.compare("1/4"))
		payload = 2001;
	else if(!coding_rate.compare("1/3"))
		payload = 2676;
	else if(!coding_rate.compare("2/5"))
		payload = 3216;
	else if(!coding_rate.compare("1/2"))
		payload = 4026;
	else if(!coding_rate.compare("3/5"))
		payload = 4836;
	else if(!coding_rate.compare("2/3"))
		payload = 5380;
	else if(!coding_rate.compare("3/4"))
		payload = 6051;
	else if(!coding_rate.compare("4/5"))
		payload = 6456;
	else if(!coding_rate.compare("5/6"))
		payload = 6730;
	else if(!coding_rate.compare("8/9"))
		payload = 7184;
	else if(!coding_rate.compare("9/10"))
		payload = 7274;
	else
		payload = 8100; //size of a normal FECFRAME

	return payload;
}


ForwardSchedulingS2::ForwardSchedulingS2(const EncapPlugin::EncapPacketHandler *packet_handler,
                                         const fifos_t &fifos,
                                         FmtSimulation *const fwd_fmt_simu,
                                         const TerminalCategory *const category):
	Scheduling(packet_handler, fifos),
	incomplete_bb_frames(),
	incomplete_bb_frames_ordered(),
	pending_bbframes(),
	fwd_fmt_simu(fwd_fmt_simu),
	category(category)
{
}

ForwardSchedulingS2::~ForwardSchedulingS2()
{
	list<BBFrame *>::iterator it;
	for(it = this->incomplete_bb_frames_ordered.begin();
	    it != this->incomplete_bb_frames_ordered.end(); ++it)
	{
		delete *it;
	}
	for(it = this->pending_bbframes.begin();
	    it != this->pending_bbframes.end(); ++it)
	{
		delete *it;
	}
}


bool ForwardSchedulingS2::schedule(const time_sf_t current_superframe_sf,
                                   const time_frame_t UNUSED(current_frame),
                                   clock_t current_time,
                                   list<DvbFrame *> *complete_dvb_frames,
                                   uint32_t &remaining_allocation)
{
	fifos_t::const_iterator fifo_it;
	vector<CarriersGroup *> carriers;
	vector<CarriersGroup *>::iterator carrier_it;
	carriers = this->category->getCarriersGroups();

	// initialize carriers capacity
	for(carrier_it = carriers.begin();
	    carrier_it != carriers.end();
	    ++carrier_it)
	{
		vol_sym_t capacity_sym  = (*carrier_it)->getTotalCapacity() +
		                          (*carrier_it)->getRemainingCapacity();
		(*carrier_it)->setRemainingCapacity(capacity_sym);
	}

	for(fifo_it = this->dvb_fifos.begin();
	    fifo_it != this->dvb_fifos.end(); ++fifo_it)
	{
		for(carrier_it = carriers.begin();
		    carrier_it != carriers.end();
		    ++carrier_it)
		{
			if(!this->scheduleEncapPackets((*fifo_it).second,
			                               current_superframe_sf,
			                               current_time,
			                               complete_dvb_frames,
			                               *carrier_it))
			{
				return false;
			}
		}
	}

	for(carrier_it = carriers.begin();
	    carrier_it != carriers.end();
	    ++carrier_it)
	{
		// keep total remaining capacity (for stats)
		remaining_allocation += (*carrier_it)->getRemainingCapacity();
		// reset remaining capacity
		(*carrier_it)->setRemainingCapacity(0);
	}

	return true;
}

bool ForwardSchedulingS2::scheduleEncapPackets(DvbFifo *fifo,
                                               const time_sf_t current_superframe_sf,
                                               clock_t current_time,
                                               list<DvbFrame *> *complete_dvb_frames,
                                               CarriersGroup *carriers)
{
	int ret;
	unsigned int sent_packets = 0;
	MacFifoElement *elem;
	long max_to_send;
	BBFrame *current_bbframe;
	list<unsigned int> supported_modcods = carriers->getFmtIds();
	vol_sym_t capacity_sym = carriers->getRemainingCapacity();
	vol_sym_t previous_sym = carriers->getPreviousCapacity(current_superframe_sf);
	vol_sym_t init_capa = capacity_sym;
	capacity_sym += previous_sym;

	Output::sendLog(this->log_scheduling, LEVEL_INFO,
	                "SF#%u: capacity is %u symbols (+ %u previous)\n",
	                current_superframe_sf, capacity_sym, previous_sym);

	// first add the pending complete BBFrame in the complete BBframes list
	// we add previous remaining capacity here because if a BBFrame was
	// not send before, previous_capacity contains the remaining capacity at the
	// end of the previous frame
	this->schedulePending(supported_modcods, complete_dvb_frames,
	                      capacity_sym);
	// reset previous capacity
	carriers->setPreviousCapacity(0, 0);
	// all the previous capacity was not consumed, remove it as we are not on
	// pending frames anymore
	capacity_sym = std::min(init_capa, capacity_sym);

	// retrieve the number of packets waiting for retransmission
	max_to_send = fifo->getCurrentSize();
	if(max_to_send <= 0)
	{
		goto skip;
	}

	// there are really packets to send
	Output::sendLog(this->log_scheduling, LEVEL_INFO,
	                "SF#%u: send at most %ld encapsulation packets "
	                "for %s fifo\n", current_superframe_sf,
	                max_to_send, fifo->getName().c_str());

	// now build BB frames with packets extracted from the MAC FIFO
	while(fifo->getCurrentSize() > 0)
	{
		NetPacket *encap_packet;
		tal_id_t tal_id;
		NetPacket *data;
		NetPacket *remaining_data;

		// simulate the satellite delay
		if(fifo->getTickOut() > current_time)
		{
			Output::sendLog(this->log_scheduling, LEVEL_INFO,
			                "SF#%u: packet is not scheduled for the "
			                "moment, break\n", current_superframe_sf); 
			// this is the first MAC FIFO element that is not ready yet,
			// there is no more work to do, break now
			break;
		}

		elem = fifo->pop();
		// examine the packet to be sent
		if(elem->getType() != 1)
		{
			Output::sendLog(this->log_scheduling, LEVEL_ERROR,
			                "SF#%u: MAC FIFO element does not "
			                "contain NetPacket\n", current_superframe_sf);
			goto error_fifo_elem;
		}

		encap_packet = elem->getPacket();
		// retrieve the encapsulation packet
		if(encap_packet == NULL)
		{
			Output::sendLog(this->log_scheduling, LEVEL_ERROR,
			                "SF#%u: invalid packet #%u in MAC FIFO "
			                "element\n", current_superframe_sf,
			                sent_packets + 1);
			goto error_fifo_elem;
		}

		// retrieve the ST ID associated to the packet
		tal_id = encap_packet->getDstTalId();
		// This is a broadcast/multicast destination
		if(tal_id == BROADCAST_TAL_ID)
		{
			// Select the tal_id corresponding to the lower modcod in order to
			// make all terminal able to read the message
			tal_id =
				this->fwd_fmt_simu->getTalIdWithLowerModcod();
			if(tal_id == 255)
			{
				Output::sendLog(this->log_scheduling, LEVEL_ERROR,
				                "SF#%u: The scheduling of a "
				                "multicast frame failed\n",
				                current_superframe_sf);
				Output::sendLog(this->log_scheduling, LEVEL_ERROR,
				                "SF#%u: The Tal_Id corresponding to "
				                "the terminal using the lower modcod can not "
				                "be retrieved\n", current_superframe_sf);
				goto error;
			}
			Output::sendLog(this->log_scheduling, LEVEL_INFO,
			                "SF#%u: TAL_ID corresponding to lower "
			                "MODCOD = %u\n", current_superframe_sf,
			                tal_id);
		}

		if(!this->getIncompleteBBFrame(tal_id, carriers, &current_bbframe))
		{
			// cannot initialize incomplete BB Frame
			delete encap_packet;
			goto error_fifo_elem;
		}
		else if(!current_bbframe)
		{
			// cannot get modcod for the ST delete the element
			delete encap_packet;
			delete elem;
			continue;
		}

		Output::sendLog(this->log_scheduling, LEVEL_DEBUG,
		                "SF#%u: Got the BBFrame for packet #%u, "
		                "there is now %zu complete BBFrames and %zu "
		                "incomplete\n", current_superframe_sf,
		                sent_packets + 1, complete_dvb_frames->size(),
		                this->incomplete_bb_frames.size());

		// get the part of the packet to store in the BBFrame
		ret = this->packet_handler->getChunk(encap_packet,
		                                     current_bbframe->getFreeSpace(),
		                                     &data, &remaining_data);
		// use case 4 (see @ref getChunk)
		if(!ret)
		{
			Output::sendLog(this->log_scheduling, LEVEL_ERROR,
			                "SF#%u: error while processing packet "
			                "#%u\n", current_superframe_sf,
			                sent_packets + 1);
			delete elem;
		}
		// use cases 1 (see @ref getChunk)
		else if(data && !remaining_data)
		{
			if(!current_bbframe->addPacket(data))
			{
				Output::sendLog(this->log_scheduling, LEVEL_ERROR,
				                "SF#%u: failed to add encapsulation "
				                "packet #%u in BB frame with MODCOD ID %u "
				                "(packet length %zu, free space %zu",
				                current_superframe_sf,
				                sent_packets + 1,
				                current_bbframe->getModcodId(),
				                data->getTotalLength(),
				                current_bbframe->getFreeSpace());
				goto error_fifo_elem;
			}
			// delete the NetPacket once it has been copied in the BBFrame
			delete data;
			sent_packets++;
			// destroy the element
			delete elem;
		}
		// use case 2 (see @ref getChunk)
		else if(data && remaining_data)
		{
			if(!current_bbframe->addPacket(data))
			{
				Output::sendLog(this->log_scheduling, LEVEL_ERROR,
				                "SF#%u: failed to add encapsulation "
				                "packet #%u in BB frame with MODCOD ID %u "
				                "(packet length %zu, free space %zu",
				                current_superframe_sf,
				                sent_packets + 1,
				                current_bbframe->getModcodId(),
				                data->getTotalLength(),
				                current_bbframe->getFreeSpace());
				goto error_fifo_elem;
			}
			// delete the NetPacket once it has been copied in the BBFrame
			delete data;

			// replace the fifo first element with the remaining data
			elem->setPacket(remaining_data);
			fifo->pushFront(elem);

			Output::sendLog(this->log_scheduling, LEVEL_INFO,
			                "SF#%u: packet fragmented, there is "
			                "still %zu bytes of data\n",
			                current_superframe_sf,
			                remaining_data->getTotalLength());
		}
		// use case 3 (see @ref getChunk)
		else if(!data && remaining_data)
		{
			// replace the fifo first element with the remaining data
			elem->setPacket(remaining_data);
			fifo->pushFront(elem);

			// keep the NetPacket in the fifo
			Output::sendLog(this->log_scheduling, LEVEL_INFO,
			                "SF#%u: not enough free space in BBFrame "
			                "(%zu bytes) for %s packet (%zu bytes)\n",
			                current_superframe_sf,
			                current_bbframe->getFreeSpace(),
			                this->packet_handler->getName().c_str(),
			                encap_packet->getTotalLength());
		}
		else
		{
			Output::sendLog(this->log_scheduling, LEVEL_ERROR,
			                "SF#%u: bad getChunk function "
			                "implementation, assert or skip packet #%u\n",
			                current_superframe_sf,
			                sent_packets + 1);
			assert(0);
			delete elem;
		}

		// the BBFrame has been completed or the next packet is too long
		// add the BBFrame in the list of complete BBFrames and decrease
		// duration credit
		if(current_bbframe->getFreeSpace() <= 0 ||
		   remaining_data != NULL)
		{
			ret = this->addCompleteBBFrame(complete_dvb_frames,
			                               current_bbframe,
			                               capacity_sym);
			if(ret == status_error)
			{
				goto error;
			}
			else
			{
				unsigned int modcod = current_bbframe->getModcodId();

				this->incomplete_bb_frames_ordered.remove(current_bbframe);
				this->incomplete_bb_frames.erase(modcod);
				if(ret == status_full)
				{
					time_sf_t next_sf = current_superframe_sf + 1;
					// we keep the remaining capacity that won't be used for
					// next frame
					carriers->setPreviousCapacity(capacity_sym,
					                              next_sf);
					capacity_sym = 0;
					this->pending_bbframes.push_back(current_bbframe);
					break;
				}
			}
		}
	}

	// try to fill the BBFrames list with the remaining incomplete BBFrames
	for(list<BBFrame *>::iterator it = this->incomplete_bb_frames_ordered.begin();
	    it != this->incomplete_bb_frames_ordered.end();
	    it = this->incomplete_bb_frames_ordered.erase(it))
	{
		if(capacity_sym <= 0)
		{
			break;
		}

		ret = this->addCompleteBBFrame(complete_dvb_frames, *it,
		                               capacity_sym);
		if(ret == status_error)
		{
			goto error;
		}
		else if(ret == status_ok)
		{
			unsigned int modcod = (*it)->getModcodId();

			this->incomplete_bb_frames.erase(modcod);
			// incomplete ordered erased in loop
		}
	}

	if(sent_packets != 0)
	{
		unsigned int cpt_frame = complete_dvb_frames->size();

		Output::sendLog(this->log_scheduling, LEVEL_INFO,
		                "SF#%u: %u %s been scheduled and %u BB %s "
		                "completed\n", current_superframe_sf,
		                sent_packets,
		                (sent_packets > 1) ? "packets have" : "packet has",
		                cpt_frame,
		                (cpt_frame > 1) ? "frames were" : "frame was");
	}

	// keep the remaining capacity for next computation
	// it can be used for next timeslot
	carriers->setRemainingCapacity(capacity_sym);

skip:
	return true;
error_fifo_elem:
	delete elem;
error:
	return false;
}


bool ForwardSchedulingS2::createIncompleteBBFrame(BBFrame **bbframe,
                                                  unsigned int modcod_id)
{
	// if there is no incomplete BB frame create a new one
	size_t bbframe_size_bytes;
	string coding_rate;
	const FmtDefinitionTable *modcod_definitions;
	modcod_definitions = this->fwd_fmt_simu->getModcodDefinitions();

	*bbframe = new BBFrame();
	if(bbframe == NULL)
	{
		Output::sendLog(this->log_scheduling, LEVEL_ERROR,
		                "failed to create an incomplete BB frame\n");
		goto error;
	}

	if(!this->packet_handler)
	{
		Output::sendLog(this->log_scheduling, LEVEL_ERROR,
		                "packet handler is NULL\n");
		goto error;
	}

	// set the MODCOD ID of the BB frame
	(*bbframe)->setModcodId(modcod_id);

	// get the payload size
	// to simulate the modcod applied to transmitted data, we limit the
	// size of the BBframe to be the payload size
	coding_rate = modcod_definitions->getCodingRate(modcod_id);
	bbframe_size_bytes = getPayloadSize(coding_rate);
	Output::sendLog(this->log_scheduling, LEVEL_DEBUG,
	                "size of the BBFRAME for MODCOD %u = %zu\n",
	                modcod_id, bbframe_size_bytes);

	// set the size of the BB frame
	(*bbframe)->setMaxSize(bbframe_size_bytes);

	return true;

error:
	return false;
}


bool ForwardSchedulingS2::retrieveCurrentModcod(tal_id_t tal_id,
                                                unsigned int &modcod_id)
{
	bool is_advertised;

	// retrieve the current MODCOD for the ST and whether
	// it changed or not
	if(!this->fwd_fmt_simu->doTerminalExist(tal_id))
	{
		Output::sendLog(this->log_scheduling, LEVEL_ERROR,
		                "encapsulation packet is for ST with ID %u "
		                "that is not registered\n", tal_id);
		goto error;
	}
	modcod_id = this->fwd_fmt_simu->getCurrentModcodId(tal_id);
	is_advertised = this->fwd_fmt_simu->isCurrentModcodAdvertised(tal_id);
	if(!is_advertised)
	{
		// send the most robust MODCOD if not advertised
		modcod_id = std::min(this->fwd_fmt_simu->getCurrentModcodId(tal_id),
		                     this->fwd_fmt_simu->getPreviousModcodId(tal_id));
	}

	Output::sendLog(this->log_scheduling, LEVEL_DEBUG,
	                "MODCOD for ST ID %u = %u (changed = %s)\n",
	                tal_id, modcod_id, is_advertised ? "no" : "yes");

	return true;

error:
	return false;
}

bool ForwardSchedulingS2::getBBFrameSize(size_t bbframe_size_bytes,
                                         unsigned int modcod_id,
                                         vol_sym_t &bbframe_size_sym)
{
	const FmtDefinitionTable *modcod_definitions;
	float spectral_efficiency;

	modcod_definitions = this->fwd_fmt_simu->getModcodDefinitions();

	if(!modcod_definitions->doFmtIdExist(modcod_id))
	{
		Output::sendLog(this->log_scheduling, LEVEL_ERROR,
		                "failed to found the definition of MODCOD ID %u\n",
		                modcod_id);
		goto error;
	}
	spectral_efficiency = modcod_definitions->getSpectralEfficiency(modcod_id);

	// duration is calculated over the complete BBFrame size, the BBFrame data
	// size represents the payload without coding
	bbframe_size_sym = (bbframe_size_bytes * 8) / spectral_efficiency;

	Output::sendLog(this->log_scheduling, LEVEL_INFO,
	                "size of the BBFRAME = %u symbols\n", bbframe_size_sym);

	return true;

error:
	return false;
}


bool ForwardSchedulingS2::getIncompleteBBFrame(tal_id_t tal_id,
                                               CarriersGroup *carriers,
                                               BBFrame **bbframe)
{
	map<unsigned int, BBFrame *>::iterator iter;
	unsigned int desired_modcod;
	unsigned int modcod_id;

	*bbframe = NULL;

	// retrieve the current MODCOD for the ST
	if(!this->retrieveCurrentModcod(tal_id, desired_modcod))
	{
		// cannot get modcod for the ST skip this element
		goto skip;
	}

	// get best modcod ID according to carrier
	modcod_id = carriers->getNearestFmtId(desired_modcod);
	if(modcod_id == 0)
	{
		Output::sendLog(this->log_scheduling, LEVEL_NOTICE,
		                "cannot serve terminal %u with any modcod (desired %u) "
		                "on carrier %u\n", tal_id, desired_modcod,
		                carriers->getCarriersId());

		goto skip;
	}
	Output::sendLog(this->log_scheduling, LEVEL_DEBUG,
	                "Available MODCOD for ST id %u = %u\n", tal_id, modcod_id);

	// find if the BBFrame exists
	iter = this->incomplete_bb_frames.find(modcod_id);
	if(iter != this->incomplete_bb_frames.end() && (*iter).second != NULL)
	{
		Output::sendLog(this->log_scheduling, LEVEL_INFO,
		                "Found a BBFrame for MODCOD %u\n", modcod_id);
		*bbframe = (*iter).second;
	}
	// no BBFrame for this MOCDCOD create a new one
	else
	{
		Output::sendLog(this->log_scheduling, LEVEL_INFO,
		                "Create a new BBFrame for MODCOD %u\n", modcod_id);
		// if there is no incomplete BB frame create a new one
		if(!this->createIncompleteBBFrame(bbframe, modcod_id))
		{
			goto error;
		}
		// add the BBFrame in the map and list
		this->incomplete_bb_frames[modcod_id] = *bbframe;
		this->incomplete_bb_frames_ordered.push_back(*bbframe);
	}

skip:
	return true;
error:
	return false;
}


sched_status_t ForwardSchedulingS2::addCompleteBBFrame(list<DvbFrame *> *complete_bb_frames,
                                                       BBFrame *bbframe,
                                                       vol_sym_t &remaining_capacity_sym)
{
	unsigned int modcod_id = bbframe->getModcodId();
	size_t bbframe_size_bytes = bbframe->getMaxSize();
	vol_sym_t bbframe_size_sym;

	// how much time do we need to send the BB frame ?
	if(!this->getBBFrameSize(bbframe_size_bytes, modcod_id, bbframe_size_sym))
	{
		Output::sendLog(this->log_scheduling, LEVEL_ERROR,
		                "failed to get BB frame size (MODCOD ID = %u)\n",
		                modcod_id);
		return status_error;
	}

	// not enough space for this BBFrame
	if(remaining_capacity_sym < bbframe_size_sym)
	{
		Output::sendLog(this->log_scheduling, LEVEL_INFO,
		                "not enough capacity (%u symbols) for the BBFrame of "
		                "size %u symbols\n", remaining_capacity_sym,
		                bbframe_size_sym);
		return status_full;
	}

	// check if some terminals need to be advertised
	if(!this->fwd_fmt_simu->areCurrentModcodsAdvertised())
	{
		// we can create up to MAX_MODCOD_OPTIONS, if we need more, they
		// will be advertised in next BBFrame
		for(unsigned int i = 0; i < MAX_MODCOD_OPTIONS; i++)
		{
			tal_id_t tal_id;
			uint8_t modcod;
			if(!this->fwd_fmt_simu->getNextModcodToAdvertise(tal_id, modcod))
			{
				Output::sendLog(this->log_scheduling, LEVEL_INFO,
				                "%u MODCOD advertised\n", i);
				break;
			}
			bbframe->addModcodOption(tal_id, modcod);
			Output::sendLog(this->log_scheduling, LEVEL_INFO,
			                "Advertise MODCOD for terminal %u\n", tal_id);
		}
	}

	// we can send the BBFrame
	complete_bb_frames->push_back((DvbFrame *)bbframe);

	// reduce the time carrier capacity by the BBFrame size
	remaining_capacity_sym -= bbframe_size_sym;

	return status_ok;
}


void ForwardSchedulingS2::schedulePending(const list<unsigned int> supported_modcods,
                                          list<DvbFrame *> *complete_dvb_frames,
                                          vol_sym_t &remaining_capacity_sym)
{
	if(this->pending_bbframes.size() == 0)
	{
		return;
	}

	list<BBFrame *>::iterator it;
	list<BBFrame *> new_pending;

	for(it = this->pending_bbframes.begin();
		it != this->pending_bbframes.end();
		++it)
	{
		unsigned int modcod = (*it)->getModcodId();

		if(std::find(supported_modcods.begin(), supported_modcods.end(), modcod) !=
		   supported_modcods.end())
		{
			if(this->addCompleteBBFrame(complete_dvb_frames,
			                            (*it),
			                            remaining_capacity_sym) != status_ok)
			{
				Output::sendLog(this->log_scheduling, LEVEL_ERROR,
				                "cannot add pending BBFrame in the list "
				                "of complete BBFrames\n");
			}
		}
		else
		{
			// keep the BBFrame in pending list
			new_pending.push_back(*it);
		}
	}
	this->pending_bbframes.clear();
	this->pending_bbframes.insert(this->pending_bbframes.end(),
	                              new_pending.begin(), new_pending.end());

}

// TODO scheduling improvement
// At the moment, incomplete BBFrames that can not be sent are kept:
//  1 - until they are completed
//  2 - until there is space to send them
//  In first case, we have a problem if no terminal required the same
//  modcod, the BBFrame will wait forever to be completed and we will
//  have to wait case 2 for the BBFrame to be sent
//  One way to improve this algo, use to counter :
//   - first: if the counter is reached, try to complete the frame
//            with packet requiring higher MODCODs
//   - second: (the frame is still not completed) force sending the incomplete
//             frame

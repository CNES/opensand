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
 * @brief    The scheduling functions for MAC FIFOs with DVB-S2 forward
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 */



#define DBG_PACKAGE PKG_DAMA_DC
#include <opensand_conf/uti_debug.h>

#include "ForwardSchedulingS2.h"
#include "MacFifoElement.h"

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
		payload = 7274;
	else if(!coding_rate.compare("9/10"))
		payload = 7184;
	else
		payload = 8100; //size of a normal FECFRAME

	return payload;
}


ForwardSchedulingS2::ForwardSchedulingS2(const EncapPlugin::EncapPacketHandler *packet_handler,
                                         const fifos_t &fifos,
                                         const unsigned int frames_per_superframe,
                                         const FmtSimulation *const fmt_simu,
                                         const TerminalCategory *const category):
	Scheduling(packet_handler, fifos),
	frames_per_superframe(frames_per_superframe),
	incomplete_bb_frames(),
	incomplete_bb_frames_ordered(),
	pending_bbframes(),
	fmt_simu(fmt_simu),
	category(category)
{
}

ForwardSchedulingS2::~ForwardSchedulingS2()
{
	for(list<BBFrame *>::iterator it = incomplete_bb_frames_ordered.begin();
	    it != incomplete_bb_frames_ordered.end(); ++it)
	{
		delete *it;
	}
}


bool ForwardSchedulingS2::schedule(const time_sf_t current_superframe_sf,
                                   const time_frame_t current_frame,
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
		// this function is called each frame, the total capacity is set for
		// a superframe, divide the capacity by the number of frames per superframe
		capacity_sym /= this->frames_per_superframe;
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
			                               current_frame,
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
		// TODO
		// we could keep some remaining capacity for next frame but it should not be
		// kept among frames so we should add a variable to now if
		// something has been allocated during fifo iterations and reset if
		// nothing has been done
		// we should also take care of the amount of remaining capa that
		// could be greater than timeslot if we keep it twice
		(*carrier_it)->setRemainingCapacity(0);
	}

	return true;
}

bool ForwardSchedulingS2::scheduleEncapPackets(DvbFifo *fifo,
                                               const time_sf_t current_superframe_sf,
                                               const time_frame_t current_frame,
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

	UTI_DEBUG("SF#%u: frame %u: capacity is %u symbols\n",
	          current_superframe_sf, current_frame, capacity_sym);

	// first add the pending complete BBFrame in the complete BBframes list
	this->schedulePending(supported_modcods, complete_dvb_frames, capacity_sym);

	// retrieve the number of packets waiting for retransmission
	max_to_send = fifo->getCurrentSize();
	if(max_to_send <= 0)
	{
		goto skip;
	}

	// there are really packets to send
	UTI_DEBUG("SF#%u: frame %u: send at most %ld encapsulation packets "
	          "for %s fifo\n", current_superframe_sf, current_frame,
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
			UTI_DEBUG("SF#%u: frame %u: packet is not scheduled for the moment, "
			          "break\n", current_superframe_sf, current_frame);
			// this is the first MAC FIFO element that is not ready yet,
			// there is no more work to do, break now
			break;
		}

		elem = fifo->pop();
		// examine the packet to be sent
		if(elem->getType() != 1)
		{
			UTI_ERROR("SF#%u: frame %u: MAC FIFO element does not contain NetPacket\n",
			          current_superframe_sf, current_frame);
			goto error_fifo_elem;
		}

		encap_packet = elem->getPacket();
		// retrieve the encapsulation packet
		if(encap_packet == NULL)
		{
			UTI_ERROR("SF#%u: frame %u: invalid packet #%u in MAC FIFO element\n",
			          current_superframe_sf, current_frame, sent_packets + 1);
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
				this->fmt_simu->getTalIdWithLowerFwdModcod();
			if(tal_id < 0)
			{
				UTI_ERROR("SF#%u: frame %u: The shceduling of a multicast "
				          "frame failed\n", current_superframe_sf, current_frame);
				UTI_ERROR("SF#%u: frame %u: The Tal_Id corresponding to the "
				          "terminal using the lower modcod can not be retrieved\n",
				          current_superframe_sf, current_frame);
				goto error;
			}
			UTI_DEBUG("SF#%u: frame %u: TAL_ID corresponding to lower MODCOD = %u\n",
			          current_superframe_sf, current_frame, tal_id);
		}

		if(!this->getIncompleteBBFrame(tal_id, &current_bbframe))
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

		UTI_DEBUG_L3("SF#%u: frame %u: Got the BBFrame for packet #%u, "
		             "there is now %zu complete BBFrames and %zu incomplete\n",
		             current_superframe_sf, current_frame,
		             sent_packets + 1, complete_dvb_frames->size(),
		             this->incomplete_bb_frames.size());

		// get the part of the packet to store in the BBFrame
		ret = this->packet_handler->getChunk(encap_packet,
		                                     current_bbframe->getFreeSpace(),
		                                     &data, &remaining_data);
		// use case 4 (see @ref getChunk)
		if(!ret)
		{
			UTI_ERROR("SF#%u: frame %u: error while processing packet #%u\n",
			          current_superframe_sf, current_frame, sent_packets + 1);
			delete elem;
		}
		// use cases 1 (see @ref getChunk)
		else if(data && !remaining_data)
		{
			if(!current_bbframe->addPacket(data))
			{
				UTI_ERROR("SF#%u: frame %u: failed to add encapsulation packet "
				          "#%u in BB frame with MODCOD ID %u (packet length %i,"
				          "  free space %u",
				          current_superframe_sf, current_frame,
				          sent_packets + 1, current_bbframe->getModcodId(),
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
				UTI_ERROR("SF#%u: frame %u: failed to add encapsulation packet "
				          "#%u in BB frame with MODCOD ID %u (packet "
				          "length %u, free space %u",
				          current_superframe_sf, current_frame,
				          sent_packets + 1, current_bbframe->getModcodId(),
				          data->getTotalLength(),
				          current_bbframe->getFreeSpace());
				goto error_fifo_elem;
			}
			// delete the NetPacket once it has been copied in the BBFrame
			delete data;

			// replace the fifo first element with the remaining data
			elem->setPacket(remaining_data);
			fifo->pushFront(elem);

			UTI_DEBUG("SF#%u: frame %u: packet fragmented, there is still "
			          "%u bytes of data\n",
			          current_superframe_sf, current_frame,
			          remaining_data->getTotalLength());
		}
		// use case 3 (see @ref getChunk)
		else if(!data && remaining_data)
		{
			// replace the fifo first element with the remaining data
			elem->setPacket(remaining_data);
			fifo->pushFront(elem);

			// keep the NetPacket in the fifo
			UTI_DEBUG("SF#%u: frame %u: not enough free space in BBFrame "
			          "(%u bytes) for %s packet (%u bytes)\n",
			          current_superframe_sf, current_frame,
			          current_bbframe->getFreeSpace(),
			          this->packet_handler->getName().c_str(),
			          encap_packet->getTotalLength());
		}
		else
		{
			UTI_ERROR("SF#%u: frame %u: bad getChunk function implementation, "
			          "assert or skip packet #%u\n",
			          current_superframe_sf, current_frame, sent_packets + 1);
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
		}
	}

	if(sent_packets != 0)
	{
		unsigned int cpt_frame = complete_dvb_frames->size();

		UTI_DEBUG("SF#%u: frame %u: %u %s been scheduled and %u BB %s completed\n",
		          current_superframe_sf, current_frame,
		          sent_packets, (sent_packets > 1) ? "packets have" : "packet has",
		          cpt_frame, (cpt_frame > 1) ? "frames were" : "frame was");
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
	modcod_definitions = this->fmt_simu->getFwdModcodDefinitions();

	*bbframe = new BBFrame();
	if(bbframe == NULL)
	{
		UTI_ERROR("failed to create an incomplete BB frame\n");
		goto error;
	}

	if(!this->packet_handler)
	{
		UTI_ERROR("packet handler is NULL\n");
		goto error;
	}

	// set the MODCOD ID of the BB frame
	(*bbframe)->setModcodId(modcod_id);

	// set the type of encapsulation packets the BB frame will contain
	(*bbframe)->setEncapPacketEtherType(this->packet_handler->getEtherType());

	// get the payload size
	// to simulate the modcod applied to transmitted data, we limit the
	// size of the BBframe to be the payload size
	coding_rate = modcod_definitions->getCodingRate(modcod_id);
	bbframe_size_bytes = getPayloadSize(coding_rate);
	UTI_DEBUG_L3("size of the BBFRAME for MODCOD %u = %zu\n",
	             modcod_id, bbframe_size_bytes);

	// set the size of the BB frame
	(*bbframe)->setMaxSize(bbframe_size_bytes);

	return true;

error:
	return false;
}


bool ForwardSchedulingS2::retrieveCurrentModcod(long tal_id,
                                                unsigned int &modcod_id)
{
	bool do_advertise_modcod;

	// retrieve the current MODCOD for the ST and whether
	// it changed or not
	if(!this->fmt_simu->doTerminalExist(tal_id))
	{
		UTI_ERROR("encapsulation packet is for ST with ID %ld "
		          "that is not registered\n", tal_id);
		goto error;
	}
	do_advertise_modcod = !this->fmt_simu->isCurrentFwdModcodAdvertised(tal_id);
	if(!do_advertise_modcod)
	{
		modcod_id = this->fmt_simu->getCurrentFwdModcodId(tal_id);
	}
	else
	{
		modcod_id = this->fmt_simu->getPreviousFwdModcodId(tal_id);
	}
	UTI_DEBUG_L3("MODCOD for ST ID %ld = %u (changed = %s)\n",
	             tal_id, modcod_id,
	             do_advertise_modcod ? "yes" : "no");

/*	if(do_advertise_modcod)
	{
		if(!this->createOptionModcod(comp, nb_row, *modcod_id, id))
		{
	}*/

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

	modcod_definitions = this->fmt_simu->getFwdModcodDefinitions();

	if(!modcod_definitions->doFmtIdExist(modcod_id))
	{
		UTI_ERROR("failed to found the definition of MODCOD ID %u\n",
		          modcod_id);
		goto error;
	}
	spectral_efficiency = modcod_definitions->getSpectralEfficiency(modcod_id);

	// duration is calculated over the complete BBFrame size, the BBFrame data
	// size represents the payload without coding
	bbframe_size_sym = (bbframe_size_bytes * 8) / spectral_efficiency;

	UTI_DEBUG("size of the BBFRAME = %u symbols\n", bbframe_size_sym);

	return true;

error:
	return false;
}


bool ForwardSchedulingS2::getIncompleteBBFrame(unsigned int tal_id,
                                               BBFrame **bbframe)
{
	map<unsigned int, BBFrame *>::iterator iter;
	unsigned int modcod_id;

	*bbframe = NULL;

	// retrieve the current MODCOD for the ST
	if(!this->retrieveCurrentModcod(tal_id, modcod_id))
	{
		// cannot get modcod for the ST skip this element
		goto skip;
	}

	// find if the BBFrame exists
	iter = this->incomplete_bb_frames.find(modcod_id);
	if(iter != this->incomplete_bb_frames.end() && (*iter).second != NULL)
	{
		UTI_DEBUG("Found a BBFrame for MODCOD %u\n", modcod_id);
		*bbframe = (*iter).second;
	}
	// no BBFrame for this MOCDCOD create a new one
	else
	{
		UTI_DEBUG("Create a new BBFrame for MODCOD %u\n", modcod_id);
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
		UTI_ERROR("failed to get BB frame size (MODCOD ID = %u)\n", modcod_id);
		return status_error;
	}

	// not enough space for this BBFrame
	if(remaining_capacity_sym < bbframe_size_sym)
	{
		UTI_DEBUG("not enough capacity (%u symbols) for the BBFrame of "
		          "size %u symbols\n", remaining_capacity_sym, bbframe_size_sym);
		return status_full;
	}

	// we can send the BBFrame
	complete_bb_frames->push_back(bbframe);

	// reduce the time carrier capacity by the BBFrame size
	remaining_capacity_sym -= bbframe_size_sym;;

	return status_ok;
}


void ForwardSchedulingS2::schedulePending(const list<unsigned int> supported_modcods,
                                          list<DvbFrame *> *complete_dvb_frames,
                                          vol_sym_t &remaining_capacity_sym)
{

	if(this->pending_bbframes.size() > 0)
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
				UTI_ERROR("cannot add pending BBFrame in the list "
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

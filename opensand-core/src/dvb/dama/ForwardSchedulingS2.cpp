/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @author   Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */



#include "ForwardSchedulingS2.h"
#include "MacFifoElement.h"

#include <opensand_output/Output.h>

#include <cassert>
#include <sstream>


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


ForwardSchedulingS2::ForwardSchedulingS2(time_ms_t fwd_timer_ms,
                                         EncapPlugin::EncapPacketHandler *packet_handler,
                                         const fifos_t &fifos,
                                         const StFmtSimuList *const fwd_sts,
                                         const FmtDefinitionTable *const fwd_modcod_def,
                                         const TerminalCategoryDama *const category,
                                         spot_id_t spot, 
                                         bool is_gw, 
                                         tal_id_t gw_id,
                                         string dst_name):
	Scheduling(packet_handler, fifos, fwd_sts),
	fwd_timer_ms(fwd_timer_ms),
	incomplete_bb_frames(),
	incomplete_bb_frames_ordered(),
	pending_bbframes(),
	fwd_modcod_def(fwd_modcod_def),
	category(category),
	spot_id(spot),
	probe_section("")
{
	vector<CarriersGroupDama *> carriers_group;
	vector<CarriersGroupDama *>::iterator carrier_it;
	string label = this->category->getLabel();
	std::stringstream section;
	char probe_name[128];

	section << "Spot_" << (unsigned int)this->spot_id;
	if(!is_gw)
	{
		section << ".GW_" << (unsigned int)gw_id;
	}
	section << "." << label << "." << dst_name << " ";
	this->probe_section = section.str();

	snprintf(probe_name, sizeof(probe_name),
	         "%sDown/Forward capacity.Total.Available",
	         this->probe_section.c_str());
	this->probe_fwd_total_capacity = Output::registerProbe<int>(
		probe_name, "Symbols per frame", true, SAMPLE_LAST);
	snprintf(probe_name, sizeof(probe_name),
	         "%sDown/Forward capacity.Total.Remaining",
	         this->probe_section.c_str());
	this->probe_fwd_total_remaining_capacity = Output::registerProbe<int>(
		probe_name, "Symbols per frame", true, SAMPLE_LAST);
	snprintf(probe_name, sizeof(probe_name),
	         "%sBBFrame number",
	         this->probe_section.c_str());
	this->probe_bbframe_nbr = Output::registerProbe<int>(
		probe_name, true, SAMPLE_AVG);

	carriers_group = this->category->getCarriersGroups();
	for(carrier_it = carriers_group.begin();
	    carrier_it != carriers_group.end();
	    ++carrier_it)
	{
		CarriersGroupDama *carriers = *carrier_it;
		vector<CarriersGroupDama *> vcm_carriers;
		vector<CarriersGroupDama *>::iterator vcm_it;
		vector<Probe<int> *> remain_probes;
		vector<Probe<int> *> avail_probes;
		unsigned int carriers_id = carriers->getCarriersId();
	
		vcm_carriers = carriers->getVcmCarriers();
		for(vcm_it = vcm_carriers.begin();
		    vcm_it != vcm_carriers.end();
		    ++vcm_it)
		{
			this->checkBBFrameSize(vcm_it, vcm_carriers);
			this->createProbes(vcm_it, vcm_carriers, remain_probes,
			                   avail_probes, carriers_id);
		}
		this->probe_fwd_available_capacity.insert(
			std::make_pair<unsigned int, vector<Probe<int> *> >((unsigned int) carriers_id,
			                                                    (vector<Probe<int> *>) avail_probes));
		this->probe_fwd_remaining_capacity.insert(
			std::make_pair<unsigned int, vector<Probe<int> *> >((unsigned int) carriers_id,
			                                                    (vector<Probe<int> * >) remain_probes));
	}
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

	delete this->category;
}


bool ForwardSchedulingS2::schedule(const time_sf_t current_superframe_sf,
                                   clock_t current_time,
                                   list<DvbFrame *> *complete_dvb_frames,
                                   uint32_t &remaining_allocation)
{
	fifos_t::const_iterator fifo_it;
	vector<CarriersGroupDama *> carriers_group;
	vector<CarriersGroupDama *>::iterator carrier_it;
	carriers_group = this->category->getCarriersGroups();
	vol_sym_t init_capacity_sym;
	int total_capa = 0;

	for(carrier_it = carriers_group.begin();
	    carrier_it != carriers_group.end();
	    ++carrier_it)
	{
		CarriersGroupDama *carriers = *carrier_it;
		vector<CarriersGroupDama *> vcm_carriers;
		vector<CarriersGroupDama *>::iterator vcm_it;
		unsigned int vcm_id = 0;
			
		vcm_carriers = carriers->getVcmCarriers();
		// if no VCM, getVcm() will return only one carrier
		for(vcm_it = vcm_carriers.begin();
		    vcm_it != vcm_carriers.end();
		    ++vcm_it)
		{
			list<BBFrame *>::iterator it;
			CarriersGroupDama *vcm = *vcm_it;
			unsigned int capacity_sym = 0;

			// initialize carriers capacity, remaining capacity should be 0
			// as we use previous capacity to keep track of unused capacity here
			init_capacity_sym = vcm->getTotalCapacity() +
			                    vcm->getRemainingCapacity();
			vcm->setRemainingCapacity(init_capacity_sym);
			total_capa += init_capacity_sym;

			for(fifo_it = this->dvb_fifos.begin();
			    fifo_it != this->dvb_fifos.end(); ++fifo_it)
			{
				DvbFifo *fifo = (*fifo_it).second;

				// check if the FIFO can emit on this carriers group
				if(vcm_carriers.size() <= 1)
				{
					// ACM
					if(fifo->getAccessType() != access_acm)
					{
						LOG(this->log_scheduling, LEVEL_DEBUG,
						    "SF#%u: Ignore carriers with id %u in category %s "
						    "for non-ACM fifo %s\n",
						    current_superframe_sf,
						    carriers->getCarriersId(),
						    this->category->getLabel().c_str(),
						    fifo->getName().c_str());
						continue;
					}
				}
				else
				{
					// VCM
					if(fifo->getAccessType() != access_vcm)
					{
						LOG(this->log_scheduling, LEVEL_DEBUG,
						    "SF#%u: Ignore carriers with id %u in category %s "
						    "for non-VCM fifo %s\n",
						    current_superframe_sf,
						    carriers->getCarriersId(),
						    this->category->getLabel().c_str(),
						    fifo->getName().c_str());
						continue;
					}
					if(fifo->getVcmId() != vcm_id)
					{
						continue;
					}
				}
				LOG(this->log_scheduling, LEVEL_DEBUG,
				    "SF#%u: Can send data from fifo %s on carriers group "
				    "%u in category %s\n",
				    current_superframe_sf,
				    fifo->getName().c_str(), carriers->getCarriersId(),
				    this->category->getLabel().c_str());

				if(!this->scheduleEncapPackets(fifo,
				                               current_superframe_sf,
				                               current_time,
				                               complete_dvb_frames,
				                               *vcm_it))
				{
					return false;
				}
				// TODO with VCM, previous capacity should be handled differently
			}

			// try to fill the BBFrames list with the remaining
			// incomplete BBFrames
			capacity_sym = vcm->getRemainingCapacity();
			for(it = this->incomplete_bb_frames_ordered.begin();
			    it != this->incomplete_bb_frames_ordered.end();
			    it = this->incomplete_bb_frames_ordered.erase(it))
			{
				int ret; 
				if(capacity_sym <= 0)
				{
					break;
				}

				ret = this->addCompleteBBFrame(complete_dvb_frames, *it,
				                               current_superframe_sf,
				                               capacity_sym);
				if(ret == status_error)
				{
					return false;
				}
				else if(ret == status_ok)
				{
					unsigned int modcod = (*it)->getModcodId();

					this->incomplete_bb_frames.erase(modcod);
					// incomplete ordered erased in loop
				}
				else if(ret == status_full)
				{
					time_sf_t next_sf = current_superframe_sf + 1;
					// we keep the remaining capacity that won't be used for
					// next frame
					(*vcm_it)->setPreviousCapacity(std::min(capacity_sym,
					                                        init_capacity_sym),
					                               next_sf);
					break;
				}
			}
			// update remaining capacity for statistics
			(*vcm_it)->setRemainingCapacity(std::min(capacity_sym,
			                                         init_capacity_sym));
			vcm_id++;
		}
	}
	this->probe_fwd_total_capacity->put(total_capa);
	this->probe_bbframe_nbr->put(complete_dvb_frames->size());

	for(carrier_it = carriers_group.begin();
	    carrier_it != carriers_group.end();
	    ++carrier_it)
	{
		CarriersGroupDama *carriers = *carrier_it;
		vector<CarriersGroupDama *> vcm_carriers;
		vector<CarriersGroupDama *>::iterator vcm_it;
		unsigned int carriers_id = carriers->getCarriersId();
		unsigned int id = 0;

		vcm_carriers = carriers->getVcmCarriers();
		for(vcm_it = vcm_carriers.begin();
		    vcm_it != vcm_carriers.end();
		    ++vcm_it)
		{
			unsigned int remain = (*vcm_it)->getRemainingCapacity();
			unsigned int avail = (*vcm_it)->getTotalCapacity();
			// keep total remaining capacity (for stats)
			remaining_allocation += remain;

			// get remain in Kbits/s instead of symbols if possible
			if((*vcm_it)->getFmtIds().size() == 1)
			{
				remain = this->fwd_modcod_def->symToKbits((*vcm_it)->getFmtIds().front(),
				                                 remain);
				avail = this->fwd_modcod_def->symToKbits((*vcm_it)->getFmtIds().front(),
				                               avail);
				// we get kbits per frame, convert in kbits/s
				remain = remain * 1000 / this->fwd_timer_ms;
				avail = avail * 1000 / this->fwd_timer_ms;
			}

			// If the probes doesn't exist
			// (in case of carrier reallocation with SVNO interface),
			// create them
			if(this->probe_fwd_available_capacity.find(carriers_id)
			   == this->probe_fwd_available_capacity.end())
			{
				vector<Probe<int> *> remain_probes;
				vector<Probe<int> *> avail_probes;

				this->createProbes(vcm_it, vcm_carriers, remain_probes,
				                   avail_probes, carriers_id);

				this->probe_fwd_available_capacity.insert(
				    std::make_pair<unsigned int, vector<Probe<int> *> >((unsigned int)carriers_id,
				                                                        (vector<Probe<int> *>)avail_probes));
				this->probe_fwd_remaining_capacity.insert(
				    std::make_pair<unsigned int, vector<Probe<int> *> >((unsigned int)carriers_id,
				                                                        (vector<Probe<int> *>)remain_probes));
			}

			this->probe_fwd_available_capacity[carriers_id][id]->put(avail);
			this->probe_fwd_remaining_capacity[carriers_id][id]->put(remain);
			id++;
			// reset remaining capacity
			(*vcm_it)->setRemainingCapacity(0);
		}
	}
	this->probe_fwd_total_remaining_capacity->put(remaining_allocation);

	return true;
}

bool ForwardSchedulingS2::scheduleEncapPackets(DvbFifo *fifo,
                                               const time_sf_t current_superframe_sf,
                                               clock_t current_time,
                                               list<DvbFrame *> *complete_dvb_frames,
                                               CarriersGroupDama *carriers)
{
	int ret;
	unsigned int sent_packets = 0;
	MacFifoElement *elem;
	long max_to_send;
	BBFrame *current_bbframe;
	list<fmt_id_t> supported_modcods = carriers->getFmtIds();
	vol_sym_t capacity_sym = carriers->getRemainingCapacity();
	vol_sym_t previous_sym = carriers->getPreviousCapacity(current_superframe_sf);
	vol_sym_t init_capa = capacity_sym;
	capacity_sym += previous_sym;

	// retrieve the number of packets waiting for retransmission
	max_to_send = fifo->getCurrentSize();
	if (max_to_send <= 0 && this->pending_bbframes.size() == 0)
	{
		// reset previous capacity
		carriers->setPreviousCapacity(0, 0);
		// set the remaining capacity for incomplete frames scheduling
		carriers->setRemainingCapacity(capacity_sym);
		goto skip;
	}

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: Scheduling FIFO %s, carriers group %u, "
	    "capacity is %u symbols (+ %u previous)\n",
	    current_superframe_sf,
	    fifo->getName().c_str(), carriers->getCarriersId(),
	    capacity_sym, previous_sym);

	// first add the pending complete BBFrame in the complete BBframes list
	// we add previous remaining capacity here because if a BBFrame was
	// not send before, previous_capacity contains the remaining capacity at the
	// end of the previous frame
	this->schedulePending(supported_modcods, current_superframe_sf,
	                      complete_dvb_frames, capacity_sym);
	// reset previous capacity
	carriers->setPreviousCapacity(0, 0);

	// all the previous capacity was not consumed, remove it as we are not on
	// pending frames anymore of if there is no incomplete frame
	// (we consider incomplete frames can use previous capacity)
	if(this->incomplete_bb_frames.size() == 0)
	{
		capacity_sym = std::min(init_capa, capacity_sym);
	}

	// stop if there is nothing to send
	if(max_to_send <= 0)
	{
		// set the remaining capacity for incomplete frames scheduling
		carriers->setRemainingCapacity(capacity_sym);
		goto skip;
	}

	// there are really packets to send
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: send at most %ld encapsulation packets "
	    "for %s fifo\n", current_superframe_sf,
	    max_to_send, fifo->getName().c_str());

	// now build BB frames with packets extracted from the MAC FIFO
	while(fifo->getCurrentSize() > 0)
	{
		NetPacket *encap_packet;
		tal_id_t tal_id;
		NetPacket *data;
		bool partial_encap;

		// simulate the satellite delay
		if(fifo->getTickOut() > current_time)
		{
			LOG(this->log_scheduling, LEVEL_INFO,
			    "SF#%u: packet is not scheduled for the "
			    "moment, break\n", current_superframe_sf); 
			    // this is the first MAC FIFO element that is not ready yet,
			    // there is no more work to do, break now
			    break;
		}

		elem = fifo->pop();

		encap_packet = elem->getElem<NetPacket>();
		// retrieve the encapsulation packet
		if(encap_packet == NULL)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
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
			tal_id = this->simu_sts->getTalIdWithLowerModcod();
			if(tal_id == 255)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: The scheduling of a "
				    "multicast frame failed\n",
				    current_superframe_sf);
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: The Tal_Id corresponding to "
				    "the terminal using the lower modcod can not "
				    "be retrieved\n", current_superframe_sf);
				goto error;
			}
			LOG(this->log_scheduling, LEVEL_INFO,
			    "SF#%u: TAL_ID corresponding to lower "
			    "MODCOD = %u\n", current_superframe_sf,
			    tal_id);
		}

		if(!this->getIncompleteBBFrame(tal_id, carriers, current_superframe_sf,
		                               &current_bbframe))
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

		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "SF#%u: Got the BBFrame for packet #%u, "
		    "there is now %zu complete BBFrames and %zu "
		    "incomplete\n", current_superframe_sf,
		    sent_packets + 1, complete_dvb_frames->size(),
		    this->incomplete_bb_frames.size());

		// Encapsulate packet
		ret = this->packet_handler->encapNextPacket(encap_packet,
			current_bbframe->getFreeSpace(),
			current_bbframe->getPacketsCount() == 0,
			partial_encap,
			&data);
		if(!ret)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: error while processing packet "
			    "#%u\n", current_superframe_sf,
			    sent_packets + 1);
			delete elem;
			delete encap_packet;
		}
		if(!data && !partial_encap)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: bad getChunk function "
			    "implementation, assert or skip packet #%u\n",
			    current_superframe_sf,
			    sent_packets + 1);
			assert(0);
			delete elem;
			delete encap_packet;
		}
		if(data)
		{
			// Add data
			if(!current_bbframe->addPacket(data))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: failed to add encapsulation "
				    "packet #%u->in BB frame with MODCOD ID %u "
				    "(packet length %zu, free space %zu)",
				    current_superframe_sf,
				    sent_packets + 1,
				    current_bbframe->getModcodId(),
				    data->getTotalLength(),
				    current_bbframe->getFreeSpace());
				goto error_fifo_elem;
			}

			if(partial_encap)
			{
				LOG(this->log_scheduling, LEVEL_INFO,
				    "SF#%u: packet fragmented",
				    current_superframe_sf);
			}
			// delete the NetPacket once it has been copied in the BBFrame
			delete data;
			sent_packets++;
		}
		else
		{
			LOG(this->log_scheduling, LEVEL_INFO,
			    "SF#%u: not enough free space in BBFrame "
			    "(%zu bytes) for %s packet (%zu bytes)\n",
			    current_superframe_sf,
			    current_bbframe->getFreeSpace(),
			    this->packet_handler->getName().c_str(),
			    encap_packet->getTotalLength());
		}
		if(partial_encap)
		{
			// Re-insert packet
			fifo->pushFront(elem);
		}
		else
		{
			// Delete packet	
			delete elem;
			delete encap_packet;
		}

		// the BBFrame has been completed or the next packet is too long
		// add the BBFrame in the list of complete BBFrames and decrease
		// duration credit
		if(current_bbframe->getFreeSpace() <= 0 || partial_encap)
		{
			ret = this->addCompleteBBFrame(complete_dvb_frames,
			                               current_bbframe,
			                               current_superframe_sf,
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

	if(sent_packets != 0)
	{
		unsigned int cpt_frame = complete_dvb_frames->size();

		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: %u %s been scheduled and %u BB %s "
		    "completed\n", current_superframe_sf,
		    sent_packets,
		    (sent_packets > 1) ? "packets have" : "packet has",
		    cpt_frame,
		    (cpt_frame > 1) ? "frames were" : "frame was");
	}
	// update remaining capacity for incomplete frames scheduling
	carriers->setRemainingCapacity(capacity_sym);

skip:
	return true;
error_fifo_elem:
	delete elem;
error:
	return false;
}


bool ForwardSchedulingS2::createIncompleteBBFrame(BBFrame **bbframe,
                                                  const time_sf_t current_superframe_sf,
                                                  unsigned int modcod_id)
{
	// if there is no incomplete BB frame create a new one
	size_t bbframe_size_bytes;

	*bbframe = new BBFrame();
	if(bbframe == NULL)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: failed to create an incomplete BB frame\n",
		    current_superframe_sf);
		goto error;
	}

	if(!this->packet_handler)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: packet handler is NULL\n",
		    current_superframe_sf);
		goto error;
	}

	// set the MODCOD ID of the BB frame
	(*bbframe)->setModcodId(modcod_id);

	// get the payload size
	// to simulate the modcod applied to transmitted data, we limit the
	// size of the BBframe to be the payload size
	bbframe_size_bytes = this->getBBFrameSizeBytes(modcod_id);
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: size of the BBFRAME for MODCOD %u = %zu\n",
	    current_superframe_sf,
	    modcod_id, bbframe_size_bytes);

	// set the size of the BB frame
	(*bbframe)->setMaxSize(bbframe_size_bytes);

	return true;

error:
	return false;
}



bool ForwardSchedulingS2::getBBFrameSizeSym(size_t bbframe_size_bytes,
                                            unsigned int modcod_id,
                                            const time_sf_t current_superframe_sf,
                                            vol_sym_t &bbframe_size_sym)
{
	float spectral_efficiency;


	if(!this->fwd_modcod_def->doFmtIdExist(modcod_id))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: failed to found the definition of MODCOD ID %u\n",
		    current_superframe_sf, modcod_id);
		goto error;
	}
	spectral_efficiency = this->fwd_modcod_def->getSpectralEfficiency(modcod_id);

	// duration is calculated over the complete BBFrame size, the BBFrame data
	// size represents the payload without coding
	bbframe_size_sym = (bbframe_size_bytes * 8) / spectral_efficiency;

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "size of the BBFRAME = %u symbols\n", bbframe_size_sym);

	return true;

error:
	return false;
}

unsigned int ForwardSchedulingS2::getBBFrameSizeBytes(unsigned int modcod_id)
{
	// get the payload size
	FmtDefinition *fmt_def = this->fwd_modcod_def->getDefinition(modcod_id);
	if(fmt_def == NULL)
	{
		// TODO: remove default value. Calling methods should check that return
		// value is OK.
		size_t bbframe_size = getPayloadSize("");
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "could not find fmt definition with id %u, use bbframe size %u bytes",
		    modcod_id, bbframe_size);
		return bbframe_size;
	}
	return getPayloadSize(fmt_def->getCoding());
}




bool ForwardSchedulingS2::getIncompleteBBFrame(tal_id_t tal_id,
                                               CarriersGroupDama *carriers,
                                               const time_sf_t current_superframe_sf,
                                               BBFrame **bbframe)
{
	map<unsigned int, BBFrame *>::iterator iter;
	unsigned int desired_modcod;
	unsigned int modcod_id;

	*bbframe = NULL;

	// retrieve the current MODCOD for the ST
	if(!this->simu_sts->isStPresent(tal_id))
	{
		LOG(this->log_scheduling, LEVEL_WARNING,
		    "encapsulation packet is for ST%u that is not registered\n",
		    tal_id);
		goto skip;
	}
	desired_modcod = this->getCurrentModcodId(tal_id);
	if(desired_modcod == 0)
	{
		// cannot get modcod for the ST skip this element
		goto skip;
	}

	// get best modcod ID according to carrier
	modcod_id = carriers->getNearestFmtId(desired_modcod);
	if(modcod_id == 0)
	{
		LOG(this->log_scheduling, LEVEL_WARNING,
		    "SF#%u: cannot serve terminal %u with any modcod (desired %u) "
		    "on carrier %u\n", current_superframe_sf, tal_id, desired_modcod,
		    carriers->getCarriersId());

		goto skip;
	}
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: Available MODCOD for ST id %u = %u\n",
	    current_superframe_sf, tal_id, modcod_id);

	// find if the BBFrame exists
	iter = this->incomplete_bb_frames.find(modcod_id);
	if(iter != this->incomplete_bb_frames.end() && (*iter).second != NULL)
	{
		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "SF#%u: Found a BBFrame for MODCOD %u\n",
		    current_superframe_sf, modcod_id);
		*bbframe = (*iter).second;
	}
	// no BBFrame for this MOCDCOD create a new one
	else
	{
		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: Create a new BBFrame for MODCOD %u\n",
		    current_superframe_sf, modcod_id);
		// if there is no incomplete BB frame create a new one
		if(!this->createIncompleteBBFrame(bbframe, current_superframe_sf,
		                                  modcod_id))
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
                                                       const time_sf_t current_superframe_sf,
                                                       vol_sym_t &remaining_capacity_sym)
{
	unsigned int modcod_id = bbframe->getModcodId();
	size_t bbframe_size_bytes = bbframe->getMaxSize();
	vol_sym_t bbframe_size_sym;

	// how much time do we need to send the BB frame ?
	if(!this->getBBFrameSizeSym(bbframe_size_bytes, modcod_id,
	                            current_superframe_sf, bbframe_size_sym))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: failed to get BB frame size (MODCOD ID = %u)\n",
		    current_superframe_sf, modcod_id);
		return status_error;
	}

	// not enough space for this BBFrame
	if(remaining_capacity_sym < bbframe_size_sym)
	{
		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: not enough capacity (%u symbols) for the BBFrame of "
		    "size %u symbols\n", current_superframe_sf, remaining_capacity_sym,
		    bbframe_size_sym);
		return status_full;
	}

	// we can send the BBFrame
	complete_bb_frames->push_back((DvbFrame *)bbframe);
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: New complete BBFrame\n",
	    current_superframe_sf);

	// reduce the time carrier capacity by the BBFrame size
	remaining_capacity_sym -= bbframe_size_sym;

	return status_ok;
}


void ForwardSchedulingS2::schedulePending(const list<fmt_id_t> supported_modcods,
                                          const time_sf_t current_superframe_sf,
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
			sched_status_t status;
			status = this->addCompleteBBFrame(complete_dvb_frames,
			                                  (*it),
			                                  current_superframe_sf,
			                                  remaining_capacity_sym);
			if(status == status_full)
			{
				// keep the BBFrame in pending list
				new_pending.push_back(*it);
			}
			else if(status != status_ok)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: cannot add pending BBFrame in the list "
				    "of complete BBFrames\n", current_superframe_sf);
			}
		}
		else
		{
			// keep the BBFrame in pending list
			new_pending.push_back(*it);
		}
	}
	if(complete_dvb_frames->size() > 0)
	{
		LOG(this->log_scheduling, LEVEL_INFO,
		    "%zu pending frames scheduled, %zu remaining\n",
		    complete_dvb_frames->size(), new_pending.size());
	}
	this->pending_bbframes.clear();
	this->pending_bbframes.insert(this->pending_bbframes.end(),
	                              new_pending.begin(), new_pending.end());

}


void ForwardSchedulingS2::checkBBFrameSize(vector<CarriersGroupDama *>::iterator vcm_it,
                                           vector<CarriersGroupDama *> vcm_carriers)
{
	unsigned int vcm_id = 0;
	CarriersGroupDama *vcm = *vcm_it;
	vol_sym_t carrier_size_sym = vcm->getTotalCapacity() /
	                             vcm->getCarriersNumber();
	list<fmt_id_t> fmt_ids = vcm->getFmtIds();

	for(list<fmt_id_t>::const_iterator fmt_it = fmt_ids.begin();
	    fmt_it != fmt_ids.end(); ++fmt_it)
	{
		fmt_id_t fmt_id = *fmt_it;
		vol_sym_t size;
		// check that the BBFrame maximum size is smaller than the carrier size
		if(!this->getBBFrameSizeSym(this->getBBFrameSizeBytes(fmt_id),
		                            fmt_id,
		                            0,
		                            size))
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "Cannot determine the maximum BBFrame size for MODCOD %u\n", fmt_id);
			continue;
		}
		if(vcm_carriers.size() > 1)
		{
			LOG(this->log_scheduling, LEVEL_WARNING,
			    "Category %s, Carriers group %u VCM %u: the BBFrame size "
			    "with MODCOD %u (%u symbols) is greater than the carrier "
			    "size %u. This MODCOD will not work.\n",
			    this->category->getLabel().c_str(),
			    vcm->getCarriersId(), vcm_id, fmt_id,
			    size, carrier_size_sym);
		}
		else
		{
			if(vcm->getFmtIds().size() == 1)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
						"Category %s, Carriers group %u: the BBFrame size "
						"of MODCOD %u (%u symbols) is greater than "
						"the carrier size %u. This MODCOD will not work.\n",
						this->category->getLabel().c_str(),
						vcm->getCarriersId(), fmt_id,
						size, carrier_size_sym);
			}
			else
			{
				LOG(this->log_scheduling, LEVEL_WARNING,
						"Category %s, Carriers group %u: the BBFrame size "
						"with MODCOD %u (%u symbols) is greater than the carrier "
						"size %u. This MODCOD will not work.\n",
						this->category->getLabel().c_str(),
						vcm->getCarriersId(), fmt_id,
						size, carrier_size_sym);
			}
		}
	}
}


void ForwardSchedulingS2::createProbes(vector<CarriersGroupDama *>::iterator vcm_it,
                                       vector<CarriersGroupDama *> vcm_carriers,
                                       vector<Probe<int> *> &remain_probes,
                                       vector<Probe<int> *> &avail_probes,
                                       unsigned int carriers_id)
{
	unsigned int vcm_id = 0;
	CarriersGroupDama *vcm = *vcm_it;
	Probe<int> *remain_probe;
	Probe<int> *avail_probe;
	char probe_name[128];

	// For units, if there is only one MODCOD use Kbits/s else symbols
	// check if the FIFO can emit on this carriers group
	if(vcm_carriers.size() <= 1)
	{
		string type = "ACM";
		string unit = "Symbol number";
		if(vcm->getFmtIds().size() == 1)
		{
			unit = "Kbits/s";
			type = "CCM";
		}
		snprintf(probe_name, sizeof(probe_name),
		         "%sDown/Forward capacity.Carrier%u.%s.Remaining",
		         this->probe_section.c_str(), carriers_id, type.c_str());
		remain_probe = Output::registerProbe<int>(
				probe_name,    
				unit,
				true,
				SAMPLE_AVG);
		snprintf(probe_name, sizeof(probe_name),
		         "%sDown/Forward capacity.Carrier%u.%s.Available",
		         this->probe_section.c_str(), carriers_id, type.c_str());
		avail_probe = Output::registerProbe<int>(
				probe_name,
				unit,
				true,
				SAMPLE_AVG);
	}
	else
	{
		snprintf(probe_name, sizeof(probe_name),
		         "%sDown/Forward capacity.Carrier%u.VCM%u.Remaining",
		         this->probe_section.c_str(), carriers_id, vcm_id);
		remain_probe = Output::registerProbe<int>(
				probe_name,
				"Kbits/s",
				true,
				SAMPLE_AVG);
		snprintf(probe_name, sizeof(probe_name),
		         "%sDown/Forward capacity.Carrier%u.VCM%u.Available",
		         this->probe_section.c_str(), carriers_id, vcm_id);
		avail_probe = Output::registerProbe<int>(
				probe_name,
				"Kbits/s",
				true,
				SAMPLE_AVG);
		vcm_id++;
	}
	avail_probes.push_back(avail_probe);
	remain_probes.push_back(remain_probe);
}

// TODO scheduling improvement
// At the moment, incomplete BBFrames that can not be sent are kept:
//  1 - until they are completed
//  2 - until there is space to send them
//  In first case, we have a problem if no terminal required the same
//  modcod, the BBFrame will wait forever to be completed and we will
//  have to wait case 2 for the BBFrame to be sent
//  One way to improve this algo, use a counter :
//   - first: if the counter is reached, try to complete the frame
//            with packet requiring higher MODCODs
//   - second: (the frame is still not completed) force sending the incomplete
//             frame
//  Another way : keep the frame in completes and try to complete it
//                during scheduling

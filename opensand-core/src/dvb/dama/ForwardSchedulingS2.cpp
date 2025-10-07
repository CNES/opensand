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


#include <opensand_output/Output.h>

#include "ForwardSchedulingS2.h"
#include "DvbFifo.h"
#include "FifoElement.h"
#include "StFmtSimu.h"
#include "OpenSandModelConf.h"


/**
 * @brief Get the payload size in Bytes according to coding rate
 *
 * @param coding_rate  The coding rate
 * @return the payload size in Bytes
 */
static size_t getPayloadSize(std::string coding_rate)
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


ForwardSchedulingS2::ForwardSchedulingS2(time_us_t fwd_timer,
                                         std::shared_ptr<EncapPlugin> packet_handler,
                                         std::shared_ptr<fifos_t> fifos,
                                         std::shared_ptr<const StFmtSimuList> fwd_sts,
                                         const FmtDefinitionTable &fwd_modcod_def,
                                         std::shared_ptr<TerminalCategoryDama> category,
                                         spot_id_t spot, 
                                         bool is_gw, 
                                         tal_id_t,
                                         std::string dst_name):
	Scheduling(packet_handler, fifos, fwd_sts),
	fwd_timer(fwd_timer),
	incomplete_bb_frames(),
	incomplete_bb_frames_ordered(),
	pending_bbframes(),
	fwd_modcod_def(fwd_modcod_def),
	category(category),
	spot_id(spot),
	probe_section("")
{
	auto output = Output::Get();
	std::string label = this->category->getLabel();

	// generate probes prefix
	bool is_sat = OpenSandModelConf::Get()->getComponentType() == Component::satellite;
	std::string prefix = generateProbePrefix(spot_id, is_gw ? Component::gateway : Component::terminal, is_sat);
	this->probe_section = prefix + label + "." + dst_name;

	this->probe_fwd_total_capacity =
	    output->registerProbe<int>(probe_section + "Down/Forward capacity.Total.Available",
	                               "Symbols per frame", true, SAMPLE_LAST);

	this->probe_fwd_total_remaining_capacity =
	    output->registerProbe<int>(probe_section + "Down/Forward capacity.Total.Remaining",
	                               "Symbols per frame", true, SAMPLE_LAST);
	this->probe_bbframe_nbr =
	    output->registerProbe<int>(probe_section + "Global.BBFrame number",
	                               true, SAMPLE_AVG);

	this->probe_gw_sent_modcod = output->registerProbe<int>(prefix + "Up_Forward_modcod.Sent_modcod",
	                                                        "modcod index",
	                                                        true, SAMPLE_LAST);

	for (auto &&carriers: this->category->getCarriersGroups())
	{
		std::vector<std::shared_ptr<Probe<int>>> remain_probes;
		std::vector<std::shared_ptr<Probe<int>>> avail_probes;
		unsigned int carriers_id = carriers.getCarriersId();

		auto &vcm_carriers = carriers.getVcmCarriers();
		for(auto &&vcm: vcm_carriers)
		{
			this->checkBBFrameSize(vcm, vcm_carriers);
			this->createProbes(vcm, vcm_carriers, remain_probes, avail_probes, carriers_id);
		}
		this->probe_fwd_available_capacity.emplace(carriers_id, avail_probes);
		this->probe_fwd_remaining_capacity.emplace(carriers_id, remain_probes);
	}
}

ForwardSchedulingS2::~ForwardSchedulingS2()
{
	this->incomplete_bb_frames_ordered.clear();
	this->incomplete_bb_frames.clear();
	this->pending_bbframes.clear();
}


bool ForwardSchedulingS2::schedule(const time_sf_t current_superframe_sf,
                                   std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                   uint32_t &remaining_allocation)
{
	vol_sym_t init_capacity_sym;
	int total_capa = 0;

	auto& carriers_group = this->category->getCarriersGroups();
	for (auto&& carriers : carriers_group)
	{
		unsigned int vcm_id = 0;
		auto &vcm_carriers = carriers.getVcmCarriers();
		// if no VCM, getVcm() will return only one carrier
		for(auto&& vcm : vcm_carriers)
		{
			vol_sym_t capacity_sym;
			vol_sym_t previous_sym;

			// initialize carriers capacity, remaining capacity should be 0
			// as we use previous capacity to keep track of unused capacity here
			init_capacity_sym = vcm.getTotalCapacity() + vcm.getRemainingCapacity();
			vcm.setRemainingCapacity(init_capacity_sym);
			total_capa += init_capacity_sym;

			capacity_sym = init_capacity_sym;
			previous_sym = vcm.getPreviousCapacity(current_superframe_sf);
			capacity_sym += previous_sym;

			for(auto&& [_key, fifo] : *(this->dvb_fifos))
			{
				// check if the FIFO can emit on this carriers group
				if(vcm_carriers.size() <= 1)
				{
					// ACM
					if(fifo->getAccessType() != ForwardOrReturnAccessType{ForwardAccessType::acm})
					{
						LOG(this->log_scheduling, LEVEL_DEBUG,
						    "SF#%u: Ignore carriers with id %u in category %s "
						    "for non-ACM fifo %s\n",
						    current_superframe_sf,
						    carriers.getCarriersId(),
						    this->category->getLabel().c_str(),
						    fifo->getName());
						continue;
					}
				}
				else
				{
					// VCM
					if(fifo->getAccessType() != ForwardOrReturnAccessType{ForwardAccessType::vcm})
					{
						LOG(this->log_scheduling, LEVEL_DEBUG,
						    "SF#%u: Ignore carriers with id %u in category %s "
						    "for non-VCM fifo %s\n",
						    current_superframe_sf,
						    carriers.getCarriersId(),
						    this->category->getLabel().c_str(),
						    fifo->getName());
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
				    fifo->getName(), carriers.getCarriersId(),
				    this->category->getLabel().c_str());

				if(!this->scheduleEncapPackets(*fifo,
				                               current_superframe_sf,
				                               complete_dvb_frames,
				                               vcm,
				                               capacity_sym,
				                               init_capacity_sym))
				{
					return false;
				}

				if(fifo->getCurrentSize() > 0)
				{
					// Still have data on FIFO, do not schedule BBF for lower QoS
					break;
				}
			}

			vcm.setPreviousCapacity(capacity_sym, current_superframe_sf + 1);

			// try to fill the BBFrames list with the remaining
			// incomplete BBFrames
			for(auto it = this->incomplete_bb_frames_ordered.begin();
			    it != this->incomplete_bb_frames_ordered.end();
			    it = this->incomplete_bb_frames_ordered.erase(it))
			{
				if(capacity_sym <= 0)
				{
					break;
				}

				auto& current_bbframe = this->incomplete_bb_frames.at(*it);
				auto ret = this->addCompleteBBFrame(complete_dvb_frames,
				                                    current_bbframe,
				                                    current_superframe_sf,
				                                    capacity_sym);
				if(ret == sched_status::error)
				{
					return false;
				}
				else if(ret == sched_status::ok)
				{
					this->incomplete_bb_frames.erase(*it);
					// incomplete ordered erased in loop
				}
				else if(ret == sched_status::full)
				{
					time_sf_t next_sf = current_superframe_sf + 1;
					// we keep the remaining capacity that won't be used for next frame
					vcm.setPreviousCapacity(std::min(capacity_sym, init_capacity_sym), next_sf);
					break;
				}
			}
			// update remaining capacity for statistics
			vcm.setRemainingCapacity(std::min(capacity_sym, init_capacity_sym));
			vcm_id++;
		}
	}
	this->probe_fwd_total_capacity->put(total_capa);
	this->probe_bbframe_nbr->put(complete_dvb_frames.size());

	for(auto&& carriers : carriers_group)
	{
		unsigned int carriers_id = carriers.getCarriersId();
		unsigned int id = 0;

		auto &vcm_carriers = carriers.getVcmCarriers();
		for(auto&& vcm: vcm_carriers)
		{
			unsigned int remain = vcm.getRemainingCapacity();
			unsigned int avail = vcm.getTotalCapacity();
			// keep total remaining capacity (for stats)
			remaining_allocation += remain;

			// get remain in Kbits/s instead of symbols if possible
			if(vcm.getFmtIds().size() == 1)
			{
				remain = this->fwd_modcod_def.symToKbits(vcm.getFmtIds().front(), remain);
				avail = this->fwd_modcod_def.symToKbits(vcm.getFmtIds().front(), avail);
				// we get kbits per frame, convert in kbits/s
				remain = std::chrono::seconds{remain} / this->fwd_timer;
				avail = std::chrono::seconds{avail} / this->fwd_timer;
			}

			// If the probes doesn't exist
			// (in case of carrier reallocation with SVNO interface),
			// create them
			if(this->probe_fwd_available_capacity.find(carriers_id)
			   == this->probe_fwd_available_capacity.end())
			{
				std::vector<std::shared_ptr<Probe<int>>> remain_probes;
				std::vector<std::shared_ptr<Probe<int>>> avail_probes;

				this->createProbes(vcm, vcm_carriers, remain_probes,
				                   avail_probes, carriers_id);

				this->probe_fwd_available_capacity.emplace(carriers_id, avail_probes);
				this->probe_fwd_remaining_capacity.emplace(carriers_id, remain_probes);
			}

			this->probe_fwd_available_capacity[carriers_id][id]->put(avail);
			this->probe_fwd_remaining_capacity[carriers_id][id]->put(remain);
			id++;
			// reset remaining capacity
			vcm.setPreviousCapacity(vcm.getRemainingCapacity(), current_superframe_sf + 1);
			vcm.setRemainingCapacity(0);
		}
	}
	this->probe_fwd_total_remaining_capacity->put(remaining_allocation);

	return true;
}


sched_status ForwardSchedulingS2::schedulePacket(const time_sf_t current_superframe_sf,
                                                 unsigned int &sent_packets,
                                                 vol_sym_t &capacity_sym,
                                                 CarriersGroupDama &carriers,
                                                 std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                                 Rt::Ptr<NetPacket> encap_packet)
{
	while (encap_packet)
	{
		Rt::Ptr<NetPacket> data = Rt::make_ptr<NetPacket>(nullptr);
		// retrieve the ST ID associated to the packet
		tal_id_t tal_id = encap_packet->getDstTalId();
		// This is a broadcast/multicast destination
		if(tal_id == BROADCAST_TAL_ID)
		{
			// Select the tal_id corresponding to the lower modcod in order to
			// make all terminal able to read the message
			tal_id = this->simu_sts->getTalIdWithLowerModcod();
			if (tal_id == std::numeric_limits<decltype(tal_id)>::max())
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: The scheduling of a "
				    "multicast frame failed\n",
				    current_superframe_sf);
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: The Tal_Id corresponding to "
				    "the terminal using the lower modcod can not "
				    "be retrieved\n", current_superframe_sf);
				return sched_status::error;
			}
			LOG(this->log_scheduling, LEVEL_INFO,
			    "SF#%u: TAL_ID corresponding to lower "
			    "MODCOD = %u\n", current_superframe_sf,
			    tal_id);
		}
	
		std::map<unsigned int, Rt::Ptr<BBFrame>>::iterator current_bbframe_it;
		if(!this->prepareIncompleteBBFrame(tal_id, carriers,
		                                   current_superframe_sf,
		                                   current_bbframe_it))
		{
			// cannot initialize incomplete BB Frame
			return sched_status::error;
		}
		else if(current_bbframe_it == this->incomplete_bb_frames.end())
		{
			// cannot get modcod for the ST delete the element
			return sched_status::ok;
		}
	
		auto modcod = current_bbframe_it->first;
		Rt::Ptr<BBFrame>& current_bbframe = current_bbframe_it->second;
	
		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "SF#%u: Got the BBFrame for packet #%u, "
		    "there is now %zu complete BBFrames and %zu "
		    "incomplete\n", current_superframe_sf,
		    sent_packets + 1, complete_dvb_frames.size(),
		    this->incomplete_bb_frames.size());
	
		// Encapsulate packet
		auto encap_packet_total_length = encap_packet->getTotalLength();
		if(!this->packet_handler->encapNextPacket(std::move(encap_packet),
		                                          current_bbframe->getFreeSpace(),
		                                          current_bbframe->getPacketsCount() == 0,
		                                          data, this->remaining_data))
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: error while processing packet "
			    "#%u\n", current_superframe_sf,
			    sent_packets + 1);
		}
		bool partial_encap = this->remaining_data != nullptr;
		if(data)
		{
			// Add data
			if(!current_bbframe->addPacket(*data))
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
				return sched_status::error;
			}

			if(partial_encap)
			{
				LOG(this->log_scheduling, LEVEL_INFO,
				    "SF#%u: packet fragmented",
				    current_superframe_sf);
			}
			// delete the NetPacket once it has been copied in the BBFrame
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
			    encap_packet_total_length);
		}

		// the BBFrame has been completed or the next packet is too long
		// add the BBFrame in the list of complete BBFrames and decrease
		// duration credit
		if(current_bbframe->getFreeSpace() <= 0 || partial_encap)
		{
			sched_status ret = this->addCompleteBBFrame(complete_dvb_frames,
			                                            current_bbframe,
			                                            current_superframe_sf,
			                                            capacity_sym);
			if(ret == sched_status::error)
			{
				return ret;
			}
			else
			{
				Rt::Ptr<BBFrame> pending_bbframe = std::move(current_bbframe);
				this->incomplete_bb_frames_ordered.remove(modcod);
				this->incomplete_bb_frames.erase(modcod);
				if(ret == sched_status::full)
				{
					time_sf_t next_sf = current_superframe_sf + 1;
					// we keep the remaining capacity that won't be used for
					// next frame
					carriers.setPreviousCapacity(capacity_sym, next_sf);
					this->pending_bbframes.push_back(std::move(pending_bbframe));
					return ret;
				}
			}
		}
	
		if(partial_encap)
		{
			encap_packet = std::move(this->remaining_data);
		}
	}

	return sched_status::ok;
}


bool ForwardSchedulingS2::scheduleEncapPackets(DvbFifo &fifo,
                                               const time_sf_t current_superframe_sf,
                                               std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                               CarriersGroupDama &carriers,
                                               vol_sym_t &capacity_sym,
                                               vol_sym_t init_capa)
{
	unsigned int sent_packets = 0;
	std::list<fmt_id_t> supported_modcods = carriers.getFmtIds();

	// retrieve the number of packets waiting for retransmission
	vol_pkt_t max_to_send = fifo.getCurrentSize();
	if (!max_to_send && this->pending_bbframes.size() == 0)
	{
		return true;
	}

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: Scheduling FIFO %s, carriers group %u, "
	    "capacity is %u symbols\n",
	    current_superframe_sf,
	    fifo.getName(), carriers.getCarriersId(),
	    capacity_sym);

	// first add the pending complete BBFrame in the complete BBframes list
	// we add previous remaining capacity here because if a BBFrame was
	// not send before, previous_capacity contains the remaining capacity at the
	// end of the previous frame
	this->schedulePending(supported_modcods, current_superframe_sf,
	                      complete_dvb_frames, capacity_sym);

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
		return true;
	}

	// there are really packets to send
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: send at most %ld encapsulation packets "
	    "for %s fifo\n", current_superframe_sf,
	    max_to_send, fifo.getName());

	if (sched_status::error == this->schedulePacket(current_superframe_sf,
	                                                sent_packets,
	                                                capacity_sym,
	                                                carriers,
	                                                complete_dvb_frames,
	                                                std::move(this->remaining_data)))
	{
		return false;
	}

	// now build BB frames with packets extracted from the MAC FIFO
	for (auto &&elem: fifo)
	{
		// retrieve the encapsulation packet
		Rt::Ptr<NetPacket> encap_packet = elem->releaseElem<NetPacket>();
		if(!encap_packet)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: invalid packet #%u in MAC FIFO "
			    "element\n", current_superframe_sf,
			    sent_packets + 1);
		}
		else
		{
			auto ret = this->schedulePacket(current_superframe_sf,
			                                sent_packets,
			                                capacity_sym,
			                                carriers,
			                                complete_dvb_frames,
			                                std::move(encap_packet));
			if (ret == sched_status::error)
			{
				return false;
			}
			else if (ret == sched_status::full)
			{
				break;
			}
		}
	}

	if(sent_packets != 0)
	{
		unsigned int cpt_frame = complete_dvb_frames.size();

		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: %u %s been scheduled and %u BB %s "
		    "completed\n", current_superframe_sf,
		    sent_packets,
		    (sent_packets > 1) ? "packets have" : "packet has",
		    cpt_frame,
		    (cpt_frame > 1) ? "frames were" : "frame was");
	}

	return true;
}


bool ForwardSchedulingS2::createIncompleteBBFrame(Rt::Ptr<BBFrame> &bbframe,
                                                  const time_sf_t current_superframe_sf,
                                                  unsigned int modcod_id)
{
	// if there is no incomplete BB frame create a new one
	size_t bbframe_size_bytes;

	try
	{
		bbframe = Rt::make_ptr<BBFrame>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: failed to create an incomplete BB frame\n",
		    current_superframe_sf);
		return false;
	}

	if(!this->packet_handler)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: packet handler is NULL\n",
		    current_superframe_sf);
		return false;
	}

	// set the MODCOD ID of the BB frame
	bbframe->setModcodId(modcod_id);

	// get the payload size
	// to simulate the modcod applied to transmitted data, we limit the
	// size of the BBframe to be the payload size
	bbframe_size_bytes = this->getBBFrameSizeBytes(modcod_id);
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: size of the BBFRAME for MODCOD %u = %zu\n",
	    current_superframe_sf,
	    modcod_id, bbframe_size_bytes);

	// set the size of the BB frame
	bbframe->setMaxSize(bbframe_size_bytes);

	return true;
}



bool ForwardSchedulingS2::getBBFrameSizeSym(size_t bbframe_size_bytes,
                                            unsigned int modcod_id,
                                            const time_sf_t current_superframe_sf,
                                            vol_sym_t &bbframe_size_sym)
{
	float spectral_efficiency;


	if(!this->fwd_modcod_def.doFmtIdExist(modcod_id))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: failed to found the definition of MODCOD ID %u\n",
		    current_superframe_sf, modcod_id);
		goto error;
	}
	spectral_efficiency = this->fwd_modcod_def.getSpectralEfficiency(modcod_id);

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
	try
	{
		FmtDefinition &fmt_def = this->fwd_modcod_def.getDefinition(modcod_id);
		return getPayloadSize(fmt_def.getCoding());
	}
	catch (const std::range_error&)
	{
		// TODO: remove default value. Calling methods should check that return
		// value is OK.
		size_t bbframe_size = getPayloadSize("");
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "could not find fmt definition with id %u, use bbframe size %u bytes",
		    modcod_id, bbframe_size);
		return bbframe_size;
	}
}


bool ForwardSchedulingS2::prepareIncompleteBBFrame(tal_id_t tal_id,
                                                   CarriersGroupDama &carriers,
                                                   const time_sf_t current_superframe_sf,
                                                   std::map<unsigned int, Rt::Ptr<BBFrame>>::iterator &it)
{
	it = this->incomplete_bb_frames.end();

	// retrieve the current MODCOD for the ST
	if(!this->simu_sts->isStPresent(tal_id))
	{
		LOG(this->log_scheduling, LEVEL_WARNING,
		    "encapsulation packet is for ST%u that is not registered\n",
		    tal_id);
		return true;
	}

	unsigned int desired_modcod = this->getCurrentModcodId(tal_id);
	if(desired_modcod == 0)
	{
		// cannot get modcod for the ST skip this element
		return true;
	}

	// get best modcod ID according to carrier
	unsigned int modcod_id = carriers.getNearestFmtId(desired_modcod);
	if(modcod_id == 0)
	{
		LOG(this->log_scheduling, LEVEL_WARNING,
		    "SF#%u: cannot serve terminal %u with any modcod (desired %u) "
		    "on carrier %u\n", current_superframe_sf, tal_id, desired_modcod,
		    carriers.getCarriersId());

		return true;
	}
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: Available MODCOD for ST id %u = %u\n",
	    current_superframe_sf, tal_id, modcod_id);

	// find if the BBFrame exists
	it = this->incomplete_bb_frames.find(modcod_id);
	if(it != this->incomplete_bb_frames.end() && it->second != nullptr)
	{
		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "SF#%u: Found a BBFrame for MODCOD %u\n",
		    current_superframe_sf, modcod_id);
	}
	// no BBFrame for this MOCDCOD create a new one
	else
	{
		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: Create a new BBFrame for MODCOD %u\n",
		    current_superframe_sf, modcod_id);
		// if there is no incomplete BB frame create a new one
		Rt::Ptr<BBFrame> bbframe = Rt::make_ptr<BBFrame>(nullptr);
		if(!this->createIncompleteBBFrame(bbframe, current_superframe_sf, modcod_id))
		{
			return false;
		}

		// add the BBFrame in the map and list
		it = this->incomplete_bb_frames.emplace(modcod_id, std::move(bbframe)).first;
		this->incomplete_bb_frames_ordered.push_back(modcod_id);
	}

	return true;
}


sched_status ForwardSchedulingS2::addCompleteBBFrame(std::list<Rt::Ptr<DvbFrame>> &complete_bb_frames,
                                                     Rt::Ptr<BBFrame>& bbframe,
                                                     const time_sf_t current_superframe_sf,
                                                     vol_sym_t &remaining_capacity_sym)
{
	unsigned int modcod_id = bbframe->getModcodId();
	size_t bbframe_size_bytes = bbframe->getMaxSize();
	vol_sym_t bbframe_size_sym;

	// TODO
	this->probe_gw_sent_modcod->put(modcod_id);

	// how much time do we need to send the BB frame ?
	if(!this->getBBFrameSizeSym(bbframe_size_bytes, modcod_id,
	                            current_superframe_sf, bbframe_size_sym))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: failed to get BB frame size (MODCOD ID = %u)\n",
		    current_superframe_sf, modcod_id);
		return sched_status::error;
	}

	// not enough space for this BBFrame
	if(remaining_capacity_sym < bbframe_size_sym)
	{
		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: not enough capacity (%u symbols) for the BBFrame of "
		    "size %u symbols\n", current_superframe_sf, remaining_capacity_sym,
		    bbframe_size_sym);
		return sched_status::full;
	}

	// we can send the BBFrame
	complete_bb_frames.push_back(dvb_frame_downcast(std::move(bbframe)));
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: New complete BBFrame\n",
	    current_superframe_sf);

	// reduce the time carrier capacity by the BBFrame size
	remaining_capacity_sym -= bbframe_size_sym;

	return sched_status::ok;
}


void ForwardSchedulingS2::schedulePending(const std::list<fmt_id_t> supported_modcods,
                                          const time_sf_t current_superframe_sf,
                                          std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                          vol_sym_t &remaining_capacity_sym)
{
	if(this->pending_bbframes.size() == 0)
	{
		return;
	}

	std::list<Rt::Ptr<BBFrame>> new_pending;
	for (auto&& pending_frame : this->pending_bbframes)
	{
		unsigned int modcod = pending_frame->getModcodId();

		if(std::find(supported_modcods.begin(), supported_modcods.end(), modcod) !=
		   supported_modcods.end())
		{
			sched_status status = this->addCompleteBBFrame(complete_dvb_frames,
			                                               pending_frame,
			                                               current_superframe_sf,
			                                               remaining_capacity_sym);
			if(status == sched_status::full)
			{
				// keep the BBFrame in pending list
				new_pending.push_back(std::move(pending_frame));
			}
			else if(status != sched_status::ok)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: cannot add pending BBFrame in the list "
				    "of complete BBFrames\n", current_superframe_sf);
			}
		}
		else
		{
			// keep the BBFrame in pending list
			new_pending.push_back(std::move(pending_frame));
		}
	}
	if(complete_dvb_frames.size() > 0)
	{
		LOG(this->log_scheduling, LEVEL_INFO,
		    "%zu pending frames scheduled, %zu remaining\n",
		    complete_dvb_frames.size(), new_pending.size());
	}

	this->pending_bbframes = std::move(new_pending);
}


void ForwardSchedulingS2::checkBBFrameSize(CarriersGroupDama &vcm,
                                           const std::vector<CarriersGroupDama> &vcm_carriers)
{
	unsigned int vcm_id = 0;
	vol_sym_t carrier_size_sym = vcm.getTotalCapacity() / vcm.getCarriersNumber();
	std::list<fmt_id_t> fmt_ids = vcm.getFmtIds();

	for(std::list<fmt_id_t>::const_iterator fmt_it = fmt_ids.begin();
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
		if(size <= carrier_size_sym)
		{
			// Carrier size is lower than the max BBFrame size
			continue;
		}
		if(vcm_carriers.size() > 1)
		{
			LOG(this->log_scheduling, LEVEL_WARNING,
			    "Category %s, Carriers group %u VCM %u: the BBFrame size "
			    "with MODCOD %u (%u symbols) is greater than the carrier "
			    "size %u. This MODCOD will not work.\n",
			    this->category->getLabel().c_str(),
			    vcm.getCarriersId(), vcm_id, fmt_id,
			    size, carrier_size_sym);
		}
		else
		{
			if(vcm.getFmtIds().size() == 1)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
						"Category %s, Carriers group %u: the BBFrame size "
						"of MODCOD %u (%u symbols) is greater than "
						"the carrier size %u. This MODCOD will not work.\n",
						this->category->getLabel().c_str(),
						vcm.getCarriersId(), fmt_id,
						size, carrier_size_sym);
			}
			else
			{
				LOG(this->log_scheduling, LEVEL_WARNING,
						"Category %s, Carriers group %u: the BBFrame size "
						"with MODCOD %u (%u symbols) is greater than the carrier "
						"size %u. This MODCOD will not work.\n",
						this->category->getLabel().c_str(),
						vcm.getCarriersId(), fmt_id,
						size, carrier_size_sym);
			}
		}
	}
}

void ForwardSchedulingS2::createProbes(CarriersGroupDama &vcm,
                                       const std::vector<CarriersGroupDama> &vcm_carriers,
                                       std::vector<std::shared_ptr<Probe<int>>> &remain_probes,
                                       std::vector<std::shared_ptr<Probe<int>>> &avail_probes,
                                       unsigned int carriers_id)
{
	auto output = Output::Get();

	unsigned int vcm_id = 0;
	std::shared_ptr<Probe<int>> remain_probe;
	std::shared_ptr<Probe<int>> avail_probe;

	std::string prefix = probe_section + "Down/Forward capacity.Carrier" + std::to_string(carriers_id) + ".";

	// For units, if there is only one MODCOD use Kbits/s else symbols
	// check if the FIFO can emit on this carriers group
	if(vcm_carriers.size() <= 1)
	{
		std::string type = "ACM";
		std::string unit = "Symbol number";
		if(vcm.getFmtIds().size() == 1)
		{
			unit = "Kbits/s";
			type = "CCM";
		}

		remain_probe = output->registerProbe<int>(prefix + type + ".Remaining", unit, true, SAMPLE_AVG);
		avail_probe = output->registerProbe<int>(prefix + type + ".Available", unit, true, SAMPLE_AVG);
	}
	else
	{
		remain_probe = output->registerProbe<int>(prefix + ".VCM" + std::to_string(vcm_id) + ".Remaining",
		                                                 "Kbits/s", true, SAMPLE_AVG);
		avail_probe = output->registerProbe<int>(prefix + ".VCM" + std::to_string(vcm_id) + ".Available",
		                                                "Kbits/s", true, SAMPLE_AVG);
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

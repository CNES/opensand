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
 * @file     ScpcScheduling.cpp
 * @brief    The SCPC scheduling functions for MAC FIFOs with DVB-S2 forward or downlink
 * @author   David PRADAS / <david.pradas@toulouse.viveris.com>
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author   Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include <opensand_output/Output.h>

#include "ScpcScheduling.h"
#include "DvbFifo.h"
#include "FifoElement.h"
#include "OpenSandModelConf.h"
#include "Except.h"


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


// TODO try to factorize with S2Scheduling
ScpcScheduling::ScpcScheduling(time_us_t scpc_timer,
                               std::shared_ptr<EncapPlugin::EncapPacketHandler> packet_handler,
                               std::shared_ptr<fifos_t> fifos,
                               std::shared_ptr<const StFmtSimuList> simu_sts,
                               const FmtDefinitionTable &scpc_modcod_def,
                               std::shared_ptr<TerminalCategoryDama> category,
                               tal_id_t gw_id):
	Scheduling(packet_handler, fifos, simu_sts),
	scpc_timer(scpc_timer),
	incomplete_bb_frames(),
	incomplete_bb_frames_ordered(),
	pending_bbframes(),
	scpc_modcod_def(scpc_modcod_def),
	category(category),
	gw_id(gw_id)
{
	auto output = Output::Get();

	// generate probes prefix
	bool is_sat = OpenSandModelConf::Get()->getComponentType() == Component::satellite;
	std::string prefix = generateProbePrefix(gw_id, Component::terminal, is_sat);

	this->probe_scpc_total_capacity =
	    output->registerProbe<int>(prefix + "SCPC capacity.Total.Available",
	                               "Symbols per frame", true, SAMPLE_LAST);

	this->probe_scpc_total_remaining_capacity =
	    output->registerProbe<int>(prefix + "SCPC capacity.Total.Remaining",
	                               "Symbols per frame", true, SAMPLE_LAST);

	this->probe_scpc_bbframe_nbr =
	    output->registerProbe<int>(prefix + "Up_return BBFrame number SCPC.BBFrame number",
	                               "BBFrame number", true, SAMPLE_AVG);

	this->probe_sent_modcod =
	    output->registerProbe<int>(prefix + "Up_Return_modcod.Sent_modcod(SCPC)",
	                               "modcod index", true, SAMPLE_LAST);

	for(auto&& carriers : this->category->getCarriersGroups())
	{
		std::vector<std::shared_ptr<Probe<int>>> remain_probes;
		std::vector<std::shared_ptr<Probe<int>>> avail_probes;
		unsigned int carriers_id = carriers.getCarriersId();

		vol_sym_t carrier_size_sym = carriers.getTotalCapacity() /
		                             carriers.getCarriersNumber();

		for(auto&& fmt_id : carriers.getFmtIds())
		{
			vol_sym_t size;
			// check that the BBFrame maximum size is smaller than the carrier size
			if(!this->getBBFrameSizeSym(this->getBBFrameSizeBytes(fmt_id),
			                            fmt_id,
			                            0,
			                            size))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
					"Cannot determine the maximum BBFrame size\n");
				continue;
			}

			if (size > carrier_size_sym) {
				// send a warning message, this will work but this is not
				// a good configuration
				// if there is more than one carrier, this won't really
				// be a problem but this won't be representative
				LOG(this->log_scheduling, LEVEL_WARNING,
				    "Category %s, Carriers group %u : the BBFrame size "
				    "with MODCOD %u (%u symbols) is greater than the carrier "
				    "size %u. This MODCOD will not work.\n",
				    this->category->getLabel().c_str(),
				    carriers.getCarriersId(), fmt_id,
				    size, carrier_size_sym);
			}
		}

		// For units, if there is only one MODCOD use Kbits/s else symbols
		// check if the FIFO can emit on this carriers group
		std::string unit = "Symbol number";
		std::string path = prefix + "SCPC capacity.Category " + this->category->getLabel() +
		                   ".Carrier" + std::to_string(carriers_id) + ".SCPC.";

		auto remain_probe = output->registerProbe<int>(path + "Remaining", unit, true, SAMPLE_AVG);
		auto avail_probe = output->registerProbe<int>(path + "Available", unit, true, SAMPLE_AVG);

		avail_probes.push_back(avail_probe);
		remain_probes.push_back(remain_probe);

		this->probe_scpc_available_capacity.emplace(carriers_id, avail_probes);
		this->probe_scpc_remaining_capacity.emplace(carriers_id, avail_probes);
	}
}

ScpcScheduling::~ScpcScheduling()
{
	this->incomplete_bb_frames.clear();
	this->pending_bbframes.clear();
}


bool ScpcScheduling::schedule(const time_sf_t current_superframe_sf,
                              std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                              uint32_t &remaining_allocation)
{
	vol_sym_t init_capacity_sym;
	int total_capa = 0;

	auto &carriers_group = this->category->getCarriersGroups();
	for(auto&& carriers : carriers_group)
	{
		unsigned int capacity_sym = 0;

		// initialize carriers capacity, remaining capacity should be 0
		// as we use previous capacity to keep track of unused capacity here
		init_capacity_sym = carriers.getTotalCapacity() +
		                    carriers.getRemainingCapacity();
		carriers.setRemainingCapacity(init_capacity_sym);
		total_capa += init_capacity_sym;

		for(auto&& fifo_it : *(this->dvb_fifos))
		{
			// check if the FIFO can emit on this carriers group
			// SCPC
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: Can send data from fifo %s on carriers group "
			    "%u in category %s\n",
			    current_superframe_sf,
			    fifo_it.second->getName().c_str(), carriers.getCarriersId(),
			    this->category->getLabel().c_str());

			if(!this->scheduleEncapPackets(*(fifo_it.second),
			                               current_superframe_sf,
			                               complete_dvb_frames,
			                               carriers))
			{
				return false;
			}
		}

		// try to fill the BBFrames list with the remaining
		// incomplete BBFrames
		capacity_sym = carriers.getRemainingCapacity();
		for(auto it = this->incomplete_bb_frames_ordered.begin();
		    it != this->incomplete_bb_frames_ordered.end();
		    it = this->incomplete_bb_frames_ordered.erase(it))
		{
			if(capacity_sym <= 0)
			{
				break;
			}

			auto& current_bbframe = this->incomplete_bb_frames.at(*it);
			sched_status ret = this->addCompleteBBFrame(complete_dvb_frames,
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
				// we keep the remaining capacity that won't be used for
				// next frame
				carriers.setPreviousCapacity(std::min(capacity_sym, init_capacity_sym), next_sf);
				break;
			}
		}
		// update remaining capacity for statistics
		carriers.setRemainingCapacity(std::min(capacity_sym, init_capacity_sym));
	}
	this->probe_scpc_total_capacity->put(total_capa);
	this->probe_scpc_bbframe_nbr->put(complete_dvb_frames.size());

	for(auto&& carriers : carriers_group)
	{
		unsigned int carriers_id = carriers.getCarriersId();
		unsigned int id = 0;

		unsigned int remain = carriers.getRemainingCapacity();
		unsigned int avail = carriers.getTotalCapacity();
		// keep total remaining capacity (for stats)
		remaining_allocation += remain;

		// get remain in Kbits/s instead of symbols if possible
		if(carriers.getFmtIds().size() == 1)
		{
			remain = this->scpc_modcod_def.symToKbits(carriers.getFmtIds().front(), remain);
			avail = this->scpc_modcod_def.symToKbits(carriers.getFmtIds().front(), avail);
			// we get kbits per frame, convert in kbits/s
			remain = std::chrono::seconds{remain} / this->scpc_timer;
			avail = std::chrono::seconds{avail} / this->scpc_timer;
		}

		this->probe_scpc_available_capacity[carriers_id][id]->put(avail);
		this->probe_scpc_remaining_capacity[carriers_id][id]->put(remain);
		id++;
		// reset remaining capacity
		carriers.setRemainingCapacity(0);
	}
	this->probe_scpc_total_remaining_capacity->put(remaining_allocation);

	return true;
}


sched_status ScpcScheduling::schedulePacket(const time_sf_t current_superframe_sf,
                                            unsigned int &sent_packets,
                                            vol_sym_t &capacity_sym,
                                            CarriersGroupDama &carriers,
                                            std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                            Rt::Ptr<NetPacket> encap_packet)
{
	while (encap_packet)
	{
		std::map<unsigned int, Rt::Ptr<BBFrame>>::iterator current_bbframe_it;
		if(!this->prepareIncompleteBBFrame(carriers, current_superframe_sf, current_bbframe_it))
		{
			// cannot initialize incomplete BB Frame
			return sched_status::error;
		}
		else if(current_bbframe_it == this->incomplete_bb_frames.end())
		{
			// cannot get modcod for the ST delete the element and continue
			return sched_status::ok;
		}

		fmt_id_t modcod = current_bbframe_it->first;
		Rt::Ptr<BBFrame>& current_bbframe = current_bbframe_it->second;

		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "SF#%u: Got the BBFrame for packet #%u, "
		    "there is now %zu complete BBFrames and %zu "
		    "incomplete\n", current_superframe_sf,
		    sent_packets + 1, complete_dvb_frames.size(),
		    this->incomplete_bb_frames.size());

		// Encapsulate packet
		auto encap_packet_total_length = encap_packet->getTotalLength();
		Rt::Ptr<NetPacket> data = Rt::make_ptr<NetPacket>(nullptr);
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
		if(!data && !partial_encap)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: bad getChunk function "
			    "implementation, assert or skip packet #%u\n",
			    current_superframe_sf,
			    sent_packets + 1);
			throw BadPrecondition("getChunk function returned neither data nor partial"
			                      "_data in ScpcScheduling::scheduleEncapPackets");
		}
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
					// we keep the remaining capacity that won't be used for next frame
					carriers.setPreviousCapacity(capacity_sym, next_sf);
					capacity_sym = 0;
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


bool ScpcScheduling::scheduleEncapPackets(DvbFifo &fifo,
                                          const time_sf_t current_superframe_sf,
                                          std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                          CarriersGroupDama &carriers)
{
	unsigned int sent_packets = 0;
	long max_to_send;
	std::list<fmt_id_t> supported_modcods = carriers.getFmtIds();
	vol_sym_t capacity_sym = carriers.getRemainingCapacity();
	vol_sym_t previous_sym = carriers.getPreviousCapacity(current_superframe_sf);
	vol_sym_t init_capa = capacity_sym;
	capacity_sym += previous_sym;

	// retrieve the number of packets waiting for retransmission
	max_to_send = fifo.getCurrentSize();
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: ScpcScheduling::scheduleEncapPackets: "
	    "max_to_send is %u, this->pending_bbframes.size() = %u\n",
	    current_superframe_sf,
	    max_to_send, this->pending_bbframes.size());
	if (max_to_send <= 0 && this->pending_bbframes.size() == 0)
	{
		// reset previous capacity
		carriers.setPreviousCapacity(0, 0);
		// set the remaining capacity for incomplete frames scheduling
		carriers.setRemainingCapacity(capacity_sym);
		return true;
	}

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: Scheduling FIFO %s, carriers group %u, "
	    "capacity is %u symbols (+ %u previous)\n",
	    current_superframe_sf,
	    fifo.getName().c_str(), carriers.getCarriersId(),
	    capacity_sym, previous_sym);

	// first add the pending complete BBFrame in the complete BBframes list
	// we add previous remaining capacity here because if a BBFrame was
	// not send before, previous_capacity contains the remaining capacity at the
	// end of the previous frame
	this->schedulePending(supported_modcods, current_superframe_sf,
	                      complete_dvb_frames, capacity_sym);
	// reset previous capacity
	carriers.setPreviousCapacity(0, 0);

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
		carriers.setRemainingCapacity(capacity_sym);
		return true;
	}

	// there are really packets to send
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: send at most %ld encapsulation packets "
	    "for %s fifo\n", current_superframe_sf,
	    max_to_send, fifo.getName());

	// now build BB frames with packets extracted from the MAC FIFO
	if (sched_status::error == schedulePacket(current_superframe_sf,
                                              sent_packets,
                                              capacity_sym,
                                              carriers,
                                              complete_dvb_frames,
                                              std::move(this->remaining_data)))
	{
		return false;
	}

	for (auto &&elem : fifo)
	{
		// retrieve the encapsulation packet
		Rt::Ptr<NetPacket> encap_packet = elem->releaseElem<NetPacket>();
		if (!encap_packet)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: invalid packet #%u in MAC FIFO "
			    "element\n", current_superframe_sf,
			    sent_packets + 1);
		}
		else
		{
			auto ret = schedulePacket(current_superframe_sf,
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
	// update remaining capacity for incomplete frames scheduling
	carriers.setRemainingCapacity(capacity_sym);

	return true;
}


bool ScpcScheduling::createIncompleteBBFrame(Rt::Ptr<BBFrame> &bbframe,
                                             const time_sf_t current_superframe_sf,
                                             fmt_id_t modcod_id)
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
	this->probe_sent_modcod->put(modcod_id);

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

bool ScpcScheduling::getBBFrameSizeSym(size_t bbframe_size_bytes,
                                       fmt_id_t modcod_id,
                                       const time_sf_t current_superframe_sf,
                                       vol_sym_t &bbframe_size_sym)
{
	float spectral_efficiency;

	if(!this->scpc_modcod_def.doFmtIdExist(modcod_id))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: failed to found the definition of MODCOD ID %u\n",
		    current_superframe_sf, modcod_id);
		goto error;
	}
	spectral_efficiency = this->scpc_modcod_def.getSpectralEfficiency(modcod_id);

	// duration is calculated over the complete BBFrame size, the BBFrame data
	// size represents the payload without coding
	bbframe_size_sym = (bbframe_size_bytes * 8) / spectral_efficiency;

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "size of the BBFRAME = %u symbols\n", bbframe_size_sym);

	return true;

error:
	return false;
}

unsigned int ScpcScheduling::getBBFrameSizeBytes(fmt_id_t modcod_id)
{
	// get the payload size
	try
	{
		FmtDefinition &fmt_def = this->scpc_modcod_def.getDefinition(modcod_id);
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


bool ScpcScheduling::prepareIncompleteBBFrame(CarriersGroupDama &carriers,
                                              const time_sf_t current_superframe_sf,
                                              std::map<unsigned int, Rt::Ptr<BBFrame>>::iterator &it)
{
	auto desired_modcod = this->getCurrentModcodId(this->gw_id);
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "Simulated MODCOD for GW = %u\n", desired_modcod);

	// get best modcod ID according to carrier
	auto modcod_id = carriers.getNearestFmtId(desired_modcod);
	if(modcod_id == 0)
	{
		LOG(this->log_scheduling, LEVEL_WARNING,
		    "SF#%u: cannot serve Gateway with any modcod (desired %u) "
		    "on carrier %u\n", current_superframe_sf, desired_modcod,
		    carriers.getCarriersId());

		modcod_id = scpc_modcod_def.getMinId();
	}

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: Available MODCOD for GW = %u\n",
	    current_superframe_sf, modcod_id);

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


sched_status ScpcScheduling::addCompleteBBFrame(std::list<Rt::Ptr<DvbFrame>> &complete_bb_frames,
                                                Rt::Ptr<BBFrame> &bbframe,
                                                const time_sf_t current_superframe_sf,
                                                vol_sym_t &remaining_capacity_sym)
{
	fmt_id_t modcod_id = bbframe->getModcodId();
	size_t bbframe_size_bytes = bbframe->getMaxSize();
	vol_sym_t bbframe_size_sym;

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


void ScpcScheduling::schedulePending(const std::list<fmt_id_t> supported_modcods,
                                     const time_sf_t current_superframe_sf,
                                     std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                     vol_sym_t &remaining_capacity_sym)
{
	if(this->pending_bbframes.size() == 0)
	{
		return;
	}

	std::list<Rt::Ptr<BBFrame>> new_pending;
	for (auto&& pending_bbframe : this->pending_bbframes)
	{
		fmt_id_t modcod = pending_bbframe->getModcodId();

		if(std::find(supported_modcods.begin(), supported_modcods.end(), modcod) !=
		   supported_modcods.end())
		{
			if(this->addCompleteBBFrame(complete_dvb_frames,
			                            pending_bbframe,
			                            current_superframe_sf,
			                            remaining_capacity_sym) != sched_status::ok)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: cannot add pending BBFrame in the list "
				    "of complete BBFrames\n", current_superframe_sf);
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "this errors may mean that you don't have enough "
				    "band to send BBFrames, please change your configuration\n");
			}
		}
		else
		{
			// keep the BBFrame in pending list
			new_pending.push_back(std::move(pending_bbframe));
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

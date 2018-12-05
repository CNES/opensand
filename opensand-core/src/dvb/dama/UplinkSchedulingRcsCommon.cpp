/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file     UplinkSchedulingRcsCommon.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS/RCS2 uplink on GW
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author   Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 */


#include "UplinkSchedulingRcsCommon.h"

#include "MacFifoElement.h"
#include "OpenSandFrames.h"
#include "FmtDefinitionTable.h"

#include <opensand_output/Output.h>

#include <map>

using std::map;

UplinkSchedulingRcsCommon::UplinkSchedulingRcsCommon(
			time_ms_t frame_duration_ms,
			EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos,
			const StFmtSimuList *const ret_sts,
			const FmtDefinitionTable *const ret_modcod_def,
			const TerminalCategoryDama *const category,
			tal_id_t gw_id):
	Scheduling(packet_handler, fifos, ret_sts),
	frame_duration_ms(frame_duration_ms),
	gw_id(gw_id),
	lowest_modcod(0),
	ret_modcod_def(ret_modcod_def),
	category(category),
	converter(NULL)
{
	vector<CarriersGroupDama *> carriers;
	vector<CarriersGroupDama *>::iterator carrier_it;
	carriers = this->category->getCarriersGroups();

	this->lowest_modcod = this->ret_modcod_def->getMaxId();
	for(carrier_it = carriers.begin();
		carrier_it != carriers.end();
		++carrier_it)
	{
		if((*carrier_it)->getFmtIds().front() < this->lowest_modcod)
		{
			this->lowest_modcod = (*carrier_it)->getFmtIds().front();
		}
	}
}

UplinkSchedulingRcsCommon::~UplinkSchedulingRcsCommon()
{
	delete this->converter;	
}

bool UplinkSchedulingRcsCommon::init()
{
	// Initialize unit converter
	this->converter = this->generateUnitConverter();
	if(!this->converter)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
			"Unit converter initialization failed\n");
		return false;
	}
	
	// configure unit converter
	this->converter->setModulationEfficiency(
		this->ret_modcod_def->getModulationEfficiency(this->lowest_modcod));
	return true;
}

bool UplinkSchedulingRcsCommon::schedule(const time_sf_t current_superframe_sf,
                                   clock_t current_time,
                                   list<DvbFrame *> *complete_dvb_frames,
                                   uint32_t &UNUSED(remaining_allocation))
{
	fifos_t::const_iterator fifo_it;
	vector<CarriersGroupDama *> carriers;
	vector<CarriersGroupDama *>::iterator carrier_it;
	uint8_t desired_modcod;
	CarriersGroupDama *carrier;
	unsigned int carrier_id;
	uint8_t modcod_id;
	map<unsigned int, uint8_t> carriers_modcod;

	carriers = this->category->getCarriersGroups();
	desired_modcod = this->getCurrentModcodId(this->gw_id);

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "Simulated MODCOD for GW%u = %u\n", this->gw_id, desired_modcod);

	// FIXME we consider the band is not the same for GW and terminals (this
	//       is a good consideration...) but as we have only one band configuration
	//       we use the same parameters
	// FIXME we use the first available carriers with a good MODCOD, if the first MODCOD
	//       found is 1, we will always use one till be do not need to sent on more
	//       than one carrier
	// initialize carriers capacity
	for(carrier_it = carriers.begin();
	    carrier_it != carriers.end();
	    ++carrier_it)
	{
		vol_kb_t remaining_capacity_kb;
		rate_pktpf_t remaining_capacity_pktpf;
		carrier = *carrier_it;
		carrier_id = carrier->getCarriersId();

		// get best modcod ID according to carrier
		modcod_id = carrier->getNearestFmtId(desired_modcod);
		if(modcod_id == 0)
		{
			LOG(this->log_scheduling, LEVEL_NOTICE,
			    "cannot use any modcod (desired %u) "
			    "to send on on carrier %u\n", desired_modcod,
			    carrier_id);

			// do not skip if this is a carriers group with the lowest MODCOD
			if(this->lowest_modcod != carrier->getFmtIds().front())
			{
				// no available allocation on this carrier
				carrier->setRemainingCapacity(0);
				carriers_modcod[carrier_id] = modcod_id;
				continue;
			}
			modcod_id = this->lowest_modcod;
			LOG(this->log_scheduling, LEVEL_NOTICE,
			    "No carrier found to use modcod %u, "
			    "send data with lowest available MODCOD %u\n",
			    desired_modcod, this->lowest_modcod);
		}
		carriers_modcod[carrier_id] = modcod_id;
		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "Available MODCOD for GW = %u\n", modcod_id);

		this->converter->setModulationEfficiency(
			this->ret_modcod_def->getModulationEfficiency(modcod_id));

		remaining_capacity_kb =
			this->ret_modcod_def->symToKbits(modcod_id,
			                      carrier->getTotalCapacity());

		// as this function is called each superframe we can directly
		// convert number of packet to rate in packet per superframe
		// and dividing by the frame number per superframes we have
		// the rate in packet per frame
		remaining_capacity_pktpf = this->converter->kbitsToPkt(remaining_capacity_kb);

		// initialize remaining capacity with total capacity in
		// packet per superframe as it is the unit used in DAMA computations
		carrier->setRemainingCapacity(remaining_capacity_pktpf);
		LOG(this->log_scheduling, LEVEL_INFO,
		    "SF#%u: capacity before scheduling on GW uplink %u: "
		    "%u packet (per frame) (%u kb)",
		    current_superframe_sf,
		    carrier_id,
		    remaining_capacity_pktpf,
		    remaining_capacity_kb);
	}

	for(fifo_it = this->dvb_fifos.begin();
	    fifo_it != this->dvb_fifos.end(); ++fifo_it)
	{
		for(carrier_it = carriers.begin();
		    carrier_it != carriers.end();
		    ++carrier_it)
		{
			carrier = *carrier_it;
			carrier_id = carrier->getCarriersId();
			modcod_id = carriers_modcod[carrier_id];
			this->converter->setModulationEfficiency(
				this->ret_modcod_def->getModulationEfficiency(modcod_id));

			if(!this->scheduleEncapPackets((*fifo_it).second,
			                               current_superframe_sf,
			                               current_time,
			                               complete_dvb_frames,
			                               carrier,
			                               modcod_id))
			{
				return false;
			}
		}
	}
	return true;
}

bool UplinkSchedulingRcsCommon::createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame,
                                                      fmt_id_t modcod_id)
{
	vol_bytes_t length_bytes;

	if(!this->packet_handler)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "packet handler is NULL\n");
		goto error;
	}

	*incomplete_dvb_frame = new DvbRcsFrame();
	if(*incomplete_dvb_frame == NULL)
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS frame\n");
		goto error;
	}

	// Get the max burst length
	length_bytes = this->converter->getPacketBitLength() >> 3;
	if(length_bytes <= 0)
	{
		delete (*incomplete_dvb_frame);
		*incomplete_dvb_frame = NULL;
		LOG(this->log_scheduling, LEVEL_ERROR,
			"failed to create DVB-RCS/RCS2 frame: invalid burst length\n");
		goto error;
	}

	// Add header length
	length_bytes += (*incomplete_dvb_frame)->getHeaderLength();
	//length_bytes += sizeof(T_DVB_PHY);
	if(MSG_DVB_RCS_SIZE_MAX < length_bytes)
	{
		length_bytes = MSG_DVB_RCS_SIZE_MAX;
	}

	// set the max size of the DVB-RCS2 frame, also set the type
	// of encapsulation packets the DVB-RCS2 frame will contain
	(*incomplete_dvb_frame)->setMaxSize(length_bytes);

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "new DVB-RCS/RCS2 frame with max length %u bytes (<= %u bytes), "
	    "payload length %u bytes, header length %u bytes\n",
	    (*incomplete_dvb_frame)->getMaxSize(),
	    MSG_DVB_RCS_SIZE_MAX,
	    (*incomplete_dvb_frame)->getFreeSpace(),
	    (*incomplete_dvb_frame)->getHeaderLength());

	// set the type of encapsulation packets the DVB-RCS frame will
	// contain we do not need to handle MODCOD here because the size
	// to send is managed by the allocation, the DVB frame is only an
	// abstract object to transport data
	(*incomplete_dvb_frame)->setModcodId(modcod_id);

	return true;

error:
	return false;
}




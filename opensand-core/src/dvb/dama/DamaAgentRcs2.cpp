/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file    DamaAgentRcs2.cpp
 * @brief   Implementation of the DAMA agent for DVB-RCS2 emission standard.
 * @author  Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "DamaAgentRcs2.h"

#include <opensand_output/Output.h>


DamaAgentRcs2::DamaAgentRcs2(FmtDefinitionTable *ret_modcod_def):
	DamaAgentRcsCommon(),
	ret_modcod_def(ret_modcod_def)
{
}

DamaAgentRcs2::~DamaAgentRcs2()
{
}

ReturnSchedulingRcsCommon *DamaAgentRcs2::generateReturnScheduling() const
{
	return new ReturnSchedulingRcs2(this->packet_handler, this->dvb_fifos);
}

// a TTP reading function that handles MODCOD but not priority and frame id
// only one TP is supported for MODCOD handling
bool DamaAgentRcs2::hereIsTTP(Ttp *ttp)
{
	fmt_id_t prev_modcod_id;
	map<uint8_t, emu_tp_t> tp;

	if(this->group_id != ttp->getGroupId())
	{
		LOG(this->log_ttp, LEVEL_ERROR,
		    "SF#%u: TTP with different group_id (%d).\n",
		    this->current_superframe_sf, ttp->getGroupId());
		return true;
	}

	if(!ttp->getTp(this->tal_id, tp))
	{
		// Update stats and probes
		this->probe_st_total_allocation->put(0);
		return true;
	}
	if(tp.size() > 1)
	{
		LOG(this->log_ttp, LEVEL_WARNING,
		    "Received more than one TP in TTP, "
		    "allocation will be correctly handled but not "
		    "modcod for physical layer emulation\n");
	}

	prev_modcod_id = this->modcod_id;
	for(map<uint8_t, emu_tp_t>::iterator it = tp.begin();
	    it != tp.end(); ++it)
	{
		vol_kb_t assign_kb;
		time_pkt_t assign_pkt;

		assign_kb = (*it).second.assignment_count;
		assign_pkt = this->converter->kbitsToPkt(assign_kb);
		this->allocated_pkt += assign_pkt;
		// we can directly assign here because we should have
		// received only one TTP
		this->modcod_id = (*it).second.fmt_id;
		LOG(this->log_ttp, LEVEL_DEBUG,
		    "SF#%u: frame#%u: offset:%u, assignment_count:%u, "
		    "fmt_id:%u priority:%u\n", ttp->getSuperframeCount(),
		    (*it).first, (*it).second.offset, assign_pkt,
		    (*it).second.fmt_id, (*it).second.priority);
	}

	if(prev_modcod_id != this->modcod_id)
	{
		vol_b_t payload_length_b = 0;
		FmtDefinition *fmt_def = this->ret_modcod_def->getDefinition(this->modcod_id);

		if(fmt_def != NULL)
		{
			payload_length_b = fmt_def->getBurstLength()
				* fmt_def->getModulationEfficiency()
				* fmt_def->getCodingRate();
		}

		// update the packet length in function of MODCOD
		this->converter->updatePacketLength(payload_length_b);
		LOG(this->log_ttp, LEVEL_DEBUG,
		    "SF#%u: modcod changed to %u, packet length: %u kbits\n",
		    ttp->getSuperframeCount(), this->modcod_id,
		    payload_length_b / 1000);
	}

	// Update stats and probes
	this->probe_st_total_allocation->put(
		this->converter->pktpfToKbps(this->allocated_pkt));

	LOG(this->log_ttp, LEVEL_INFO,
	    "SF#%u: allocated TS=%u\n",
	    ttp->getSuperframeCount(), this->allocated_pkt);
	return true;
}

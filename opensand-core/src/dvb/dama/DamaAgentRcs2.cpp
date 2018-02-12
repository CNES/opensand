/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
#include "UnitConverterFixedSymbolLength.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>


DamaAgentRcs2::DamaAgentRcs2(FmtDefinitionTable *ret_modcod_def):
	DamaAgentRcsCommon(ret_modcod_def)
{
}

DamaAgentRcs2::~DamaAgentRcs2()
{
}

bool DamaAgentRcs2::processOnFrameTick()
{
	FmtDefinition *fmt_def;
	vol_b_t length_b;

	if(!DamaAgentRcsCommon::processOnFrameTick())
	{
		return false;
	}

	fmt_def = this->ret_modcod_def->getDefinition(this->modcod_id);
	if(fmt_def == NULL)
	{
		LOG(this->log_schedule, LEVEL_WARNING,
		    "SF#%u: no MODCOD %u found",
		    this->current_superframe_sf,
		    this->modcod_id);
		return false;
	}

	length_b = this->burst_length_b;
	this->burst_length_b = fmt_def->removeFec(this->burst_length_b);
	LOG(this->log_schedule, LEVEL_DEBUG,
	    "SF#%u: burst length without FEC %u b, with FEC %u b",
	    this->current_superframe_sf,
	    this->burst_length_b,
	    length_b);
	return true;
}

UnitConverter *DamaAgentRcs2::generateUnitConverter() const
{
	vol_sym_t length_sym = 0;
	
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   RCS2_BURST_LENGTH, length_sym))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_BUFFER);
		return NULL;
	}
	if(length_sym == 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "invalid value '%u' value of '%s", length_sym, DELAY_BUFFER);
		return NULL;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "Burst length = %u sym\n", length_sym);
	
	return new UnitConverterFixedSymbolLength(this->frame_duration_ms, 
		0, length_sym);
}


ReturnSchedulingRcsCommon *DamaAgentRcs2::generateReturnScheduling() const
{
	return new ReturnSchedulingRcs2(this->packet_handler, this->dvb_fifos);
}


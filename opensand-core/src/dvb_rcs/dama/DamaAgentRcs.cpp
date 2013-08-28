/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file    DamaAgentRcs.cpp
 * @brief   Implementation of the DAMA agent for DVB-RCS emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#define DBG_PACKAGE PKG_DAMA_DA
#include <opensand_conf/uti_debug.h>
#define DA_DBG_PREFIX "[RCS]"

#include "DamaAgentRcs.h"


DamaAgentRcs::DamaAgentRcs():
	DamaAgent(),
	allocated_pkt(0),
	dynamic_allocation_pkt(0),
	remaining_allocation_pktpf(0)
{
}


bool DamaAgentRcs::processOnFrameTick()
{
	// Call parent method
	if(!DamaAgent::processOnFrameTick())
	{
		UTI_ERROR("SF#%u: cannot call DamaAgent::processOnFrameTick()\n",
		          this->current_superframe_sf);
		return false;
	}

	this->current_frame++;
	this->remaining_allocation_pktpf = this->dynamic_allocation_pkt;

	return true;
}

bool DamaAgentRcs::hereIsSOF(time_sf_t superframe_number_sf)
{
	// Call parent method
	if(!DamaAgent::hereIsSOF(superframe_number_sf))
	{
		UTI_ERROR("SF#%u: cannot call DamaAgent::hereIsSOF()\n",
		          this->current_superframe_sf);
		return false;
	}
	this->current_frame = 0;
	return true;
}

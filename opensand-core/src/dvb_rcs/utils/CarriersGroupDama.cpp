/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file    CarriersGroupDama.cpp
 * @brief   Represent a group of carriers with the same characteristics
 *          for DAMA
 * @author  Audric Schiltknecht / Viveris Technologies
 */


#include "CarriersGroupDama.h"

CarriersGroupDama::CarriersGroupDama(unsigned int carriers_id,
                                     const FmtGroup *const fmt_group,
                                     unsigned int ratio,
                                     rate_symps_t symbol_rate_symps,
                                     access_type_t access_type):
	CarriersGroup(carriers_id, fmt_group, ratio, symbol_rate_symps, access_type),
	remaining_capacity(0),
	previous_capacity(0),
	previous_sf(0)
{
}

CarriersGroupDama::~CarriersGroupDama()
{
}

void CarriersGroupDama::setRemainingCapacity(const unsigned int remaining_capacity)
{
	this->remaining_capacity = remaining_capacity;
}

unsigned int CarriersGroupDama::getRemainingCapacity() const
{
	return this->remaining_capacity;
}

void CarriersGroupDama::setPreviousCapacity(const unsigned int previous_capacity,
                                            const time_sf_t superframe_sf)
{
	this->previous_capacity = previous_capacity;
	this->previous_sf = superframe_sf;
}

unsigned int CarriersGroupDama::getPreviousCapacity(const time_sf_t superframe_sf) const
{
	if(this->previous_sf != superframe_sf)
	{
		return 0;
	}
	return this->previous_capacity;
}

unsigned int CarriersGroupDama::getNearestFmtId(unsigned int fmt_id)
{
	return this->fmt_group->getNearest(fmt_id);
}



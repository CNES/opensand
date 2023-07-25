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


#include <cmath>

#include "CarriersGroupDama.h"


CarriersGroupDama::CarriersGroupDama(unsigned int carriers_id,
                                     std::shared_ptr<const FmtGroup> fmt_group,
                                     unsigned int ratio,
                                     rate_symps_t symbol_rate_symps,
                                     AccessType access_type):
	CarriersGroup(carriers_id, fmt_group, ratio, symbol_rate_symps, access_type),
	remaining_capacity(0),
	previous_capacity(0),
	previous_sf(0),
	vcm_carriers()
{
}


void CarriersGroupDama::setCapacity(const vol_sym_t capacity_sym)
{
	CarriersGroup::setCapacity(capacity_sym);

	// compute VCM capacities	
	for (auto &&vcm: this->vcm_carriers)
	{
		vcm.setCapacity(floor(vcm.getRatio() * capacity_sym / this->ratio));
	}
}

void CarriersGroupDama::setCarriersNumber(const unsigned int carriers_number)
{
	CarriersGroup::setCarriersNumber(carriers_number);

	// compute VCM capacities	
	for (auto &&vcm: this->vcm_carriers)
	{
		vcm.setCarriersNumber(carriers_number);
	}
}

void CarriersGroupDama::setSymbolRate(const double symbol_rate_symps)
{
	CarriersGroup::setSymbolRate(symbol_rate_symps);

	// compute VCM capacities	
	for (auto &&vcm: this->vcm_carriers)
	{
		vcm.setSymbolRate(symbol_rate_symps);
	}
}

void CarriersGroupDama::addVcm(std::shared_ptr<const FmtGroup> fmt_group,
                               unsigned int ratio)
{
	if(this->vcm_carriers.size() > 0)
	{
		// if this carriers group is already in the vcm carriers list
		// add the new ratio to the total ratio, else the ratio
		// is already taken into account
		this->ratio += ratio;
	}

	this->vcm_carriers.emplace_back(this->carriers_id,
	                                fmt_group,
	                                ratio,
	                                this->symbol_rate_symps,
	                                this->access_type);
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

std::vector<CarriersGroupDama> &CarriersGroupDama::getVcmCarriers()
{
	return this->vcm_carriers;
}

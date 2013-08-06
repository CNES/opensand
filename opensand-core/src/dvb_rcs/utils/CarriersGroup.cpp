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
 * @file    CarriersGroup.cpp
 * @brief   Represent a group of carriers with the same characteristics
 * @author  Audric Schiltknecht / Viveris Technologies
 */


#include "CarriersGroup.h"

CarriersGroup::CarriersGroup(unsigned int carriers_id,
                             const FmtGroup *const fmt_group,
                             unsigned int ratio,
                             rate_symps_t symbol_rate_symps):
	carriers_id(carriers_id),
	fmt_group(fmt_group),
	carriers_number(0),
	ratio(ratio),
	capacity_sym(0),
	remaining_capacity(0),
	symbol_rate_symps(symbol_rate_symps)
{
}

CarriersGroup::~CarriersGroup()
{
}


unsigned int CarriersGroup::getCarriersId() const
{
	return this->carriers_id;
}

void CarriersGroup::setCarriersNumber(const unsigned int carriers_number)
{
	this->carriers_number = carriers_number;
}

void CarriersGroup::setCapacity(const vol_sym_t capacity_sym)
{
	this->capacity_sym = capacity_sym;
}

vol_sym_t CarriersGroup::getTotalCapacity() const
{
	return (this->capacity_sym * this->carriers_number);
}

void CarriersGroup::setRemainingCapacity(const unsigned int remaining_capacity)
{
	this->remaining_capacity = remaining_capacity;
}

unsigned int CarriersGroup::getRemainingCapacity() const
{
	return this->remaining_capacity;
}

rate_symps_t CarriersGroup::getSymbolRate() const
{
	return this->symbol_rate_symps;
}

void CarriersGroup::setSymbolRate(const double symbol_rate_symps)
{
	this->symbol_rate_symps = symbol_rate_symps;
}

unsigned int CarriersGroup::getCarriersNumber() const
{
	return this->carriers_number;
}

unsigned int CarriersGroup::getRatio() const
{
	return this->ratio;
}

unsigned int CarriersGroup::getNearestFmtId(unsigned int fmt_id)
{
	return this->fmt_group->getNearest(fmt_id);
}

const list<unsigned int> CarriersGroup::getFmtIds() const
{
	return this->fmt_group->getFmtIds();
}

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file UnitConverter.cpp
 * @brief Converters for OpenSAND units
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "UnitConverter.h"

#include <math.h>

UnitConverter::UnitConverter(time_ms_t duration_ms, unsigned int efficiency)
{
	this->setFrameDuration(duration_ms);
	this->setModulationEfficiency(efficiency);
}

UnitConverter::~UnitConverter()
{
}

void UnitConverter::setFrameDuration(time_ms_t duration_ms)
{
	this->frame_duration_ms = duration_ms;
	if(0 < this->frame_duration_ms)
	{
		this->frame_duration_ms_inv = 1.0 / this->frame_duration_ms;
	}
	else
	{
		this->frame_duration_ms_inv = 0.0;
	}
}

time_ms_t UnitConverter::getFrameDuration() const
{
	return this->frame_duration_ms;
}

void UnitConverter::setModulationEfficiency(unsigned int efficiency)
{
	this->modulation_efficiency = efficiency;
	if(0 < this->modulation_efficiency)
	{
		this->modulation_efficiency_inv = 1.0 / this->modulation_efficiency;
	}
	else
	{
		this->modulation_efficiency_inv = 0.0;
	}
}

unsigned int UnitConverter::getModulationEfficiency() const
{
	return this->modulation_efficiency;
}

vol_sym_t UnitConverter::bitsToSym(vol_b_t vol_b) const
{
	return ceil(vol_b * this->modulation_efficiency_inv);
}

vol_b_t UnitConverter::symToBits(vol_sym_t vol_sym) const
{
	return vol_sym * this->modulation_efficiency;
}

vol_sym_t UnitConverter::kbitsToSym(vol_kb_t vol_kb) const
{
	return ceil(vol_kb * 1000 * this->modulation_efficiency_inv);
}

vol_kb_t UnitConverter::symToKbits(vol_sym_t vol_sym) const
{
	return ceil(vol_sym * this->modulation_efficiency / 1000.0);
}

vol_kb_t UnitConverter::bitsToKbits(vol_b_t vol_b) const
{
	return ceil(vol_b / 1000.0);
}

vol_b_t UnitConverter::kbitsToBits(vol_kb_t vol_kb) const
{
	return vol_kb * 1000;
}

rate_symps_t UnitConverter::bpsToSymps(rate_bps_t rate_bps) const
{
	return ceil(rate_bps * this->modulation_efficiency_inv);
}

rate_bps_t UnitConverter::sympsToBps(rate_symps_t rate_symps) const
{
	return rate_symps * this->modulation_efficiency;
}

rate_symps_t UnitConverter::kbpsToSymps(rate_kbps_t rate_kbps) const
{
	return ceil(rate_kbps * 1000 * this->modulation_efficiency_inv);
}

rate_kbps_t UnitConverter::sympsToKbps(rate_symps_t rate_symps) const
{
	return ceil(rate_symps * this->modulation_efficiency / 1000.0);
}

rate_kbps_t UnitConverter::bpsToKbps(rate_bps_t rate_bps) const
{
	return ceil(rate_bps / 1000.0);
}

rate_bps_t UnitConverter::kbpsToBps(rate_kbps_t rate_kbps) const
{
	return rate_kbps * 1000;
}

unsigned int UnitConverter::pfToPs(unsigned int rate_pf) const
{
	return ceil(rate_pf * this->frame_duration_ms_inv * 1000);
}

unsigned int UnitConverter::psToPf(unsigned int rate_ps) const
{
	return ceil(rate_ps * this->frame_duration_ms / 1000.0);
}

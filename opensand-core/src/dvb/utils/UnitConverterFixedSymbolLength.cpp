/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file UnitConverterFixedSymbolLength.cpp
 * @brief Converters for OpenSAND units
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "UnitConverterFixedSymbolLength.h"

#include <math.h>


using time_seconds_t = std::chrono::duration<double>;
using time_milliseconds_t = std::chrono::duration<double, std::milli>;


UnitConverterFixedSymbolLength::UnitConverterFixedSymbolLength(time_us_t duration,
                                                               unsigned int efficiency,
                                                               vol_sym_t length_sym):
	UnitConverter(duration, efficiency)
{
	this->setPacketSymbolLength(length_sym);
}

UnitConverterFixedSymbolLength::~UnitConverterFixedSymbolLength()
{
}

vol_b_t UnitConverterFixedSymbolLength::getPacketBitLength() const
{
	return this->symToBits(this->packet_length_sym);
}

vol_kb_t UnitConverterFixedSymbolLength::getPacketKbitLength() const
{
	return this->symToKbits(this->packet_length_sym);
}

vol_sym_t UnitConverterFixedSymbolLength::getPacketSymbolLength() const
{
	return this->packet_length_sym;
}

void UnitConverterFixedSymbolLength::setPacketSymbolLength(vol_sym_t length_sym)
{
	this->packet_length_sym = length_sym;
	if(0 < this->packet_length_sym)
	{
		this->packet_length_sym_inv = 1.0 / this->packet_length_sym;
	}
	else
	{
		this->packet_length_sym_inv = 0.0;
	}
}

vol_pkt_t UnitConverterFixedSymbolLength::symToPkt(vol_sym_t vol_sym) const
{
	return ceil(vol_sym * this->packet_length_sym_inv);
}

vol_sym_t UnitConverterFixedSymbolLength::pktToSym(vol_pkt_t vol_pkt) const
{
	return vol_pkt * this->packet_length_sym;
}

vol_pkt_t UnitConverterFixedSymbolLength::bitsToPkt(vol_b_t vol_b) const
{
	return ceil(vol_b * this->packet_length_sym_inv * this->modulation_efficiency_inv);
}

vol_b_t UnitConverterFixedSymbolLength::pktToBits(vol_pkt_t vol_pkt) const
{
	return vol_pkt * this->packet_length_sym * this->modulation_efficiency;
}
	
vol_pkt_t UnitConverterFixedSymbolLength::kbitsToPkt(vol_kb_t vol_kb) const
{
	return ceil(vol_kb * this->packet_length_sym_inv * this->modulation_efficiency_inv * 1000);
}

vol_kb_t UnitConverterFixedSymbolLength::pktToKbits(vol_pkt_t vol_pkt) const
{
	return ceil(vol_pkt * this->packet_length_sym * this->modulation_efficiency / 1000.0);
}

rate_pktpf_t UnitConverterFixedSymbolLength::sympsToPktpf(rate_symps_t rate_symps) const
{
	return ceil(std::chrono::duration_cast<time_seconds_t>(rate_symps * this->packet_length_sym_inv * this->frame_duration).count());
}

rate_symps_t UnitConverterFixedSymbolLength::pktpfToSymps(rate_pktpf_t rate_pktpf) const
{
	return ceil(time_seconds_t{rate_pktpf * this->packet_length_sym} / this->frame_duration);
}

rate_pktpf_t UnitConverterFixedSymbolLength::bpsToPktpf(rate_bps_t rate_bps) const
{
	return ceil(std::chrono::duration_cast<time_seconds_t>(rate_bps * this->packet_length_sym_inv * this->modulation_efficiency_inv * this->frame_duration).count());
}

rate_bps_t UnitConverterFixedSymbolLength::pktpfToBps(rate_pktpf_t rate_pktpf) const
{
	return ceil(time_seconds_t{rate_pktpf * this->packet_length_sym * this->modulation_efficiency} / this->frame_duration);
}
	
rate_pktpf_t UnitConverterFixedSymbolLength::kbpsToPktpf(rate_kbps_t rate_kbps) const
{
	// bit/ms <=> kbits/s
	return ceil(std::chrono::duration_cast<time_milliseconds_t>(rate_kbps * this->packet_length_sym_inv * this->modulation_efficiency_inv * this->frame_duration).count());
}

rate_kbps_t UnitConverterFixedSymbolLength::pktpfToKbps(rate_pktpf_t rate_pktpf) const
{
	// bit/ms <=> kbits/s
	return ceil(time_milliseconds_t{rate_pktpf * this->packet_length_sym * this->modulation_efficiency} / this->frame_duration);
}

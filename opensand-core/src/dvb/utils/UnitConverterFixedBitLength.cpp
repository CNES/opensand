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
 * @file UnitConverterFixedBitLength.cpp
 * @brief Converters for OpenSAND units
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "UnitConverterFixedBitLength.h"

#include <math.h>

UnitConverterFixedBitLength::UnitConverterFixedBitLength(
		time_ms_t duration_ms,
		unsigned int efficiency,
		vol_b_t length_b):
	UnitConverter(duration_ms, efficiency)
{
	this->setPacketBitLength(length_b);
}

UnitConverterFixedBitLength::~UnitConverterFixedBitLength()
{
}

vol_b_t UnitConverterFixedBitLength::getPacketBitLength() const
{
	return this->packet_length_b;
}

vol_kb_t UnitConverterFixedBitLength::getPacketKbitLength() const
{
	return this->bitsToKbits(this->packet_length_b);
}

vol_sym_t UnitConverterFixedBitLength::getPacketSymbolLength() const
{
	return this->bitsToSym(this->packet_length_b);
}

void UnitConverterFixedBitLength::setPacketBitLength(vol_b_t length_b)
{
	this->packet_length_b = length_b;
	if(0 < this->packet_length_b)
	{
		this->packet_length_b_inv = 1.0 / this->packet_length_b;
	}
	else
	{
		this->packet_length_b_inv = 0.0;
	}
}

vol_pkt_t UnitConverterFixedBitLength::symToPkt(vol_sym_t vol_sym) const
{
	return ceil(vol_sym * this->packet_length_b_inv * this->modulation_efficiency);
}

vol_sym_t UnitConverterFixedBitLength::pktToSym(vol_pkt_t vol_pkt) const
{
	return vol_pkt * this->packet_length_b * this->modulation_efficiency_inv;
}

vol_pkt_t UnitConverterFixedBitLength::bitsToPkt(vol_b_t vol_b) const
{
	return ceil(vol_b * this->packet_length_b_inv);
}

vol_b_t UnitConverterFixedBitLength::pktToBits(vol_pkt_t vol_pkt) const
{
	return vol_pkt * this->packet_length_b;
}
	
vol_pkt_t UnitConverterFixedBitLength::kbitsToPkt(vol_kb_t vol_kb) const
{
	return ceil(vol_kb * this->packet_length_b_inv * 1000);
}

vol_kb_t UnitConverterFixedBitLength::pktToKbits(vol_pkt_t vol_pkt) const
{
	return ceil(vol_pkt * this->packet_length_b / 1000.0);
}

rate_pktpf_t UnitConverterFixedBitLength::sympsToPktpf(rate_symps_t rate_symps) const
{
	return ceil(rate_symps * this->packet_length_b_inv * this->modulation_efficiency
		* this->frame_duration_ms * 1000);
}

rate_symps_t UnitConverterFixedBitLength::pktpfToSymps(rate_pktpf_t rate_pktpf) const
{
	return ceil(rate_pktpf * this->packet_length_b * this->modulation_efficiency_inv
		* this->frame_duration_ms_inv / 1000.0);
}

rate_pktpf_t UnitConverterFixedBitLength::bpsToPktpf(rate_bps_t rate_bps) const
{
	return ceil(rate_bps * this->packet_length_b_inv * this->frame_duration_ms / 1000.0);
}

rate_bps_t UnitConverterFixedBitLength::pktpfToBps(rate_pktpf_t rate_pktpf) const
{
	return ceil(rate_pktpf * this->packet_length_b * this->frame_duration_ms_inv * 1000);
}
	
rate_pktpf_t UnitConverterFixedBitLength::kbpsToPktpf(rate_kbps_t rate_kbps) const
{
	// bit/ms <=> kbits/s
	return ceil(rate_kbps * this->packet_length_b_inv * this->frame_duration_ms);
}

rate_kbps_t UnitConverterFixedBitLength::pktpfToKbps(rate_pktpf_t rate_pktpf) const
{
	// bit/ms <=> kbits/s
	return ceil(rate_pktpf * this->packet_length_b * this->frame_duration_ms_inv);
}

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
 * @file UnitConverter.cpp
 * @brief Converters for OpenSAND units
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "UnitConverter.h"

#include <math.h>

/*
 * @param Duration is the Frame duration in ms
 * @param Size is the UL packet size in bytes
 */
UnitConverter::UnitConverter(vol_bytes_t packet_length_bytes,
                             time_ms_t frame_duration_ms):
	packet_length_b(packet_length_bytes * 8),
	frame_duration_ms(frame_duration_ms)
{
}

UnitConverter::~UnitConverter()
{
}

//FIXME: kb and kbps may have decimal part, should they be double instead of
// int ???
vol_b_t UnitConverter::pktToBits(vol_pkt_t vol_pkt) const
{
	return (vol_pkt * this->packet_length_b);
}

vol_kb_t UnitConverter::pktToKbits(vol_pkt_t vol_pkt) const
{
	return ceil((vol_pkt * this->packet_length_b / 1000));
}

vol_pkt_t UnitConverter::kbitsToPkt(vol_kb_t vol_kb) const
{
	return floor(vol_kb * 1000 / this->packet_length_b);
}

rate_pktpf_t UnitConverter::kbpsToPktpf(rate_kbps_t rate_kbps) const
{
	// bit/ms <=> kbits/s
	return ceil((rate_kbps * this->frame_duration_ms) /
	            this->packet_length_b);
}

rate_kbps_t UnitConverter::pktpfToKbps(rate_pktpf_t rate_pktpf) const
{
	// bits/ms <=> kbits/s
	return ceil((rate_pktpf * this->packet_length_b) / this->frame_duration_ms);
}



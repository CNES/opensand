/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file UnitConverter.h
 * @brief Converters for OpenSAND units
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 */

#ifndef _UNIT_CONVERTER_H_
#define _UNIT_CONVERTER_H_

#include "OpenSandCore.h"


/**
 * @class UnitConverter
 * @brief class managing unit conversion between kbits/s, cells per frame, etc
 */
class UnitConverter
{
 protected:

	vol_b_t packet_length_b;            ///< Uplink packets size (in bits)
	time_ms_t frame_duration_ms;        ///< Uplink frame duration (in ms)

 public:

	/**
	 * @brief Create the unit converter
	 *
	 * @param  packet_length_kb        The packet length in kbits, for constant length
	 * @param  superframe_duration_ms  The frame duration in ms
	 */
	UnitConverter(vol_b_t packet_length_kb,
	              time_ms_t frame_duration_ms);
	~UnitConverter();

	/**
	 * @brief convert from packet number to bits
	 *
	 * @param vol_pkt  The number of packets
	 * @return the size of data in bits
	 */
	vol_b_t pktToBits(vol_pkt_t vol_pkt) const;

	/**
	 * @brief convert from packet number to kbits
	 *
	 * @param vol_pkt  The number of packets
	 * @return the size of data in kbits
	 */
	vol_kb_t pktToKbits(vol_pkt_t vol_pkt) const;

	/**
	 * @brief convert from kbits to packets
	 *
	 * @param vol_kb The volume in kbits
	 * @return the volume in packets
	 */
	vol_pkt_t kbitsToPkt(vol_pkt_t) const;

	/**
	 * @brief convert from rate in kbits/s to a number of packets
	 *        per superframe
	 *
	 * @param rate_kbps  The bitrate
	 * @return the number of packets
	 */
	rate_pktpf_t kbpsToPktpf(rate_kbps_t rate_kbps) const;

	/**
	 * @brief convert from a number of packets per superframe to kbits/sec
	 *
	 * @param   The rate in number of packets per superframe
	 * @return the number of packets
	 */
	rate_kbps_t pktpfToKbps(rate_pktpf_t rate_pktpf) const;
};

#endif

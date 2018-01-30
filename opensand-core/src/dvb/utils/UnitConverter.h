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
 * @file UnitConverter.h
 * @brief Converters for OpenSAND units
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
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

	time_ms_t frame_duration_ms;          ///< Frame duration (in ms)
	float frame_duration_ms_inv;          ///< Inverse of frame duration (in ms-1)

	unsigned int modulation_efficiency;   ///< Modulation efficiency
	float modulation_efficiency_inv;      ///< Invers of modulation efficiency

	/**
	 * @brief Create the unit converter
	 *
	 * @param  duration_ms  The frame duration in ms
	 * @param  efficiency   The modulation efficiency
	 */
	UnitConverter(time_ms_t duration_ms, unsigned int efficiency);

 public:

	virtual ~UnitConverter();

	/**
	 * @brief Get the number of slots
	 *
	 * @param carrier_symps  the carrier rate
	 */
	unsigned int getSlotsNumber(rate_symps_t carrier_symps) const;

	/**
	 * @brief Get the packet length
	 * 
	 * @return  The packet length in bits
	 */
	virtual vol_b_t getPacketBitLength() const = 0;

	/**
	 * @brief Get the packet length
	 * 
	 * @return  The packet length in kbits
	 */
	virtual vol_kb_t getPacketKbitLength() const = 0;

	/**
	 * @brief Get the packet length
	 * 
	 * @return  The packet length in symbol
	 */
	virtual vol_sym_t getPacketSymbolLength() const = 0;

	/**
	 * @brief Set the frame duration
	 *
	 * @param duration_ms  The frame duration in ms
	 */
	void setFrameDuration(time_ms_t duration_ms);

	/**
	 * @brief Get the frame duration
	 * 
	 * @return  The frame duration in ms
	 */
	time_ms_t getFrameDuration() const;

	/**
	 * @brief Set the modulation efficiency
	 *
	 * @param efficiency  The modulation efficiency
	 */
	void setModulationEfficiency(unsigned int efficiency);

	/**
	 * @brief Get the modulation efficiency
	 * 
	 * @return  The modulation efficiency
	 */
	unsigned int getModulationEfficiency() const;

	vol_sym_t bitsToSym(vol_b_t vol_b) const;
	vol_b_t symToBits(vol_sym_t vol_sym) const;

	virtual vol_pkt_t symToPkt(vol_sym_t vol_sym) const = 0;
	virtual vol_sym_t pktToSym(vol_pkt_t vol_pkt) const = 0;

	virtual vol_pkt_t bitsToPkt(vol_b_t vol_b) const = 0;
	virtual vol_b_t pktToBits(vol_pkt_t vol_pkt) const = 0;
	
	vol_sym_t kbitsToSym(vol_kb_t vol_kb) const;
	vol_kb_t symToKbits(vol_sym_t vol_sym) const;

	virtual vol_pkt_t kbitsToPkt(vol_kb_t vol_kb) const = 0;
	virtual vol_kb_t pktToKbits(vol_pkt_t vol_pkt) const = 0;

	vol_kb_t bitsToKbits(vol_b_t vol_b) const;
	vol_b_t kbitsToBits(vol_kb_t vol_kb) const;

	rate_symps_t bpsToSymps(rate_bps_t rate_bps) const;
	rate_bps_t sympsToBps(rate_symps_t rate_symps) const;

	virtual rate_pktpf_t sympsToPktpf(rate_symps_t rate_symps) const = 0;
	virtual rate_symps_t pktpfToSymps(rate_pktpf_t rate_pktpf) const = 0;

	virtual rate_pktpf_t bpsToPktpf(rate_bps_t rate_bps) const = 0;
	virtual rate_bps_t pktpfToBps(rate_pktpf_t rate_pktpf) const = 0;
	
	rate_symps_t kbpsToSymps(rate_kbps_t rate_kbps) const;
	rate_kbps_t sympsToKbps(rate_symps_t rate_symps) const;

	virtual rate_pktpf_t kbpsToPktpf(rate_kbps_t rate_kbps) const = 0;
	virtual rate_kbps_t pktpfToKbps(rate_pktpf_t rate_pktpf) const = 0;

	rate_kbps_t bpsToKbps(rate_bps_t rate_bps) const;
	rate_bps_t kbpsToBps(rate_kbps_t rate_kbps) const;

	unsigned int pfToPs(unsigned int rate_pf) const;
	unsigned int psToPf(unsigned int rate_ps) const;
};

#endif

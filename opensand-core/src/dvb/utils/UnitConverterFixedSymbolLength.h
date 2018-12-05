/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
 * @file UnitConverterFixedSymbolLength.h
 * @brief Converters for OpenSAND units
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef _UNIT_CONVERTER_FIXED_SYMBOL_LENGTH_H_
#define _UNIT_CONVERTER_FIXED_SYMBOL_LENGTH_H_

#include "OpenSandCore.h"
#include "UnitConverter.h"

/**
 * @class UnitConverterFixedSymbolLength
 * @brief class managing unit conversion between kbits/s, cells per frame, etc
 */
class UnitConverterFixedSymbolLength: public UnitConverter
{
 protected:

	vol_sym_t packet_length_sym;    ///< Fixed packet length (in symbols)
	float packet_length_sym_inv;    ///< Inverse of fixed packet length (in symbols-1)

 public:

	/**
	 * @brief Create the unit converter
	 *
	 * @param  duration_ms  The frame duration in ms
	 * @param  efficiency   The modulation efficiency
	 * @param  length_sym   The fixed packet length in symbols
	 */
	UnitConverterFixedSymbolLength(
		time_ms_t duration_ms,
		unsigned int efficiency,
		vol_sym_t length_sym);
	virtual ~UnitConverterFixedSymbolLength();

	/**
	 * @brief Get the packet length
	 * 
	 * @return  The packet length in bits
	 */
	virtual vol_b_t getPacketBitLength() const;

	/**
	 * @brief Get the packet length
	 * 
	 * @return  The packet length in kbits
	 */
	virtual vol_kb_t getPacketKbitLength() const;

	/**
	 * @brief Get the packet length
	 * 
	 * @return  The packet length in symbol
	 */
	virtual vol_sym_t getPacketSymbolLength() const;

	/**
	 * @brief Set the fixed packet length
	 *
	 * @param length_sym  The fixed packet length in symbols
	 */
	void setPacketSymbolLength(vol_sym_t length_sym);
	
	virtual vol_pkt_t symToPkt(vol_sym_t vol_sym) const;
	virtual vol_sym_t pktToSym(vol_pkt_t vol_pkt) const;

	virtual vol_pkt_t bitsToPkt(vol_b_t vol_b) const;
	virtual vol_b_t pktToBits(vol_pkt_t vol_pkt) const;
	
	virtual vol_pkt_t kbitsToPkt(vol_kb_t vol_kb) const;
	virtual vol_kb_t pktToKbits(vol_pkt_t vol_pkt) const;

	virtual rate_pktpf_t sympsToPktpf(rate_symps_t rate_symps) const;
	virtual rate_symps_t pktpfToSymps(rate_pktpf_t rate_pktpf) const;

	virtual rate_pktpf_t bpsToPktpf(rate_bps_t rate_bps) const;
	virtual rate_bps_t pktpfToBps(rate_pktpf_t rate_pktpf) const;
	
	virtual rate_pktpf_t kbpsToPktpf(rate_kbps_t rate_kbps) const;
	virtual rate_kbps_t pktpfToKbps(rate_pktpf_t rate_pktpf) const;
};

#endif

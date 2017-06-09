/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file TerminalContextDamaRcs2.h
 * @brief The terminal context for DAMA DVB-RCS2
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 */


#ifndef _TERMINAL_CONTEXT_DAMA_RCS2_H_
#define _TERMINAL_CONTEXT_DAMA_RCS2_H_

#include "TerminalContextDamaRcs.h"
#include "UnitConverter.h"
#include "FmtDefinition.h"

/**
 * @class TerminalContextDamaRcs2
 */
class TerminalContextDamaRcs2: public TerminalContextDamaRcs
{
 public:

	/**
	 * @brief  Create a terminal context for DAMA
	 *
	 * @param  tal_id             terminal id.
	 * @param  cra_kbps           terminal CRA (kb/s).
	 * @param  max_rbdc_kbps      maximum RBDC value (kb/s).
	 * @param  rbdc_timeout_sf    RBDC timeout (in superframe number).
	 * @param  max_vbdc_kb        maximum VBDC value (kb).
	 * @param  frame_duration_ms  frame duration (ms)
	 * @prama  packet_length_b    packet length (b)
	 */
	TerminalContextDamaRcs2(tal_id_t tal_id,
		rate_kbps_t cra_kbps,
		rate_kbps_t max_rbdc_kbps,
		time_sf_t rbdc_timeout_sf,
		vol_kb_t max_vbdc_kb,
		time_ms_t frame_duration_ms,
		vol_b_t packet_length_b = 0);

	virtual ~TerminalContextDamaRcs2();

	/**
	 * @brief Set the current FMT of the terminal
	 *
	 * @param fmt_def  The current FMT of the terminal
	 */
	virtual void updateFmt(FmtDefinition *fmt);

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

  protected:

	/** The packet length (b) */
	vol_b_t packet_length_b;

	/** The unit converter */
	UnitConverter converter;

	/**
	 * @brief Get the payload length
	 *
	 * @return  The current payload length in bits
	 */
	vol_b_t getPayloadLength() const;
};

#endif

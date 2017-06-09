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
 * @file    TerminalContextDamaRcs2.cpp
 * @brief   The terminal context for DAMA DVB-RCS2
 * @author  Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "TerminalContextDamaRcs2.h"

TerminalContextDamaRcs2::TerminalContextDamaRcs2(tal_id_t tal_id,
		rate_kbps_t cra_kbps,
		rate_kbps_t max_rbdc_kbps,
		time_sf_t rbdc_timeout_sf,
		vol_kb_t max_vbdc_kb,
		time_ms_t frame_duration_ms,
		vol_b_t packet_length_b):
	TerminalContextDamaRcs(tal_id,
		cra_kbps,
		max_rbdc_kbps,
		rbdc_timeout_sf,
		max_vbdc_kb),
	packet_length_b(packet_length_b),
	converter(frame_duration_ms)
{
}

TerminalContextDamaRcs2::~TerminalContextDamaRcs2()
{
}

void TerminalContextDamaRcs2::updateFmt(FmtDefinition *fmt)
{
	TerminalContextDamaRcs::updateFmt(fmt);

	this->converter.updatePacketLength(this->getPayloadLength());
}

vol_b_t TerminalContextDamaRcs2::getPayloadLength() const
{
	vol_b_t payload_length = 0;
	
	if(this->fmt_def == NULL)
	{
		return 0;
	}

	if(!this->fmt_def->hasBurstLength())
	{
		payload_length = this->packet_length_b
			* this->fmt_def->getCodingRate();
	}
	else
	{
		payload_length = this->fmt_def->getBurstLength()
			* this->fmt_def->getModulationEfficiency()
			* this->fmt_def->getCodingRate();
	}

	return payload_length;
}

vol_b_t TerminalContextDamaRcs2::pktToBits(vol_pkt_t vol_pkt) const
{
	return this->converter.pktToBits(vol_pkt);
}

vol_kb_t TerminalContextDamaRcs2::pktToKbits(vol_pkt_t vol_pkt) const
{
	return this->converter.pktToKbits(vol_pkt);
}

vol_pkt_t TerminalContextDamaRcs2::kbitsToPkt(vol_kb_t vol_kb) const
{
	return this->converter.kbitsToPkt(vol_kb);
}

rate_pktpf_t TerminalContextDamaRcs2::kbpsToPktpf(rate_kbps_t rate_kbps) const
{
	return this->converter.kbpsToPktpf(rate_kbps);
}

rate_kbps_t TerminalContextDamaRcs2::pktpfToKbps(rate_pktpf_t rate_pktpf) const
{
	return this->converter.pktpfToKbps(rate_pktpf);
}


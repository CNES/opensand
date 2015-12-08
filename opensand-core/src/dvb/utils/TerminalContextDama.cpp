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
 * @file    TerminalContextDama.cpp
 * @brief   The terminal context for DAMA
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#include "TerminalContextDama.h"

#include <opensand_output/Output.h>


TerminalContextDama::TerminalContextDama(tal_id_t tal_id,
                                         rate_kbps_t cra_kbps,
                                         rate_kbps_t max_rbdc_kbps,
                                         time_sf_t rbdc_timeout_sf,
                                         vol_kb_t max_vbdc_kb):
	TerminalContext(tal_id),
	cra_kbps(cra_kbps),
	max_rbdc_kbps(max_rbdc_kbps),
	rbdc_timeout_sf(rbdc_timeout_sf),
	max_vbdc_kb(max_vbdc_kb),
	fmt_id(1), // at beginning the terminal need to be served while FMT ID is unknown
	carrier_id()
{
}

TerminalContextDama::~TerminalContextDama()
{
}

void TerminalContextDama::setCra(rate_kbps_t cra_kbps)
{
	this->cra_kbps = cra_kbps;
}

rate_kbps_t TerminalContextDama::getCra() const
{
	return this->cra_kbps;
}

void TerminalContextDama::setMaxRbdc(rate_kbps_t max_rbdc_kbps)
{
	this->max_rbdc_kbps = max_rbdc_kbps;
}

rate_kbps_t TerminalContextDama::getMaxRbdc() const
{
	return this->max_rbdc_kbps;
}

vol_kb_t TerminalContextDama::getMaxVbdc() const
{
	return this->max_vbdc_kb;
}

unsigned int TerminalContextDama::getFmtId()
{
	return this->fmt_id;
}

void TerminalContextDama::setFmtId(unsigned int fmt_id)
{
	this->fmt_id = fmt_id;
}

unsigned int TerminalContextDama::getCarrierId()
{
	return this->carrier_id;
}

void TerminalContextDama::setCarrierId(unsigned int carrier_id)
{
	this->carrier_id = carrier_id;
}



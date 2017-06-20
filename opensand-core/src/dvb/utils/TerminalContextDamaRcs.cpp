/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file    TerminalContextDamaRcs.cpp
 * @brief   The terminal context for RCS terminals handled with DAMA
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieutoulouse.viveris.com>
 */


#include "TerminalContextDamaRcs.h"

#include "OpenSandCore.h"

#include <opensand_output/Output.h>

#include <math.h>
#include <string>
#include <cstdlib>


TerminalContextDamaRcs::TerminalContextDamaRcs(tal_id_t tal_id,
                                               rate_kbps_t cra_kbps,
                                               rate_kbps_t max_rbdc_kbps,
                                               time_sf_t rbdc_timeout_sf,
                                               vol_kb_t max_vbdc_kb):
	TerminalContextDama(tal_id, cra_kbps, max_rbdc_kbps, rbdc_timeout_sf, max_vbdc_kb),
	carrier_id()
{
	this->setCra(cra_kbps);
	this->setMaxRbdc(max_rbdc_kbps);
	this->setRequiredFmt(NULL); // at beginning the terminal need to be served while FMT ID is unknown
	this->setFmt(NULL); // at beginning the terminal need to be served while FMT ID is unknown
}

TerminalContextDamaRcs::~TerminalContextDamaRcs()
{
}

unsigned int TerminalContextDamaRcs::getFmtId() const
{
	return this->fmt_def != NULL ? this->fmt_def->getId() : 0;
}

FmtDefinition *TerminalContextDamaRcs::getRequiredFmt() const
{
	return this->req_fmt_def;
}

void TerminalContextDamaRcs::setRequiredFmt(FmtDefinition *fmt)
{
	this->req_fmt_def = fmt;
}

FmtDefinition *TerminalContextDamaRcs::getFmt() const
{
	return this->fmt_def;
}

void TerminalContextDamaRcs::setFmt(FmtDefinition *fmt)
{
	this->fmt_def = fmt;
}

unsigned int TerminalContextDamaRcs::getCarrierId() const
{
	return this->carrier_id;
}

void TerminalContextDamaRcs::setCarrierId(unsigned int carrier_id)
{
	this->carrier_id = carrier_id;
}

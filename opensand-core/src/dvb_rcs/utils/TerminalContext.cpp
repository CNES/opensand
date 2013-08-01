/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file    TerminalContext.cpp
 * @brief   The terminal context.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#include "TerminalContext.h"

#define DBG_PACKAGE PKG_DAMA_DC
#include "opensand_conf/uti_debug.h"

#define DEFAULT_PRIO 1

TerminalContext::TerminalContext(tal_id_t tal_id,
                                 rate_kbps_t cra_kbps,
                                 rate_kbps_t max_rbdc_kbps,
                                 time_sf_t rbdc_timeout_sf,
                                 vol_kb_t min_vbdc_kb):
	tal_id(tal_id),
	category(""),
	cra_kbps(cra_kbps),
	max_rbdc_kbps(max_rbdc_kbps),
	rbdc_timeout_sf(rbdc_timeout_sf),
	min_vbdc_kb(min_vbdc_kb),
	fmt_id(0),
	carrier_id()
{
}

TerminalContext::~TerminalContext()
{
}

tal_id_t TerminalContext::getTerminalId() const
{
	return this->tal_id;
}

void TerminalContext::setCra(rate_kbps_t cra_kbps)
{
	this->cra_kbps = cra_kbps;
}

rate_kbps_t TerminalContext::getCra() const
{
	return this->cra_kbps;
}

void TerminalContext::setMaxRbdc(rate_kbps_t max_rbdc_kbps)
{
	this->max_rbdc_kbps = max_rbdc_kbps;
}

rate_kbps_t TerminalContext::getMaxRbdc() const
{
	return this->max_rbdc_kbps;
}

vol_kb_t TerminalContext::getMinVbdc() const
{
	return this->min_vbdc_kb;
}

unsigned int TerminalContext::getFmtId()
{
	return this->fmt_id;
}

void TerminalContext::setFmtId(unsigned int fmt_id)
{
	this->fmt_id = fmt_id;
}

unsigned int TerminalContext::getCarrierId()
{
	return this->carrier_id;
}

void TerminalContext::setCarrierId(unsigned int carrier_id)
{
	this->carrier_id = carrier_id;
}

void TerminalContext::setCurrentCategory(string name)
{
	this->category = name;
}

string TerminalContext::getCurrentCategory() const
{
	return this->category;
}


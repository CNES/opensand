/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author  Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include <cmath>

#include <opensand_output/Output.h>

#include "TerminalContextDama.h"


TerminalContextDama::TerminalContextDama(tal_id_t tal_id,
                                         rate_kbps_t cra_kbps,
                                         rate_kbps_t max_rbdc_kbps,
                                         time_sf_t rbdc_timeout_sf,
                                         vol_kb_t max_vbdc_kb):
	TerminalContext(tal_id),
	cra_request_kbps(cra_kbps),
	max_rbdc_kbps(max_rbdc_kbps),
	rbdc_timeout_sf(rbdc_timeout_sf),
	max_vbdc_kb(max_vbdc_kb),
	rbdc_credit(0.0),
	timer_sf(0),
	rbdc_request_kbps(0),
	rbdc_alloc_kbps(0),
	vbdc_request_kb(0),
	vbdc_alloc_kb(0),
	fca_alloc_kbps(0)
{
}

TerminalContextDama::~TerminalContextDama()
{
}

void TerminalContextDama::updateRbdcTimeout(time_sf_t timeout_sf)
{
	this->rbdc_timeout_sf = timeout_sf;
}

void TerminalContextDama::setRequiredCra(rate_kbps_t val_kbps)
{
	this->cra_request_kbps = val_kbps;
	LOG(this->log_band, LEVEL_INFO,
	    "Required CRA is %u kbits/s (for "
	    "ST%u)\n",
	    this->cra_request_kbps, this->tal_id);
}

rate_kbps_t TerminalContextDama::getRequiredCra() const
{
	return this->cra_request_kbps;
}

void TerminalContextDama::setCraAllocation(rate_kbps_t val_kbps)
{
	this->cra_alloc_kbps = val_kbps;
	LOG(this->log_band, LEVEL_INFO,
	    "Allocated CRA is %u kbits/s (for "
	    "ST%u)\n",
	    this->cra_alloc_kbps, this->tal_id);
}

rate_kbps_t TerminalContextDama::getCraAllocation() const
{
	return this->cra_alloc_kbps;
}

void TerminalContextDama::setMaxRbdc(rate_kbps_t val_kbps)
{
	this->max_rbdc_kbps = val_kbps;
	LOG(this->log_band, LEVEL_INFO,
	    "max RBDC is %u kbits/s (for "
	    "ST%u)\n", this->max_rbdc_kbps,
	    this->max_rbdc_kbps, this->tal_id);
}

rate_kbps_t TerminalContextDama::getMaxRbdc() const
{
	return this->max_rbdc_kbps;
}

vol_kb_t TerminalContextDama::getMaxVbdc() const
{
	return this->max_vbdc_kb;
}

void TerminalContextDama::setRequiredRbdc(rate_kbps_t val_kbps)
{
	// limit the requets to Max RBDC
	this->rbdc_request_kbps = std::min(val_kbps, this->max_rbdc_kbps);

	// save the request
	this->rbdc_credit = 0.0;
	this->timer_sf = this->rbdc_timeout_sf;
	LOG(this->log_band, LEVEL_DEBUG,
	    "new RBDC request %d (kb/s) credit %.2f timer %d for ST%u.\n",
	    this->rbdc_request_kbps, this->rbdc_credit,
	    this->timer_sf, this->tal_id);
}

rate_kbps_t TerminalContextDama::getRequiredRbdc() const
{
	return this->rbdc_request_kbps;
}

void TerminalContextDama::setRbdcAllocation(rate_kbps_t val_kbps)
{
	this->rbdc_alloc_kbps = val_kbps;
	LOG(this->log_band, LEVEL_DEBUG,
	    "RBDC allocation %u (kb/s) request %d (kb/s) credit %.2f timer %d for ST%u.\n",
	    this->rbdc_alloc_kbps, this->rbdc_request_kbps, this->rbdc_credit,
	    this->timer_sf, this->tal_id);
}

rate_kbps_t TerminalContextDama::getRbdcAllocation() const
{
	return this->rbdc_alloc_kbps;
}

void TerminalContextDama::addRbdcCredit(double credit)
{
	this->rbdc_credit += credit;
}

double TerminalContextDama::getRbdcCredit() const
{
	return this->rbdc_credit;
}

void TerminalContextDama::setRbdcCredit(double credit)
{
	this->rbdc_credit = credit;
}

time_sf_t TerminalContextDama::getTimer() const
{
	return this->timer_sf;
}

void TerminalContextDama::decrementTimer()
{
	if(0 < this->timer_sf)
	{
		--(this->timer_sf);
	}
	else
	{
		this->timer_sf = 0;	
	}

}

void TerminalContextDama::setRequiredVbdc(vol_kb_t val_kb)
{
	this->vbdc_request_kb += val_kb;
	this->vbdc_request_kb = std::min(this->vbdc_request_kb, this->max_vbdc_kb);
	LOG(this->log_band, LEVEL_DEBUG,
	    "new VBDC request %u (kb) for ST%u\n",
	    vbdc_request_kb, this->tal_id);
}

void TerminalContextDama::setVbdcAllocation(vol_kb_t val_kb)
{
	this->vbdc_alloc_kb = val_kb;
	if(this->vbdc_request_kb >= this->vbdc_alloc_kb)
	{
		// The allocation on Agent is processed per frame so for one TTP we
		// will allocate as many time the allocated value as we have frames
		// in superframes
		this->vbdc_request_kb -= this->vbdc_alloc_kb;
	}
	else
	{
		this->vbdc_request_kb = 0;
	}
	LOG(this->log_band, LEVEL_DEBUG,
	    "VBDC allocation %u (kb) request %d (kb) for ST%u.\n",
	    this->vbdc_alloc_kb, this->vbdc_request_kb, this->tal_id);
}

vol_kb_t TerminalContextDama::getVbdcAllocation() const
{
	return this->vbdc_alloc_kb;
}

vol_kb_t TerminalContextDama::getRequiredVbdc() const
{
	// the allocation is used for each frame per supertrame so it should
	// be divided by the number of frames per superframes
	return ceil(this->vbdc_request_kb);
}

void TerminalContextDama::setFcaAllocation(rate_kbps_t val_kbps)
{
	this->fca_alloc_kbps = val_kbps;
}

rate_kbps_t TerminalContextDama::getFcaAllocation() const
{
	return this->fca_alloc_kbps;
}

rate_kbps_t TerminalContextDama::getTotalRateAllocation() const
{
	LOG(this->log_band, LEVEL_DEBUG,
	    "Rate allocation: RBDC %u kb/s, FCA %u kb/s, "
	    "CRA %u kb/s for ST%u\n", this->rbdc_alloc_kbps,
	    this->fca_alloc_kbps, this->cra_alloc_kbps, this->tal_id);
	return this->rbdc_alloc_kbps + this->fca_alloc_kbps + this->cra_alloc_kbps;
}

vol_kb_t TerminalContextDama::getTotalVolumeAllocation() const
{
	return this->vbdc_alloc_kb;
}

bool TerminalContextDama::sortByRemainingCredit(const std::shared_ptr<TerminalContextDama> &e1,
                                                const std::shared_ptr<TerminalContextDama> &e2)
{
	return e1->rbdc_credit > e2->rbdc_credit;
}

bool TerminalContextDama::sortByVbdcReq(const std::shared_ptr<TerminalContextDama> &e1,
                                        const std::shared_ptr<TerminalContextDama> &e2)
{
	return e1->vbdc_request_kb > e2->vbdc_request_kb;
}


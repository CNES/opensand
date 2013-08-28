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
 * @file lib_dama_ctrl_st.cpp
 * @brief This bloc implements a ST and request context used by the DAMA CTRL
 *        within the NCC.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include <math.h>
#include <string>
#include <cstdlib>


#include "TerminalContextRcs.h"

#include "OpenSandCore.h"

#define DBG_PACKAGE PKG_DAMA_DC
#include "opensand_conf/uti_debug.h"
#define DC_DBG_PREFIX "[Generic]"


// TODO check requests validity (especially VBDC)

TerminalContextRcs::TerminalContextRcs(tal_id_t tal_id,
                                       rate_kbps_t cra_kbps,
                                       rate_kbps_t max_rbdc_kbps,
                                       time_sf_t rbdc_timeout_sf,
                                       vol_kb_t max_vbdc_kb,
                                       const UnitConverter *converter):
	TerminalContext(tal_id, cra_kbps, max_rbdc_kbps, rbdc_timeout_sf, max_vbdc_kb),
	rbdc_credit_pktpf(0.0),
	timer_sf(0),
	rbdc_request_pktpf(0),
	rbdc_alloc_pktpf(0),
	vbdc_request_pkt(0),
	vbdc_alloc_pkt(0),
	fca_alloc_pktpf(0),
	converter(converter)
{
	this->setMaxRbdc(max_rbdc_kbps);
	this->setCra(cra_kbps);
	this->max_vbdc_pkt = this->converter->kbitsToPkt(max_vbdc_kb);
}

TerminalContextRcs::~TerminalContextRcs()
{
}

void TerminalContextRcs::setCra(rate_kbps_t cra_kbps)
{
	this->cra_kbps = cra_kbps;
	this->cra_pktpf = this->converter->kbpsToPktpf(cra_kbps);
}

void TerminalContextRcs::setMaxRbdc(rate_kbps_t max_rbdc_kbps)
{
	this->max_rbdc_kbps = max_rbdc_kbps;
	this->max_rbdc_pktpf = this->converter->kbpsToPktpf(max_rbdc_kbps);
	UTI_DEBUG("max RBDC is %u kbits/s (%u packet per superframe for ST%u)\n",
	          this->max_rbdc_kbps, this->max_rbdc_pktpf, this->tal_id);
}

void TerminalContextRcs::setRbdcTimeout(time_sf_t rbdc_timeout_sf)
{
	this->rbdc_timeout_sf = rbdc_timeout_sf;
}

void TerminalContextRcs::setRequiredRbdc(rate_pktpf_t rbdc_request_pktpf)
{
	// limit the requets to Max RBDC
	rbdc_request_pktpf = std::min(rbdc_request_pktpf, this->max_rbdc_pktpf);

	// save the request
	this->rbdc_request_pktpf = rbdc_request_pktpf;
	this->rbdc_credit_pktpf = 0;
	this->timer_sf = this->rbdc_timeout_sf;
	UTI_DEBUG_L3("new RBDC request %d credit %f timer %d for ST%u.\n",
	             this->rbdc_request_pktpf, this->rbdc_credit_pktpf,
	             this->timer_sf, this->tal_id);
}

rate_pktpf_t TerminalContextRcs::getRequiredRbdc() const
{
	return this->rbdc_request_pktpf;
}

void TerminalContextRcs::setRbdcAllocation(rate_pktpf_t rbdc_alloc_pktpf)
{
	this->rbdc_alloc_pktpf = rbdc_alloc_pktpf;
}

void TerminalContextRcs::addRbdcCredit(rate_pktpf_t credit_pktpf)
{
	this->rbdc_credit_pktpf += credit_pktpf;
}

rate_pktpf_t TerminalContextRcs::getRbdcCredit()
{
	return this->rbdc_credit_pktpf;
}

void TerminalContextRcs::setRequiredVbdc(vol_pkt_t vbdc_request_pkt)
{
	this->vbdc_request_pkt += vbdc_request_pkt;
	this->vbdc_request_pkt = std::min(this->vbdc_request_pkt, this->max_vbdc_pkt);
	UTI_DEBUG_L3("new VBDC request %u for ST%u\n",
	             vbdc_request_pkt, this->tal_id);
}

void TerminalContextRcs::setVbdcAllocation(vol_pkt_t vbdc_alloc_pkt,
                                           unsigned int allocation_cycle)
{
	this->vbdc_alloc_pkt += vbdc_alloc_pkt;
	if(this->vbdc_request_pkt >= (vbdc_alloc_pkt * allocation_cycle))
	{
		// The allocation on Agent is processed per frame so for one TTP we
		// will allocate as many time the allocated value as we have frames
		// in superframes
		this->vbdc_request_pkt -= (vbdc_alloc_pkt * allocation_cycle);
	}
	else
	{
		this->vbdc_request_pkt = 0;
	}
}

vol_pkt_t TerminalContextRcs::getRequiredVbdc(unsigned int allocation_cycle) const
{
	// the allocation is used for each frame per supertrame so it should
	// be devided by the number of frames per superframes
	return ceil(this->vbdc_request_pkt / allocation_cycle);
}

void TerminalContextRcs::setFcaAllocation(rate_pktpf_t fca_alloc_pktpf)
{
	this->fca_alloc_pktpf = fca_alloc_pktpf;
}

rate_pktpf_t TerminalContextRcs::getTotalRateAllocation()
{
	UTI_DEBUG_L3("Rate allocation: RBDC %u packets, FCA %u packets, "
	             "CRA %u packets for ST%u\n", this->rbdc_alloc_pktpf,
	             this->fca_alloc_pktpf, this->cra_pktpf, this->tal_id);
	return this->rbdc_alloc_pktpf + this->fca_alloc_pktpf + this->cra_pktpf;
}

vol_pkt_t TerminalContextRcs::getTotalVolumeAllocation()
{
	return this->vbdc_alloc_pkt;
}

void TerminalContextRcs::onStartOfFrame()
{
	if(this->timer_sf > 0)
	{
		// timeout management
		this->timer_sf--;
	}

	if(this->timer_sf > 0)
	{
		if(this->rbdc_credit_pktpf >= 1.0)
		{
			this->rbdc_credit_pktpf -= 1.0;
			this->rbdc_request_pktpf++;
		}
	}
	else
	{
		this->rbdc_request_pktpf = 0;
		this->rbdc_credit_pktpf = 0.0;
	}

	this->rbdc_alloc_pktpf = 0;
	this->vbdc_alloc_pkt = 0;
	this->fca_alloc_pktpf = 0;
}


bool TerminalContextRcs::sortByRemainingCredit(const TerminalContextRcs *e1,
                                               const TerminalContextRcs *e2)
{
	return e1->rbdc_credit_pktpf > e2->rbdc_credit_pktpf;
}

bool TerminalContextRcs::sortByVbdcReq(const TerminalContextRcs *e1,
                                       const TerminalContextRcs *e2)
{
	return e1->vbdc_request_pkt > e2->vbdc_request_pkt;
}


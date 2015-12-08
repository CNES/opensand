/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file    DamaAgentRcsLegacy.h
 * @brief   This class defines the DAMA Agent interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#ifndef _DAMA_AGENT_RCS_LEGACY_H_
#define _DAMA_AGENT_RCS_LEGACY_H_

#include "DamaAgentRcs.h"

#include "OpenSandFrames.h"
#include "NetBurst.h"
#include "DvbRcsFrame.h"

#include <opensand_output/Output.h>

class DamaAgentRcsLegacy: public DamaAgentRcs
{
 public:

	DamaAgentRcsLegacy();
	virtual ~DamaAgentRcsLegacy();

	// Init method
	bool init();

	// Inherited methods
	bool hereIsSOF(time_sf_t superframe_number_sf);

 protected:

	/** VBDC credit */
	time_pkt_t vbdc_credit_pkt;

 private:

	rate_kbps_t computeRbdcRequest();
	vol_pkt_t computeVbdcRequest();

};

#endif


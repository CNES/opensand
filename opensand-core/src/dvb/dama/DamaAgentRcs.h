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
 * @file DamaAgentRcs.h
 * @brief Implementation of the DAMA agent for DVB-RCS emission standard.
 * @author Audric Schiltknecht / Viveris Technologies
 * @author Julien Bernard / Viveris Technologies
 */

#ifndef _DAMA_AGENT_RCS_H_
#define _DAMA_AGENT_RCS_H_

#include "DamaAgentRcsCommon.h"
#include "ReturnSchedulingRcs.h"

#include <opensand_output/OutputLog.h>

class DamaAgentRcs : public DamaAgentRcsCommon
{
 public:
	DamaAgentRcs();
	virtual ~DamaAgentRcs();

	// Inherited methods
	virtual bool hereIsTTP(Ttp *ttp);

 protected:
	/**
	 * @brief Generate a return link scheduling specialized to DVB-RCS, DVB-RCS2
	 *        or other
	 * @return                  the generated scheduling
	 */
	ReturnSchedulingRcsCommon *generateReturnScheduling() const;

	/**
	 * @brief Compute RBDC request
	 *
	 * @return                  the RBDC Request in kbits/s
	 */
	virtual rate_kbps_t computeRbdcRequest() = 0;

	/**
	 * @brief Compute VBDC request
	 *
	 * @return                  the VBDC Request in number of packets
	 *                          ready to be set in SAC field
	 */
	virtual vol_pkt_t computeVbdcRequest() = 0;
};

#endif


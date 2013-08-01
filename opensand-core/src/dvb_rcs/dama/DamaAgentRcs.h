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

#include "DamaAgent.h"

class DamaAgentRcs : public DamaAgent
{
 public:
	DamaAgentRcs();

	// Inherited methods
	virtual bool processOnFrameTick();
	virtual bool hereIsSOF(time_sf_t superframe_number_sf);

 protected:
	/** Current frame 0 <= current_frame < frames_per_superframes */
	time_frame_t current_frame;

	/** Number of allocated timeslots  */
	time_pkt_t allocated_pkt;

	/** Dynamic allocation in packets number */
	time_pkt_t dynamic_allocation_pkt;
	/** Remaining allocation for frames between two SF */
	rate_pktpf_t remaining_allocation_pktpf;
};

#endif


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
 * @file    DamaAgentRcsRrmQos.h
 * @brief   This class defines the DAMA Agent interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#ifndef _DAMA_AGENT_RCS_RRMQOS_H_
#define _DAMA_AGENT_RCS_RRMQOS_H_

#include "DamaAgentRcs.h"

#include "OpenSandFrames.h"
#include "NetBurst.h"
#include "DvbRcsFrame.h"

#include <opensand_output/Output.h>

class DamaAgentRcsRrmQos: public DamaAgentRcs
{
 public:

	DamaAgentRcsRrmQos();
	virtual ~DamaAgentRcsRrmQos();

	// Init method
	bool init();

	// Inherited methods
	bool hereIsSOF(time_sf_t superframe_number_sf);

 protected:

	/** VBDC credit */
	time_pkt_t vbdc_credit_pkt;

	/** Circular buffer to store previous RBDC requests */
	CircularBuffer *rbdc_request_buffer;

	// UL allocation in number of time-slots per frame
	CircularBuffer *dyn_alloc; ///< dynamic bandwith allocated in nb of
	                            ///  time-slots/frame for the last frames
	                            ///  (its also contains allocated CRA)
	CircularBuffer *rin;		    ///< circular buffer used to save the
   									///	incoming rates wieghted with a 
									/// coefficient for each 
									/// frame of the last OBR period
	double *rin_coeff;		///< coefficient array to balance the incoming
							/// rate (Rin) of each frame
	

 private:

	rate_kbps_t computeRbdcRequest();
	vol_pkt_t computeVbdcRequest();
};

#endif


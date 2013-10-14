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

#include "UnitConverter.h"
#include "CircularBuffer.h"
#include "ReturnSchedulingRcs.h"
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
	bool hereIsTTP(Ttp &ttp);
	bool processOnFrameTick();
	bool returnSchedule(list<DvbFrame *> *complete_dvb_frames);
	bool buildCR(cr_type_t cr_type,
	             CapacityRequest &capacity_request,
	             bool &emtpy);
	//void updateStatistics();
	void updateStatistics();

 protected:

	/** Is CRA taken into account in the RBDC computation ? */
	bool cra_in_cr;

	/** RBDC timer */
	time_sf_t rbdc_timer_sf;

	/** VBDC credit */
	time_pkt_t vbdc_credit_pkt;

	/** Unit converter */
	UnitConverter* converter;

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
	
	/** Uplink Scheduling functions */
	ReturnSchedulingRcs *ret_schedule;


 private:

	/**
	 * @brief Utility function to get total buffer size of all MAC fifos
	 *        associated to the concerned CR type
	 *
	 * @param cr_type           the type of capacity request
	 *
	 * @return                  total buffers size in packes number
	 */
	vol_pkt_t getMacBufferLength(cr_type_t cr_type);
	/**
	 * @brief Utility function to get total number of "last arrived" packets
	 *        (since last CR) of all MAC fifos associated to the concerned CR type
	 *
	 * @param crType            the type of capacity request
	 *
	 * @return                  total number of "last arrived" packets"
	 *                          in packets number
	 */
	vol_pkt_t getMacBufferArrivals(cr_type_t cr_type);

	/**
	 * @brief Compute RBDC request
	 *
	 * @return                  the RBDC Request in kbits/s
	 */
	rate_kbps_t computeRbdcRequest();

	/**
	 * @brief Compute VBDC request
	 *
	 * @return                  the VBDC Request in number of packets
	 *                          ready to be set in SAC field
	 */
	vol_pkt_t computeVbdcRequest();
};

#endif


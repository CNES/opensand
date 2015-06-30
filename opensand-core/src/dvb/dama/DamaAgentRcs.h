/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
#include "UnitConverter.h"
#include "ReturnSchedulingRcs.h"
#include "CircularBuffer.h"

#include <opensand_output/OutputLog.h>

class DamaAgentRcs : public DamaAgent
{
 public:
	DamaAgentRcs();
	virtual ~DamaAgentRcs();

	// Init method
	bool init();

	// Inherited methods
	virtual bool processOnFrameTick();
	virtual bool returnSchedule(list<DvbFrame *> *complete_dvb_frames);
	virtual bool hereIsSOF(time_sf_t superframe_number_sf);
	virtual bool hereIsTTP(Ttp *ttp);
	virtual bool buildSAC(ret_access_type_t cr_type,
	                      Sac *sac,
	                      bool &emtpy);

 protected:
	/** Number of allocated timeslots  */
	time_pkt_t allocated_pkt;

	/** Dynamic allocation in packets number */
	time_pkt_t dynamic_allocation_pkt;
	/** Remaining allocation for frames between two SF */
	rate_pktpf_t remaining_allocation_pktpf;

	/** Circular buffer to store previous RBDC requests */
	CircularBuffer *rbdc_request_buffer;

	/** Uplink Scheduling functions */
	ReturnSchedulingRcs *ret_schedule;

	/** Unit converter */
	UnitConverter* converter;

	/** RBDC timer */
	time_sf_t rbdc_timer_sf;

	/** The current MODCOD id read in TTP, this is used to inform sat and gw upon
	 *  frames reception instead of keeping TTP contexts.
	 *  Only one modcod_id here because we only receive one TTP per allocation */
	uint8_t modcod_id;

	/**
	 * @brief Utility function to get total buffer size of all MAC fifos
	 *        associated to the concerned CR type
	 *
	 * @param cr_type           the type of capacity request
	 *
	 * @return                  total buffers size in packes number
	 */
	vol_pkt_t getMacBufferLength(ret_access_type_t cr_type);
	/**
	 * @brief Utility function to get total number of "last arrived" packets
	 *        (since last SAC) of all MAC fifos associated to the concerned CR type
	 *
	 * @param cr_type            the type of capacity request
	 *
	 * @return                  total number of "last arrived" packets"
	 *                          in packets number
	 */
	vol_pkt_t getMacBufferArrivals(ret_access_type_t cr_type);

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

	/// The MODCOD for emmited frames as received in TTP
	Probe<int> *probe_st_used_modcod;
};

#endif


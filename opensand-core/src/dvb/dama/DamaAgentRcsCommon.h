/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file DamaAgentRcsCommon.h
 * @brief Implementation of the DAMA agent for DVB-RCS(2) emission standard.
 * @author Audric Schiltknecht / Viveris Technologies
 * @author Julien Bernard / Viveris Technologies
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef _DAMA_AGENT_RCS_COMMON_H_
#define _DAMA_AGENT_RCS_COMMON_H_

#include "DamaAgent.h"
#include "UnitConverter.h"
#include "ReturnSchedulingRcsCommon.h"
#include "CircularBuffer.h"
#include "FmtDefinitionTable.h"

#include <opensand_output/OutputLog.h>

class DamaAgentRcsCommon : public DamaAgent
{
 public:
	DamaAgentRcsCommon(FmtDefinitionTable *ret_modcod_def);
	virtual ~DamaAgentRcsCommon();

	// Init method
	virtual bool init();

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
	vol_kb_t allocated_kb;

	/** Dynamic allocation in packets number */
	vol_kb_t dynamic_allocation_kb;

	/** Remaining allocation for frames between two SF */
	vol_b_t remaining_allocation_b;

	/** Payload length **/
	vol_b_t burst_length_b;

	/** Circular buffer to store previous RBDC requests */
	CircularBuffer *rbdc_request_buffer;

	/** Uplink Scheduling functions */
	ReturnSchedulingRcsCommon *ret_schedule;

	/** Unit converter */
	UnitConverter *converter;

	/** RBDC timer */
	time_sf_t rbdc_timer_sf;

	/** The MODCOD definition table for return link */
	FmtDefinitionTable * ret_modcod_def;
	
	/** The current MODCOD id read in TTP, this is used to inform sat and gw upon
	 *  frames reception instead of keeping TTP contexts.
	 *  Only one modcod_id here because we only receive one TTP per allocation */
	fmt_id_t modcod_id;

	/**
	 * @brief Generate an unit converter
	 *
	 * @return  the generated unit converter
	 */
	virtual UnitConverter *generateUnitConverter() const = 0;

	/**
	 * @brief Generate a return link scheduling specialized to DVB-RCS, DVB-RCS2
	 *        or other
	 * @return                  the generated scheduling
	 */
	virtual ReturnSchedulingRcsCommon *generateReturnScheduling() const = 0;

	/**
	 * @brief Utility function to get total buffer size of all MAC fifos
	 *        associated to the concerned CR type
	 *
	 * @param cr_type           the type of capacity request
	 *
	 * @return                  total buffers size in bits
	 */
	vol_b_t getMacBufferLength(ret_access_type_t cr_type);

	/**
	 * @brief Utility function to get total number of "last arrived" packets
	 *        (since last SAC) of all MAC fifos associated to the concerned CR type
	 *
	 * @param cr_type            the type of capacity request
	 *
	 * @return                  total number of "last arrived" packets
	 *                          in bits
	 */
	vol_b_t getMacBufferArrivals(ret_access_type_t cr_type);

	/**
	 * @brief Compute RBDC request
	 *
	 * @return                  the RBDC Request in kbits/s
	 */
	virtual rate_kbps_t computeRbdcRequest() = 0;

	/**
	 * @brief Compute VBDC request
	 *
	 * @return                  the VBDC Request in kbits
	 *                          ready to be set in SAC field
	 */
	virtual vol_kb_t computeVbdcRequest() = 0;

	/// The MODCOD for emmited frames as received in TTP
	Probe<int> *probe_st_sent_modcod;

	/**
	 * @brief Check RBDC request value is lower than the max value it could
	 *        be sent in the SAC
	 * @param request_kbps  the RBDC request value (kb/s)
	 *
	 * @return the lower value between the RBDC value and the max SAC value
	 */
	static rate_kbps_t checkRbdcRequest(rate_kbps_t request_kbps);

	/**
	 * @brief Check VBDC request value is lower than the max value it could
	 *        be sent in the SAC
	 * @param request_kb  the VBDC request value (kb)
	 *
	 * @return the lower value between the VBDC value and the max SAC value
	 */
	static vol_kb_t checkVbdcRequest(vol_kb_t request_kb);
};

#endif


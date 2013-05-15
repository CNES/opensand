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
 * @file    DamaAgentRcsLegacy.h
 * @brief   This class defines the DAMA Agent interfaces
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#ifndef _DAMA_AGENT_RCS_LEGACY_H_
#define _DAMA_AGENT_RCS_LEGACY_H_

#include "DamaAgentRcs.h"

#include "CircularBuffer.h"
#include "UplinkSchedulingRcs.h"

#include "msg_dvb_rcs.h"
#include "NetBurst.h"
#include "DvbRcsFrame.h"

class DamaAgentRcsLegacy : public DamaAgentRcs
{
 public:

	DamaAgentRcsLegacy(const EncapPlugin::EncapPacketHandler *pkt_hdl,
	                   const std::map<unsigned int, DvbFifo *> &dvb_fifos);
	virtual ~DamaAgentRcsLegacy();

	// Init method
	bool init();
	
	// Inherited methods
	virtual bool hereIsSOF(time_sf_t superframe_number_sf);
	virtual bool hereIsTTP(const Ttp &ttp);
	virtual bool processOnFrameTick();
	virtual bool uplinkSchedule(std::list<DvbFrame *> *complete_dvb_frames);
	virtual bool buildCR(cr_type_t cr_type,
	                     CapacityRequest **capacity_request,
	                     bool &emtpy);

 protected:

	/** Is CRA taken into account in the RBDC computation ? */
	bool cra_in_cr;

	/** RBDC timer */
	time_sf_t rbdc_timer_sf;

	/** VBDC credit */
	time_pkt_t vbdc_credit_pkt;

	/** Unit converter */
	DU_Converter* converter;

	/** Circular buffer to store previous RBDC requests */
	CircularBuffer *rbdc_request_buffer;

	/** Uplink Scheduling functions */
	UplinkSchedulingRcs up_schedule;

 private:

	/**
	 * @brief Utility function to get total buffer size of all MAC fifos
	 *        associated to the concerned CR type
	 *
	 * @param cr_type           the type of capacity request
	 *
	 * @return                  total buffers size in packes/cells number
	 */
	vol_pkt_t getMacBufferLength(cr_type_t cr_type);
	/**
	 * @brief Utility function to get total number of "last arrived" packets/cells
	 *        (since last CR) of all MAC fifos associated to the concerned CR type
	 *
	 * @param crType            the type of capacity request
	 *
	 * @return                  total number of "last arrived" cells
	 *                          in packets/cells number
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
	 * @return                  the VBDC Request in number of packets/cells
	 *                          ready to be set in SAC field
	 */
	vol_pkt_t computeVbdcRequest();
};

#endif


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
 * @file Scheduling.h
 * @brief Scheduling for MAC FIFOs
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 *
 */

#ifndef _SCHEDULING_H_
#define _SCHEDULING_H_

#include "EncapPlugin.h"
#include "DvbFifoTypes.h"
#include "DvbFrame.h"


class StFmtSimuList;
class OutputLog;


/**
 * Scheduling is done each frame (not each superframe),
 * so allocation should be done in slot per frame (packet per frame)
 */

/**
 * @class Scheduling
 * @brief Scheduling functions for MAC FIFOs
 */
class Scheduling
{
public:
	Scheduling(std::shared_ptr<EncapPlugin::EncapPacketHandler> packet_handler,
	           std::shared_ptr<fifos_t> fifos,
	           std::shared_ptr<const StFmtSimuList> simu_sts);

	virtual ~Scheduling() = default;

	/**
	 * @brief Schedule packets emission.
	 *
	 * @param complete_dvb_frames   created DVB frames.
	 * @param current_superframe_sf the current superframe
	 * @param remaining_allocation  the remaining allocation after scheduling
	 *                              on the current superframe
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool schedule(const time_sf_t current_superframe_sf,
	                      std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
	                      uint32_t &remaining_allocation) = 0;

	
	/**
	 * @brief Get the current MODCOD ID of the ST whose ID is given as input
	 *
	 * @param id  the ID of the ST
	 * @return    the current MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	uint8_t getCurrentModcodId(tal_id_t id) const;

protected:
	/** The packet representation */
	std::shared_ptr<EncapPlugin::EncapPacketHandler> packet_handler;
	/** The MAC FIFOs */
	std::shared_ptr<fifos_t> dvb_fifos;
	/** The FMT simulated data */
	std::shared_ptr<const StFmtSimuList> simu_sts;

	/// Fragment of a packet that couldn't be scheduled
	/// in a single call to schedule; saved for priority
	/// scheduling in the next call.
	Rt::Ptr<NetPacket> remaining_data;

	// Output Log
	std::shared_ptr<OutputLog> log_scheduling;
};

#endif


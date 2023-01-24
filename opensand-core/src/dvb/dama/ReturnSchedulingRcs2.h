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
 * @file ReturnSchedulingRcs2.h
 * @brief Scheduling for MAC FIFOs for DVB-RCS2 return link
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 */

#ifndef _RETURN_SCHEDULING_RCS2_H_
#define _RETURN_SCHEDULING_RCS2_H_

#include "Scheduling.h"
#include "DvbRcsFrame.h"

#include <opensand_output/OutputLog.h>

/**
 * @class ReturnSchedulingRcs2
 * @brief Scheduling functions for MAC FIFOs with DVB-RCS2 return link
 */
class ReturnSchedulingRcs2: public Scheduling
{
public:
	ReturnSchedulingRcs2(EncapPlugin::EncapPacketHandler *packet_handler,
	                          const fifos_t &fifos);

	virtual ~ReturnSchedulingRcs2() {};

	vol_b_t getMaxBurstLength() const;
	void setMaxBurstLength(vol_b_t length_b);
	
	bool schedule(const time_sf_t current_superframe_sf,
	              clock_t current_time,
	              std::list<Rt::Ptr<DvbFrame>> *complete_dvb_frames,
	              uint32_t &remaining_allocation);

protected:
	/// The maximum burst length in bits
	vol_b_t max_burst_length_b;

	/**
	 * @brief schedule the DVB packets that are stored in the MAC Fifo
	 *
	 * @param current_superframe_sf        the current superframe (for logging)
	 * @param complete_dvb_frames          a list of completed DVB frames
	 * @param remaining_allocation_b       the remaining allocated data length after
	 *                                     scheduling on the current superframe
	 *
	 * @return true on success, false otherwise
	 */
	bool macSchedule(const time_sf_t current_superframe_sf,
	                 std::list<Rt::Ptr<DvbFrame>> *complete_dvb_frames,
	                 vol_b_t &remaining_allocation_b);

	/**
	 * @brief Allocate a new DVB frame
	 *
	 * @param incomplete_dvb_frame  the created DVB frame
	 *
	 * @return true on sucess, false otherwise
	 */
	bool allocateDvbRcsFrame(Rt::Ptr<DvbRcsFrame> &incomplete_dvb_frame);
};

#endif


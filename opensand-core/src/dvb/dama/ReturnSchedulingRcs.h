/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file ReturnSchedulingRcs.h
 * @brief Scheduling for MAC FIFOs for DVB-RCS return link
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 *
 */

#ifndef _RETURN_SCHEDULING_RCS_H_
#define _RETURN_SCHEDULING_RCS_H_

#include "ReturnSchedulingRcsCommon.h"
#include "DvbRcsFrame.h"

#include <opensand_output/OutputLog.h>

/**
 * @class ReturnSchedulingRcs
 * @brief Scheduling functions for MAC FIFOs with DVB-RCS return link
 */
class ReturnSchedulingRcs: public ReturnSchedulingRcsCommon
{
  public:

	ReturnSchedulingRcs(EncapPlugin::EncapPacketHandler *packet_handler,
	                    const fifos_t &fifos);

	virtual ~ReturnSchedulingRcs() {};

  protected:

	/**
	 * @brief schedule the DVB packets that are stored in the MAC Fifo
	 *
	 * @param current_superframe_sf        the current superframe (for logging)
	 * @param complete_dvb_frames          a list of completed DVB frames
	 * @param remaining_allocation_kb      the remaining allocated data length after
	 *                                     scheduling on the current superframe
	 *
	 * @return true on success, false otherwise
	 */
	bool macSchedule(const time_sf_t current_superframe_sf,
	                 list<DvbFrame *> *complete_dvb_frames,
	                 vol_kb_t &remaining_allocation_kb);

	/**
	 * @brief Allocate a new DVB frame
	 *
	 * @param incomplete_dvb_frame  the created DVB frame
	 *
	 * @return true on sucess, false otherwise
	 */
	bool allocateDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame);

};

#endif


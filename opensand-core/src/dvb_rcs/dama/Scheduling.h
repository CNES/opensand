/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
#include "DvbFifo.h"
#include "DvbFrame.h"

using std::list;

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

	Scheduling(const EncapPlugin::EncapPacketHandler *packet_handler,
	           const fifos_t &fifos):
		packet_handler(packet_handler),
		dvb_fifos(fifos)
	{
		// Output log
		this->log_scheduling = Output::registerLog(LEVEL_WARNING,
		                                           "Dvb.Scheduling"); 
	};

	/**
	 * @brief Schedule packets emission.
	 *
	 * @param complete_dvb_frames   created DVB frames.
	 * @param current_superframe_sf the current superframe
	 * @param current_frame         the current frame
	 * @param current_time          the current time
	 * @param remaining_allocation  the remaining allocation after scheduling
	 *                              on the current superframe
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool schedule(const time_sf_t current_superframe_sf,
	                      const time_frame_t current_frame,
	                      clock_t current_time,
	                      list<DvbFrame *> *complete_dvb_frames,
	                      uint32_t &remaining_allocation) = 0;

  protected:

    /** The packet representation */
	const EncapPlugin::EncapPacketHandler *packet_handler;
	/** The MAC FIFOs */
    const fifos_t dvb_fifos;

	// Output Log
	OutputLog *log_scheduling;
};

#endif


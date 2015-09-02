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
 * @file UplinkSchedulingRcs.h
 * @brief Scheduling for MAC FIFOs for DVB-RCS uplink on GW
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 *
 */

#ifndef _UPLINK_SCHEDULING_RCS_H_
#define _UPLINK_SCHEDULING_RCS_H_

#include "Scheduling.h"
#include "DvbRcsFrame.h"
#include "TerminalCategoryDama.h"
#include "FmtSimulation.h"

#include <opensand_output/OutputLog.h>

/**
 * @class UplinkSchedulingRcs
 * @brief Scheduling functions for MAC FIFOs with DVB-RCS uplink
 */
class UplinkSchedulingRcs: public Scheduling
{
  public:

	UplinkSchedulingRcs(const EncapPlugin::EncapPacketHandler *packet_handler,
	                    const fifos_t &fifos,
	                    const ListStFmt *const ret_sts,
	                    const FmtDefinitionTable *const ret_modcod_def,
	                    const TerminalCategoryDama *const category,
	                    tal_id_t gw_id);

	bool schedule(const time_sf_t current_superframe_sf,
	              clock_t current_time,
	              list<DvbFrame *> *complete_dvb_frames,
	              uint32_t &remaining_allocation);

  private:
	
	// the gw_id
	tal_id_t gw_id;

	/// The FMT definition table associated
	const FmtDefinitionTable *ret_modcod_def;

	///The terminal category
	const TerminalCategoryDama *category;

	/**
	 * @brief Schedule encapsulated packets from a FIFO and for a given carriers
	 *        group
	 *
	 * @param fifo  The FIFO whee packets are stored
	 * @param current_superframe_sf  The current superframe number
	 * @param current_time           The current time
	 * @param complete_dvb_frames    The list of complete DVB frames
	 * @param carriers               The carriers group
	 */
	bool scheduleEncapPackets(DvbFifo *fifo,
	                          const time_sf_t current_superframe_sf,
	                          clock_t current_time,
	                          list<DvbFrame *> *complete_dvb_frames,
	                          CarriersGroupDama *carriers);


	/**
	 * @brief Create an incomplete DVB-RCS frame
	 *
	 * @param incomplete_dvb_frame OUT: the DVB-RCS frame that will be created
	 * @param modcod_id            The MODCOD ID of the frame
	 * return                      true on success, false otherwise
	 */
	bool createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame,
	                                 uint8_t modcod_id);


	/**
	 * @brief Get the current simulated MODCOD ID for GW uplink
	 *
	 * @return the simulated modcod ID for GW uplink
	 */
	uint8_t retrieveCurrentModcod(void);

};

#endif


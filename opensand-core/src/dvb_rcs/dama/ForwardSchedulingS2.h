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
 * @file ForwardSchedulingS2.h
 * @brief Scheduling for MAC FIFOs for DVB-S2 forward
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 *
 */

#ifndef _FORWARD_SCHEDULING_S2_H_
#define _FORWARD_SCHEDULING_S2_H_

#include "Scheduling.h"

#include "BBFrame.h"
#include "FmtSimulation.h"
#include "TerminalCategory.h"


/** Status for the carrier capacity */
typedef enum
{
	status_ok,    // BBFrame added in the complete BBFrames list
	status_error, // Error when adding the BBFrame in the list
	status_full,  // The carrier is full, cannot add the BBFrame
} sched_status_t;



/**
 * @class ForwardSchedulingS2
 * @brief Scheduling functions for MAC FIFOs with DVB-S2 forward
 */
class ForwardSchedulingS2: public Scheduling
{
  public:

	ForwardSchedulingS2(const EncapPlugin::EncapPacketHandler *packet_handler,
	                    const fifos_t &fifos,
	                    FmtSimulation *const fwd_fmt_simu,
	                    const TerminalCategory *const category);

	~ForwardSchedulingS2();

	bool schedule(const time_sf_t current_superframe_sf,
	              const time_frame_t current_frame,
	              clock_t current_time,
	              list<DvbFrame *> *complete_dvb_frames,
	              uint32_t &remaining_allocation);
  
  private:

  	/** the BBFrame being built identified by their modcod */
	map<unsigned int, BBFrame *> incomplete_bb_frames;

	/** the BBframe being built in their created order */
	list<BBFrame *> incomplete_bb_frames_ordered;

	/** the pending BBFrame list if there was not enough space in previous iteration
	 *  for the corresponding MODCOD */
	list<BBFrame *> pending_bbframes;

	/** The FMT simulated data */
	FmtSimulation *fwd_fmt_simu;

	/** The terminal category */
	const TerminalCategory *category;

	// Total and unused capacity probes
	Probe<int> *probe_fwd_total_capacity;
	Probe<int> *probe_fwd_remaining_capacity;

	/**
	 * @brief Schedule encapsulated packets from a FIFO and for a given Rs
	 *        The available capacity is obtained from carrier capacity in symbols
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
	                          CarriersGroup *carriers);

	/**
	 * @brief Get the current modcod of a terminal
	 *
	 * @param tal_id    the terminal id
	 * @param modcod_id OUT: the modcod_id retrived from the terminal ID
	 * @return          true on succes, false otherwise
	 */
	bool retrieveCurrentModcod(tal_id_t tal_id, unsigned int &modcod_id);

	/**
	 * @brief Create an incomplete BB frame
	 *
	 * @param bbframe   the BBFrame that will be created
	 * @param modcod_id the BBFrame modcod
	 * @return          true on succes, false otherwise
	 */
	bool createIncompleteBBFrame(BBFrame **bbframe,
	                             unsigned int modcod_id);

	/**
	 * @brief Get the incomplete BBFrame for the current destination terminal
	 *
	 * @param tal_id    the terminal ID we want to send the frame
	 * @paarm carriers  the carriers group to which the terminal belongs
	 * @param bbframe   OUT: the BBframe for this packet
	 * @return          true on success, false otherwise
	 */
	bool getIncompleteBBFrame(tal_id_t tal_id,
	                          CarriersGroup *carriers,
	                          BBFrame **bbframe);

	/**
	 * @brief Add a BBframe to the list of complete BB frames
	 *
	 * @param complete_bb_frames the list of complete BB frames
	 * @param bbframe            the BBFrame to add in the list
	 * @param duration_credit    IN/OUT: the remaining credit for the current frame
	 * @return                   status_ok on success, status_error on error and
	 *                           status_full -2 if there is not enough capacity
	 */
	sched_status_t addCompleteBBFrame(list<DvbFrame *> *complete_bb_frames,
	                                  BBFrame *bbframe,
	                                  vol_sym_t &remaining_capacity_sym);


	/**
	 * @brief Schedule pending BBFrames from previous slot
	 *
	 * @param supported_modcods    The list of supported MODCODS for the current carrier
	 * @param complete_dvb_frames  IN/OUT: The list of complete DVB frames
	 * @param capacity_sym         IN/OUT: The remaining capacity on carriers
	 */
	void schedulePending(const list<unsigned int> supported_modcods,
	                     list<DvbFrame *> *complete_dvb_frames,
	                     vol_sym_t &remaining_capacity_sym);

	/**
	 * @brief  Get BBFrame size in symbols according to its MODCOD and size in bytes
	 *
	 * @param bbframe_size_bytes  The BBFrame size in bytes
	 * @param modcod_id           The BBFrame MODCOD ID
	 * @param bbframe_size_sym    OUT: The BBFrame size in symbols
	 * @return true on success, false otherwise
	 */
	bool getBBFrameSize(size_t bbframe_size_bytes,
	                    unsigned int modcod_id,
	                    vol_sym_t &bbframe_size_sym);

};

#endif


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
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file    Ttp.h
 * @brief   Generic TTP (Timeslot Time Plan)
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _TTP_H_
#define _TTP_H_

#include "OpenSandCore.h"

#include <map>
#include <vector>
#include <stdint.h>

using std::vector;
using std::map;

/** The information related to TTP */
typedef struct
{
	// TODO group_id ?
	uint16_t superframe_count; ///< Superframe count to wich the TP applies
	uint8_t frame_loop_count;  ///< One less than the number of superframe
} __attribute__((packed)) ttp_info_t;

/** The information related to frame */
typedef struct
{
	uint8_t frame_number;            ///< The frame number within the superframe
	// TODO we don't do one less
	uint16_t tp_loop_count;           ///< one less than the number of the frame
	                                   //   TP entry loops that follow
	                                   //   nbr max loop = nbr max of terminals
} __attribute__((packed)) frame_info_t;


/** The emulated Time Plan */
typedef struct
{
	tal_id_t tal_id;   ///> The terminal ID
	                   //   size 5 for physical ST, 5->max for simulated ST requests
	int32_t offset;    ///> The offset in the superframe (start_slot for RCS)
	// TODO we don't do one less
	// TODO uint8_t in standard and we should build more thant one TTP per ST
	uint16_t assignment_count; ///> one less than the number of timeslots assigned
	                           //   in the block (for RCS)
	uint8_t fmt_id;    ///> The ID for FMT (MODCOD ID)
	uint8_t priority;  ///> The traffic priority (no used in RCS)
} __attribute__((packed)) emu_tp_t;

/** The emulated frame */
typedef struct
{
	frame_info_t frame_info;  ///< the frame specific content
	emu_tp_t tp;              ///< The first Time Plans in the frame
	                          //   max nbr of terminals = broadcast tal_id
} __attribute__((packed)) emu_frame_t;

/** The emulated TTP field */
typedef struct
{
	ttp_info_t ttp_info;   ///< The TTP specific content
	emu_frame_t frames;    ///< The first frames in the superframe
} __attribute__((packed)) emu_ttp_t;


class Ttp
{
 public:
	/**
	 * @brief Terminal Time Plan contructor
	 */
	Ttp() {};

	~Ttp() {};

	/**
	 * @brief Parse TTP data
	 *
	 * @param data   The RAW data contining the TP
	 * @param length The data length
	 * @return true on success, false otherwise
	 */
	bool parse(const unsigned char *data, size_t length);

	/**
	 * @brief Add the new Time Plan entry
	 *
	 * @param frame_id         The frame ID
	 * @param tal_id           The terminal ID
	 * @param offset           The offset in the superframe
	 * @param assignment_count The number of assigned timeslots - 1
	 * @param fmt_id           The ID for FMT
	 * @param priority         The reaffic priority for this TP
	 *
	 * @return true on success, false othertwise
	 */
	bool addTimePlan(time_frame_t frame_id,
	                 tal_id_t tal_id,
	                 int32_t offset,
	                 uint16_t assignment_count,
	                 uint8_t fmt_id,
	                 uint8_t priority);

	/**
	 * @brief Clean the internal frames
	 */
	void reset();

	/**
	 * @brief Build the TTP
	 *
	 * @param superframe_nbr_sf The superframe number
	 * @param frame             The frame containing the TTP field
	 * @param length            The length of the frame
	 *
	 * @return true on success, false othertwise
	 */
	bool build(time_sf_t superframe_nbr_sf, unsigned char *frame, size_t &length);

	/**
	 * @brief Get the Time Plan for a terminal
	 *
	 * @param tal_id The terminal ID for which we want the TP
	 * @param tp     The Time Plans per superframe id
	 *
	 * @return true on success, false otherwise
	 */
	bool getTp(tal_id_t tal_id, map<uint8_t, emu_tp_t> &tp);

	time_sf_t getSuperframeCount() const
	{
		return this->superframe_count;
	};

 private:

	/// A list of time plans
	typedef vector<emu_tp_t> time_plans_t;
	/// The list of frames and their TP
	typedef map<uint8_t, time_plans_t> frames_t;

	/// The frames, completed each time we add a TP
	frames_t frames;
	/// The Time Plans per frame ID for a terminal ID
	map<tal_id_t, map<uint8_t, emu_tp_t> > tps;
	/// The superframe count
	time_sf_t superframe_count;
};

#endif


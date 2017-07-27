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
#include "OpenSandFrames.h"
#include "DvbFrame.h"

#include <opensand_output/OutputLog.h>

#include <map>
#include <vector>
#include <stdint.h>

using std::vector;
using std::map;

/** The information related to TTP */
typedef struct
{
	group_id_t group_id;       ///< The group ID 
	uint16_t superframe_count; ///< Superframe count to wich the TP applies
	// TODO we don't do one less
	uint8_t frame_loop_count;  ///< One less than the number of superframe
} __attribute__((packed)) ttp_info_t;

/** The information related to frame */
typedef struct
{
	uint8_t frame_number;            ///< The frame number within the superframe
	// TODO we don't do one less
	uint16_t tp_loop_count;          ///< one less than the number of the frame
	                                 //   TP entry loops that follow
	                                 //   nbr max loop = nbr max of terminals
} __attribute__((packed)) frame_info_t;

/** The emulated Time Plan */
typedef struct
{
	tal_id_t tal_id;   ///> The terminal ID (logon_id)
	                   //   size 5 for physical ST, 5->max for simulated ST requests
	int32_t offset;    ///> The offset in the superframe (start_slot for RCS)
	// TODO we don't do one less
	// TODO uint8_t in standard and we should build more than one TTP per ST
	uint16_t assignment_count; ///> one less than the number of timeslots assigned
	                           //   in the block (for RCS)
	uint8_t fmt_id;    ///> The ID for FMT (MODCOD ID)
	uint8_t priority;  ///> The traffic priority (no used in RCS)
} __attribute__((packed)) emu_tp_t;

/** The emulated frame */
typedef struct
{
	frame_info_t frame_info;  ///< the frame specific content
	emu_tp_t tp[0];           ///< The first Time Plans in the frame
	                          //   max nbr of terminals = broadcast tal_id
} __attribute__((packed)) emu_frame_t;

/** The emulated TTP field */
typedef struct
{
	ttp_info_t ttp_info;    ///< The TTP specific content
	emu_frame_t frames[0];  ///< The first frames in the superframe
} __attribute__((packed)) emu_ttp_t;

/**
 * Time Burst Time plan, essentially A basic DVB Header
 * followed by an array descriptor of frame structures
 */
typedef struct
{
	T_DVB_HDR hdr;  ///< Basic DVB Header
	emu_ttp_t ttp;  ///< The emulated TTP
} __attribute__((packed)) T_DVB_TTP;


class Ttp: public DvbFrameTpl<T_DVB_TTP>
{
 public:
	/**
	 * @brief Terminal Time Plan contructor
	 */
	Ttp();

	/**
	 * @brief Terminal Time Plan contructor
	 *
	 * @param group_id  The group ID
	 * @param sf_id     The superframe ID
	 */
	Ttp(group_id_t group_id, time_sf_t sf_id);


	~Ttp() {};

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
	                 fmt_id_t fmt_id,
	                 uint8_t priority);

	/**
	 * @brief Clean the internal frames
	 */
	void reset();

	/**
	 * @brief Build the TTP
	 *
	 * @return true on success, false othertwise
	 */
	bool build(void);

	/**
	 * @brief Get the Time Plan for a terminal
	 *
	 * @param tal_id The terminal ID for which we want the TP
	 * @param tp     The Time Plans per superframe id
	 *
	 * @return true if a TP is found, false otherwise
	 */
	bool getTp(tal_id_t tal_id, map<uint8_t, emu_tp_t> &tps);

	/**
	 * @brief  Get the group Id
	 *
	 * @return the group ID
	 */
	group_id_t getGroupId() const
	{
		return this->frame()->ttp.ttp_info.group_id;
	};

	/**
	 * @brief  Get the superframe count to which the TP applies
	 *
	 * @return the superframe count
	 */
	time_sf_t getSuperframeCount() const
	{
		return ntohs(this->frame()->ttp.ttp_info.superframe_count);
	};

	/// The log for TTP
	static OutputLog *ttp_log;

 private:

	/// A list of time plans
	typedef vector<emu_tp_t> time_plans_t;
	/// The list of frames and their TP
	typedef map<uint8_t, time_plans_t> frames_t;

	/// The frames, completed each time we add a TP
	frames_t frames;
};

#endif


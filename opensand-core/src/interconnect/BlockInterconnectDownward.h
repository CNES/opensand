/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file BlockInterconnectDownwards.h
 * @brief This bloc implements an interconnection block facing downwards.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef BlockInterconnectDownward_H
#define BlockInterconnectDownward_H

#include <opensand_rt/Rt.h>
#include "interconnect_channel.h"
#include "OpenSandFrames.h"
#include "DvbFrame.h"

struct icd_specific
{
	uint16_t port_upward; // TCP port for the upward channel
	uint16_t port_downward; // TCP port for the downward channel
};

/**
 * @class BlockInterconnectDownward
 * @brief This block implements an interconnection block facing downwards.
 */
template <class T = DvbFrame>
class BlockInterconnectDownwardTpl: public Block
{
 public:

	/**
	 * @brief The interconnect block, placed below
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	BlockInterconnectDownwardTpl(const string &name,
	                             struct icd_specific specific);

	~BlockInterconnectDownwardTpl();

	template <class O = T>
	class UpwardTpl: public RtUpward
	{
	 public:
		UpwardTpl(const string &name, struct icd_specific specific):
			RtUpward(name),
			port(specific.port_upward),
			in_channel(true,false)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the port of the socket created by this block
	uint16_t port;
	/// TCP in channel
	interconnect_channel in_channel;
	// Output log
	OutputLog *log_interconnect;
	// the signal event
	int32_t socket_event;
	// the signal event
	int32_t timer_event;
	};
	 typedef UpwardTpl<> Upward;

	template <class O = T>
	class DownwardTpl: public RtDownward
	{
	 public:
		DownwardTpl(const string &name, struct icd_specific specific):
			RtDownward(name),
			port(specific.port_downward),
			out_channel(false,true)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the port of the socket created by the Block above
		uint16_t port;
		/// TCP out channel
		interconnect_channel out_channel;
		// Output log
		OutputLog *log_interconnect;
		// the timer event
		int32_t timer_event;
	};
	typedef DownwardTpl<> Downward;

 protected:
	// Output log
	OutputLog *log_interconnect;

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();
};

#include "BlockInterconnectDownward.cpp"

typedef BlockInterconnectDownwardTpl<> BlockInterconnectDownward;

#endif

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
 * @file BlockInterconnect.h
 * @brief This file describes two blocks interconnected via an InterconnectChannel.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef BlockInterconnect_H
#define BlockInterconnect_H

#include "InterconnectChannel.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>
#include <opensand_rt/RtChannel.h>

#include <list>
#include <signal.h>
#include <unistd.h>

struct InterconnectConfig
{
	std::string interconnect_addr; // Interconnect interface IP address
	uint32_t delay;
	std::size_t isl_index;
};

/**
 * @class BlockInterconnectDownward
 * @brief This block implements an interconnection block facing downwards.
 */
class BlockInterconnectDownward: public Block
{
public:
	/**
	 * @brief The interconnect block, placed below
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	BlockInterconnectDownward(const std::string &name,
	                          const InterconnectConfig &config);

	class Upward: public RtUpward, public InterconnectChannelReceiver
	{
	public:
		Upward(const std::string &name, const InterconnectConfig &config);
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	private:
		std::size_t isl_index;
	};

	class Downward: public RtDownward, public InterconnectChannelSender
	{
	public:
		Downward(const std::string &name, const InterconnectConfig &config);
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	private:
		event_id_t delay_timer;
		uint32_t polling_rate;
		std::size_t isl_index;
	};

protected:
	// Output log
	std::shared_ptr<OutputLog> log_interconnect;

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();
};

/**
 * @class BlockInterconnectUpward
 * @brief This bloc implements an interconnection block facing upwards
 */
class BlockInterconnectUpward: public Block
{
public:
	/**
	 * @brief The interconnect block, placed below
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	BlockInterconnectUpward(const std::string &name,
	                        const InterconnectConfig &config);

	class Upward: public RtUpward, public InterconnectChannelSender
	{
	public:
		Upward(const std::string &name, const InterconnectConfig &config);
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	private:
		event_id_t delay_timer;
		uint32_t polling_rate;
		std::size_t isl_index;
	};

	class Downward: public RtDownward, public InterconnectChannelReceiver
	{
	public:
		Downward(const std::string &name, const InterconnectConfig &config);
		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	private:
		std::size_t isl_index;
	};

protected:
	// Output log
	std::shared_ptr<OutputLog> log_interconnect;

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();
};

#endif

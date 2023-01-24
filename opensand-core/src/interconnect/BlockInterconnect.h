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

#include <list>

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>

#include "OpenSandCore.h"
#include "InterconnectChannel.h"


class Output;


struct InterconnectConfig
{
	std::string interconnect_addr; // Interconnect interface IP address
	uint32_t delay;
	std::size_t isl_index;
};


template<>
class Rt::UpwardChannel<class BlockInterconnectDownward>: public Channels::Upward<UpwardChannel<BlockInterconnectDownward>>, public InterconnectChannelReceiver
{
 public:
	UpwardChannel(const std::string &name, const InterconnectConfig &config);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const NetSocketEvent& event) override;

 private:
	std::size_t isl_index;
};


template<>
class Rt::DownwardChannel<class BlockInterconnectDownward>: public Channels::Downward<DownwardChannel<BlockInterconnectDownward>>, public InterconnectChannelSender
{
 public:
	DownwardChannel(const std::string &name, const InterconnectConfig &config);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const TimerEvent& event) override;
	bool onEvent(const MessageEvent& event) override;

 private:
	event_id_t delay_timer;
	uint32_t polling_rate;
	std::size_t isl_index;
};


/**
 * @class BlockInterconnectDownward
 * @brief This block implements an interconnection block facing downwards.
 */
class BlockInterconnectDownward: public Rt::Block<BlockInterconnectDownward, const InterconnectConfig&>
{
 public:
	using Rt::Block<BlockInterconnectDownward, const InterconnectConfig&>::Block;

 protected:
	// Output log
	std::shared_ptr<OutputLog> log_interconnect;

	// initialization method
	bool onInit() override;
};


template<>
class Rt::UpwardChannel<class BlockInterconnectUpward>: public Channels::Upward<UpwardChannel<BlockInterconnectUpward>>, public InterconnectChannelSender
{
 public:
	UpwardChannel(const std::string &name, const InterconnectConfig &config);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const TimerEvent &event) override;
	bool onEvent(const MessageEvent &event) override;

 private:
	event_id_t delay_timer;
	uint32_t polling_rate;
	std::size_t isl_index;
};


template<>
class Rt::DownwardChannel<class BlockInterconnectUpward>: public Channels::Downward<DownwardChannel<BlockInterconnectUpward>>, public InterconnectChannelReceiver
{
 public:
	DownwardChannel(const std::string &name, const InterconnectConfig &config);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const NetSocketEvent &event) override;

 private:
	std::size_t isl_index;
};


/**
 * @class BlockInterconnectUpward
 * @brief This bloc implements an interconnection block facing upwards
 */
class BlockInterconnectUpward: public Rt::Block<BlockInterconnectUpward, const InterconnectConfig &>
{
public:
	using Rt::Block<BlockInterconnectUpward, const InterconnectConfig &>::Block;

protected:
	// Output log
	std::shared_ptr<OutputLog> log_interconnect;

	// initialization method
	bool onInit() override;
};

#endif

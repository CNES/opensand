/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file TestBlock.h
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Aurelien Delrieu <adelrieu@toulouse.viveris.com>
 * @brief This test check that we can raise a timer on a channel then
 *        write on a socket that will be monitored by the opposite channel
 */

#ifndef TEST_BLOCK_H
#define TEST_BLOCK_H

#include "Block.h"
#include "RtChannel.h"

#include <utility>


template<>
class Rt::UpwardChannel<class TestBlock>: public Rt::Channels::Upward<Rt::UpwardChannel<TestBlock>>
{
 public:
	UpwardChannel (const std::string& name);
	~UpwardChannel ();

	void setOutputFd (int32_t fd);

	bool onInit () override;

	using Rt::ChannelBase::onEvent;
	bool onEvent (const Rt::Event& event) override;
	bool onEvent (const Rt::TimerEvent& event) override;
	bool onEvent (const Rt::MessageEvent& event) override;

 protected:
	uint32_t nbr_timeouts;
	int32_t output_fd;

	/// the data written by timer that should be read on socket
	std::string last_written;
};


template<>
class Rt::DownwardChannel<class TestBlock>: public Rt::Channels::Downward<Rt::DownwardChannel<TestBlock>>
{
 public:
	DownwardChannel (const std::string& name);
	~DownwardChannel ();

	void setInputFd (int32_t fd);

	bool onInit () override;

	using Rt::ChannelBase::onEvent;
	bool onEvent(const Rt::Event& event) override;
	bool onEvent(const Rt::FileEvent& event) override;

 protected:
	int32_t input_fd;
};


class TestBlock: public Rt::Block<TestBlock>
{
  public:
	TestBlock(const std::string &name);
	~TestBlock();

  protected:
	bool onInit() override;
};


#endif

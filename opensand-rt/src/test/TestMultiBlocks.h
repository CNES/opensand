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
 * @file TestMultiBlocks.h
 * @author Cyrille Gaillardet <cgaillardet@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Aurelien Delrieu <adelrieu@toulouse.viveris.com>
 * @brief This test check that we can read a file on a channel then
 *        transmit content to lower block, the bottom block transmit it
 *        to the following channel that will forward it to the top and
 *        compare output
 */


#ifndef TEST_MULTI_BLOCK_H
#define TEST_MULTI_BLOCK_H


#include "Data.h"
#include "Block.h"
#include "RtChannel.h"

#include <queue>


template<>
class Rt::UpwardChannel<class TopBlock>: public Channels::Upward<UpwardChannel<TopBlock>>
{
 public:
	UpwardChannel(const std::string& name, std::string file);

	using ChannelBase::onEvent;
	bool onEvent(const MessageEvent& event) override;
};


template<>
class Rt::DownwardChannel<class TopBlock>: public Channels::Downward<DownwardChannel<TopBlock>>
{
 public:
	DownwardChannel(const std::string& name, std::string file);
	~DownwardChannel();

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const FileEvent& event) override;
	bool onEvent(const MessageEvent& event) override;

 protected:
	std::string input_file;
	int32_t input_fd;
	std::queue<Data> last_written;
};


class TopBlock: public Rt::Block<TopBlock, std::string>
{
  public:
	TopBlock(const std::string &name, std::string file);
};


template<>
class Rt::UpwardChannel<class MiddleBlock>: public Channels::Upward<UpwardChannel<MiddleBlock>>
{
 public:
	UpwardChannel(const std::string& name);

	using ChannelBase::onEvent;
	bool onEvent(const MessageEvent& event) override;
};


template<>
class Rt::DownwardChannel<class MiddleBlock>: public Channels::Downward<DownwardChannel<MiddleBlock>>
{
 public:
	DownwardChannel(const std::string& name);

	using ChannelBase::onEvent;
	bool onEvent(const MessageEvent& event) override;
};


class MiddleBlock: public Rt::Block<MiddleBlock>
{
  public:
	MiddleBlock(const std::string &name);
};


template<>
class Rt::UpwardChannel<class BottomBlock>: public Channels::Upward<UpwardChannel<BottomBlock>>
{
 public:
	UpwardChannel(const std::string& name);
	~UpwardChannel();

	void setInputFd(int32_t fd);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const NetSocketEvent& event) override;

 protected:
	int32_t input_fd;
};


template<>
class Rt::DownwardChannel<class BottomBlock>: public Channels::Downward<DownwardChannel<BottomBlock>>
{
 public:
	DownwardChannel(const std::string& name);
	~DownwardChannel();

	void setOutputFd(int32_t fd);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const MessageEvent& event) override;

 protected:
	int32_t output_fd;
};


class BottomBlock: public Rt::Block<BottomBlock>
{
  public:
	BottomBlock(const std::string &name);

  protected:
	bool onInit(void) override;
};


#endif

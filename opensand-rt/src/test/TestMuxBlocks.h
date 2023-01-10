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
 * @file TestMuxBlocks.h
 * @brief
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#ifndef TESTMUXBLOCKS_H
#define TESTMUXBLOCKS_H

#include "Block.h"
#include "RtChannelMux.h"
#include "RtChannelDemux.h"
#include "RtChannelMuxDemux.h"


enum struct Side
{
	LEFT,
	RIGHT
};


template<>
class Rt::UpwardChannel<class TopMux>: public Rt::Channels::UpwardMux<Rt::UpwardChannel<TopMux>>
{
 public:
	UpwardChannel(const std::string& name);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);
};


template<>
class Rt::DownwardChannel<class TopMux>: public Rt::Channels::DownwardDemux<Rt::DownwardChannel<TopMux>, Side>
{
 public:
	DownwardChannel(const std::string& name);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);
};


class TopMux : public Rt::Block<TopMux>
{
  public:
	using Rt::Block<TopMux>::Block;
};


template<>
class Rt::UpwardChannel<class MiddleBlock>: public Rt::Channels::UpwardMuxDemux<Rt::UpwardChannel<MiddleBlock>, Side>
{
 public:
	UpwardChannel(const std::string& name, Side side);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);

 protected:
	Side side;
};


template<>
class Rt::DownwardChannel<class MiddleBlock>: public Rt::Channels::DownwardMuxDemux<Rt::DownwardChannel<MiddleBlock>, Side>
{
 public:
	DownwardChannel(const std::string& name, Side side);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);

 protected:
	Side side;
};


class MiddleBlock: public Rt::Block<MiddleBlock, Side>
{
  public:
	using Rt::Block<MiddleBlock, Side>::Block;
};


template<>
class Rt::UpwardChannel<class TopBlock>: public Rt::Channels::UpwardMux<Rt::UpwardChannel<TopBlock>>
{
 public:
	UpwardChannel(const std::string& name, Side side);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);

 protected:
	Side side;
};


template<>
class Rt::DownwardChannel<class TopBlock>: public Rt::Channels::DownwardDemux<Rt::DownwardChannel<TopBlock>, Side>
{
 public:
	DownwardChannel(const std::string& name, Side side);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);

 protected:
	Side side;
};


class TopBlock: public Rt::Block<TopBlock, Side>
{
  public:
	using Rt::Block<TopBlock, Side>::Block;
};


template<>
class Rt::UpwardChannel<class BottomBlock>: public Rt::Channels::UpwardDemux<Rt::UpwardChannel<BottomBlock>, Side>
{
 public:
	UpwardChannel(const std::string& name, Side side);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);

 protected:
	Side side;
};


template<>
class Rt::DownwardChannel<class BottomBlock>: public Rt::Channels::DownwardMux<Rt::DownwardChannel<BottomBlock>>
{
 public:
	DownwardChannel(const std::string& name, Side side);

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::MessageEvent& event);

 protected:
	Side side;
};


class BottomBlock: public Rt::Block<BottomBlock, Side>
{
  public:
	using Rt::Block<BottomBlock, Side>::Block;
};


template<>
class Rt::UpwardChannel<class BottomMux>: public Rt::Channels::UpwardDemux<Rt::UpwardChannel<BottomMux>, Side>
{
 public:
	UpwardChannel(const std::string& name);

	bool onInit() override;

	bool onEvent(const Rt::Event& event);
};


template<>
class Rt::DownwardChannel<class BottomMux>: public Rt::Channels::DownwardMux<Rt::DownwardChannel<BottomMux>>
{
 public:
	DownwardChannel(const std::string& name);

	bool onInit() override;

	bool onEvent(const Rt::Event& event);
	bool onEvent(const Rt::TimerEvent& event);
	bool onEvent(const Rt::MessageEvent& event);
};


class BottomMux: public Rt::Block<BottomMux>
{
  public:
	using Rt::Block<BottomMux>::Block;
};


#endif

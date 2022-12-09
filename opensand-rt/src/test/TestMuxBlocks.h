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

class TopMux : public Block
{
  public:
	using Block::Block;

	class Upward: public RtUpwardMux
	{
	  public:
		Upward(const std::string &name);
		bool onEvent(const RtEvent *const event);
	};

	class Downward: public RtDownwardDemux<Side>
	{
	  public:
		Downward(const std::string &name);
		bool onEvent(const RtEvent *const event);
	};
};

class MiddleBlock: public Block
{
  public:
	MiddleBlock(const std::string &name, Side side);

	class Upward: public RtUpwardMuxDemux<Side>
	{
	  public:
		Upward(const std::string &name, Side side);
		bool onEvent(const RtEvent *const event);
		Side side;
	};

	class Downward: public RtDownwardMuxDemux<Side>
	{
	  public:
		Downward(const std::string &name, Side side);
		bool onEvent(const RtEvent *const event);
		Side side;
	};
};

class TopBlock: public Block
{
  public:
	TopBlock(const std::string &name, Side side);

	class Upward: public RtUpwardMux
	{
	  public:
		Upward(const std::string &name, Side side);
		bool onEvent(const RtEvent *const event) override;
		Side side;
	};

	class Downward: public RtDownwardDemux<Side>
	{
	  public:
		Downward(const std::string &name, Side side);
		bool onEvent(const RtEvent *const event) override;
		Side side;
	};
};

class BottomBlock: public Block
{
  public:
	BottomBlock(const std::string &name, Side side);

	class Upward: public RtUpwardDemux<Side>
	{
	  public:
		Upward(const std::string &name, Side side);
		bool onEvent(const RtEvent *const event) override;
		Side side;
	};

	class Downward: public RtDownwardMux
	{
	  public:
		Downward(const std::string &name, Side side);
		bool onEvent(const RtEvent *const event) override;
		Side side;
	};
};

class BottomMux: public Block
{
  public:
	using Block::Block;

	class Upward: public RtUpwardDemux<Side>
	{
	  public:
		Upward(const std::string &name);
		bool onInit() override;
		bool onEvent(const RtEvent *const event) override;
	};

	class Downward: public RtDownwardMux
	{
	  public:
		Downward(const std::string &name);
		bool onInit() override;
		bool onEvent(const RtEvent *const event) override;
	};
};

#endif

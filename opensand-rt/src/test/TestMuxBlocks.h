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
 * @author Yohan Simard <yohan.simard@viveris.com>
 */

#ifndef TESTMUXBLOCKS_H
#define TESTMUXBLOCKS_H

#include "Block.h"

enum struct Side
{
	LEFT,
	RIGHT
};

class DownMuxBlock : public Block
{
  public:
	using Block::Block;
	bool onInit();

	class Upward: public RtUpwardMux
	{
	  public:
		Upward(const std::string &name);
		bool onInit();
		bool onEvent(const RtEvent *const event);
	};

	class Downward: public RtDownwardDemux<Side>
	{
	  public:
		Downward(const std::string &name);
		bool onInit();
		bool onEvent(const RtEvent *const event);
	};
};

class SimpleBlock: public Block
{
  public:
	SimpleBlock(const std::string &name, bool send_msg);
	bool onInit();

	class Upward: public RtUpward
	{
	  public:
		Upward(const std::string &name, bool send_msg);
		bool onInit();
		bool onEvent(const RtEvent *const event);
		bool send_msg;
	};

	class Downward: public RtDownward
	{
	  public:
		Downward(const std::string &name, bool send_msg);
		bool onInit();
		bool onEvent(const RtEvent *const event);
		bool send_msg;
	};
};

#endif
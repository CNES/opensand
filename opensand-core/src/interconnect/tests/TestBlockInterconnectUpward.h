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
 * @file BlockInterconnectUpward.h
 * @brief This bloc implements an interconnection block facing upwards.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef BlockInterconnectUpward_H
#define BlockInterconnectUpward_H

#include <opensand_rt/Rt.h>
#include "interconnect_channel_test.h"
#include "Config.h"

struct icu_specific
{
	string ip_addr; // IP of the remote BlockInterconnect
    uint16_t port_upward; // TCP port for the upward channel
    uint16_t port_downward; // TCP port for the downward channel
};

/**
 * @class BlockInterconnectUpward
 * @brief This bloc implements an interconnection block facing upwards
 */
class TestBlockInterconnectUpward: public Block
{
 public:

	/**
	 * @brief The interconnect block, placed below
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	TestBlockInterconnectUpward(const string &name,
	                        struct icu_specific specific);

	~TestBlockInterconnectUpward();

	class Upward: public RtUpward
	{
	 public:
		Upward(Block *const bl, struct icu_specific specific):
			RtUpward(bl),
			ip_addr(specific.ip_addr),
            port(specific.port_upward),
            out_channel(false,true)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the IP address of the remote BlockInterconnect
		string ip_addr;
		/// the port of the socket created by the Block above
        uint16_t port;
        /// TCP out channel
        interconnect_channel_test out_channel;

	};

	class Downward: public RtDownward
	{
	 public:
		Downward(Block *const bl, struct icu_specific specific):
			RtDownward(bl),
			ip_addr(specific.ip_addr),
            port(specific.port_downward),
            in_channel(true,false),
            last_t(0)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the IP address of the remote BlockInterconnect
		string ip_addr;
		/// the port of the socket created by the Block above
        uint16_t port;
        /// TCP in channel
        interconnect_channel_test in_channel;
        /// last message counter
        unsigned short last_t;
	};

 protected:

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();
};

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file sat_carrier_channel_set.h
 * @brief This implements a set of satellite carrier channels
 * @author AQL (Antoine)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef SAT_CARRIER_CHANNEL_SET_H
#define SAT_CARRIER_CHANNEL_SET_H

#include <vector>
#include <net/if.h>

#include "sat_carrier_channel.h"
#include "sat_carrier_udp_channel.h"
#include "opensand_conf/conf.h"
#include "OpenSandCore.h"

/**
 * @class sat_carrier_channel_set
 * @brief This implements a set of satellite carrier channels
 */
class sat_carrier_channel_set: public std::vector < sat_carrier_channel * >
{
 public:

	sat_carrier_channel_set();
	~sat_carrier_channel_set();

	int readConfig(component_t host,
	               const string local_ip_addr,
	               const string interface_name);

	int send(unsigned int i_carrier, unsigned char *ip_buf,
	         unsigned int i_len);

	int receive(int fd, unsigned int *op_carrier, unsigned char *op_buf,
	            unsigned int *op_len, unsigned int op_max_len, long timeout_ms);

	int getChannelFdByChannelId(unsigned int i_channelID);

	unsigned int getNbChannel();

 private:
	// type of the socket (udp or ethernet)
	std::string socket_type;
};

#endif

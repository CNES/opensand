/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file sat_carrier_channel.cpp
 * @brief This implements a bloc sat carrier channel
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_SAT_CARRIER
#include <opensand_conf/uti_debug.h>

#include "sat_carrier_channel.h"

#include <netinet/in.h>
#include <cstring>


/**
 * Constructor
 * @param channelID the ID of the new channel
 * @param input     true if the channel accept input
 * @param output    true if channel accept output
 * @param is_data   true is this is an intput data channel
 */
sat_carrier_channel::sat_carrier_channel(unsigned int channelID,
                                         bool input,
                                         bool output,
                                         bool is_data):
	m_channelID(channelID),
	m_input(input),
	m_output(output),
	is_data(is_data),
	init_success(false)
{
}

/**
 * Destructor
 */
sat_carrier_channel::~sat_carrier_channel()
{
}

/**
 * Check if the channel was correctly created
 */
bool sat_carrier_channel::isInit()
{
	return this->init_success;
}

/**
 * Get the ID of the channel
 * @return the channel ID
 */
unsigned int sat_carrier_channel::getChannelID()
{
	return (m_channelID);
}

/**
 * Get if the channel accept input
 * @return true if channel accept input
 */
bool sat_carrier_channel::isInputOk()
{
	return (m_input);
}

/**
 * Get if the channel accept output
 * @return true if channel accept output
 */
bool sat_carrier_channel::isOutputOk()
{
	return (m_output);
}

/**
 * Get the index of a network interface
 * @param name the name of the interface
 * @return the index of the interface if successful, -1 otherwise
 */
int sat_carrier_channel::getIfIndex(const char *name)
{
	const char FUNCNAME[] = "[sat_carrier_channel::getIfIndex]";
	int sock;
	ifreq ifr;
	int index = -1;

	// open the network interface socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(sock < 0)
	{
		UTI_ERROR("%s cannot create an INET socket: %s (%d)\n", FUNCNAME,
		strerror(errno), errno);
		goto exit;
	}

	// get the network interface index
	bzero(&ifr, sizeof(ifreq));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
	if(ioctl(sock, SIOGIFINDEX, &ifr) < 0)
	{
		UTI_ERROR("%s cannot get the network interface index: %s (%d)\n",
			   FUNCNAME, strerror(errno), errno);
		goto close;
	}

	index = ifr.ifr_ifindex;

close:
	close(sock);
exit:
	return index;
}

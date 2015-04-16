/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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


#include "sat_carrier_udp_channel.h"
#include "OpenSandCore.h"
#include "OpenSandConf.h"

#include <opensand_conf/conf.h>

#include <vector>
#include <net/if.h>

/**
 * @class sat_carrier_channel_set
 * @brief This implements a set of satellite carrier channels
 */
// TODO why not a map<fd, carrier>?
class sat_carrier_channel_set: public std::vector < sat_carrier_udp_channel * >
{
 public:

	sat_carrier_channel_set(tal_id_t tal_id);
	~sat_carrier_channel_set();

	/**
	 * Read data from the configuration file and create input channels
	 *
	 * @param local_ip_addr   The IP address for emulation network
	 * @param interface_name  The interface name for emulation network
	 * @return true on success, false otherwise
	 */
	bool readInConfig(const string local_ip_addr,
	                  const string interface_name);

	/**
	 * Read data from the configuration file and create output channels
	 *
	 * @param local_ip_addr   The IP address for emulation network
	 * @param interface_name  The interface name for emulation network
	 * @return true on success, false otherwise
	 */
	bool readOutConfig(const string local_ip_addr,
	                   const string interface_name);

	/**
	 * @brief Send data on a satellite carrier
	 *
	 * @param carrier_id  The satellite carrier ID
	 * @param data        The data to send
	 * @param length      The liength of the data
	 * @return true on success, false otherwise
	 */
	bool send(uint8_t carrier_id, const unsigned char *data, size_t length);

	/**
	* @brief Receive data on a channel set
	*
	* The function works in blocking mode, so call it only when you are sure
	* some data is ready to be received.
	*
	* @param event         The event on channel fd
	* @param op_carrier    Satellite Carrier id
	* @param op_buf        pointer to a char buffer
	* @param op_len        the received data length
	* @return  0 on success, 1 if the function should be
	 *         called another time, -1 on error
	*/
	int receive(NetSocketEvent *const event,
	            unsigned int &op_carrier,
	            spot_id_t &op_spot,
	            unsigned char **op_buf, size_t &op_len);

	int getChannelFdByChannelId(unsigned int i_channelID);

	unsigned int getNbChannel();

 private:

	/**
	 * Read data from the configuration file and create channels
	 *
	 * @param local_ip_addr   The IP address for emulation network
	 * @param interface_name  The interface name for emulation network
	 * @param in              Whether we want input or output channels
	 * @return true on success, false otherwise
	 */
	bool readConfig(const string local_ip_addr,
	                const string interface_name,
	                bool in);

	/// The terminal ID
	tal_id_t tal_id;

	// Output Log
	OutputLog *log_init;
	OutputLog *log_sat_carrier;
};

#endif

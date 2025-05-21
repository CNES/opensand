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

#include "OpenSandCore.h"
#include "OpenSandModelConf.h"
#include "UdpChannel.h"


/**
 * @class sat_carrier_channel_set
 * @brief This implements a set of satellite carrier channels
 */
// TODO why not a map<fd, carrier>?
class sat_carrier_channel_set: public std::vector<std::unique_ptr<UdpChannel>>
{
public:
	sat_carrier_channel_set(tal_id_t tal_id);

	/**
	 * Read data from the configuration file and create input channels
	 *
	 * @param local_ip_addr     The IP address for emulation network
	 * @param destination_host  For sat only: destination handled by this part of the stack (terminal or gateway)
	 * @param spot_id           For sat only: the spot handled by this part of the stack
	 * @return true on success, false otherwise
	 */
	bool readInConfig(const std::string local_ip_addr,
	                  Component destination_host,
	                  spot_id_t spot_id);

	/**
	 * Read data from the configuration file and create output channels
	 *
	 * @param local_ip_addr   The IP address for emulation network
	 * @param destination_host  For sat only: destination handled by this part of the stack (terminal or gateway)
	 * @param spot_id           For sat only: the spot handled by this part of the stack
	 * @return true on success, false otherwise
	 */
	bool readOutConfig(const std::string local_ip_addr,
	                   Component destination_host,
	                   spot_id_t spot_id);

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
	UdpChannel::ReceiveStatus receive(const Rt::NetSocketEvent& event,
	            unsigned int &op_carrier,
	            spot_id_t &op_spot,
				Rt::Ptr<Rt::Data>& op_buf);
	/*
	            unsigned char **op_buf,
	            std::size_t &op_len);
	*/

	int getChannelFdByChannelId(unsigned int i_channelID);

	unsigned int getNbChannel();

private:
	/**
	 * Read data from the configuration file and create channels
	 *
	 * @param local_ip_addr   The IP address for emulation network
	 * @param destination_host  For sat only: destination handled by this part of the stack (terminal or gateway)
	 * @param spot_id           For sat only: the spot handled by this part of the stack
	 * @param in              Whether we want input or output channels
	 * @return true on success, false otherwise
	 */
	bool readConfig(const std::string local_ip_addr,
	                Component destination_host,
	                spot_id_t spot_id,
	                bool in);
	bool readSpot(const std::string &local_ip_addr,
	              bool in,
	              Component host,
	              tal_id_t gw_id,
	              bool is_satellite);
	bool readCarrier(const std::string &local_ip_addr,
	                 tal_id_t gw_id,
	                 const OpenSandModelConf::carrier_socket &carrier,
	                 bool is_input);

	/// The terminal ID
	tal_id_t tal_id;

	// Output Log
	std::shared_ptr<OutputLog> log_init;
	std::shared_ptr<OutputLog> log_sat_carrier;
};

#endif

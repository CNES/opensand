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
 * @file sat_carrier_udp_channel.h
 * @brief This implements an udp satellite carrier channel
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 */

#ifndef SAT_CARRIER_UDP_CHANNEL_H
#define SAT_CARRIER_UDP_CHANNEL_H

// system includes
#include <net/ethernet.h>
#include <sys/types.h>

// opensand includes
#include "sat_carrier_channel.h"

/*
 * @class sat_carrier_udp_channel
 * @brief UDP satellite carrier channel
 */
class sat_carrier_udp_channel: public sat_carrier_channel
{
 public:

	sat_carrier_udp_channel(unsigned int channelID,
	                        bool input, bool output,
	                        const string local_interface_name,
	                        unsigned short port,
	                        bool multicast,
	                        const string local_ip_addr,
	                        const string ip_addr);

	~sat_carrier_udp_channel();

	int getChannelFd();

	//unsigned char *getRemoteIPAddress();

	int send(unsigned char *buf, unsigned int len);

	int receive(unsigned char *buf, unsigned int *data_len,
	            unsigned int max_len, long timeout);

 protected:

	/// the socket which defines the channel
	int sock_channel;

	/// boolean which indicates if the interface is up
	bool sock_ok;

	/// address of the channel
	struct sockaddr_in m_socketAddr;

	/// the remote IP address of the channel
	struct sockaddr_in m_remoteIPAddress;

	/// boolean which indicates if the channel is multicast
	bool m_multicast;

	/// A map whose key is an IP address and value is a counter
	/// (counter ranges from 0 to 255)
	typedef map<std::string , uint8_t> ip_to_counter_map;

	/// (IP address, counter) map used to check that UDP packets are received in
	/// sequence on every UDP communication channel
	ip_to_counter_map counterMap;

	/// Counter for sending packets
	uint8_t counter;

	/// buffer to receive udp datagramms
	unsigned char recv_buffer[9000];

	/// internal buffer to build and send udp datagramms
	unsigned char send_buffer[9000];

	/// sometimes an UDP datagram containing unfragmented IP packet overtake one
	/// containing fragmented IP packets during its reassembly
	/// Thus, we use the following stack to keep the UDP datagram arrived too early
	unsigned char stack[9000];

	/// the length of the data in the stack
	unsigned int stack_len;

	/// the sequence number of the packet in the stack
	uint8_t stack_sequ;

	/// whether the content of the stack should be returned
	bool send_stack;
};

#endif

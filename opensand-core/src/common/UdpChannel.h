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
 * @file UdpChannel.h
 * @brief This implements an udp satellite carrier channel
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 */

#ifndef SAT_CARRIER_UDP_CHANNEL_H
#define SAT_CARRIER_UDP_CHANNEL_H


#include <netinet/in.h>

#include <map>
#include <string>
#include <memory>
#include <vector>

#include "OpenSandCore.h"
#include <opensand_rt/Data.h>
#include <opensand_rt/Types.h>


class UdpStack;
class OutputLog;
namespace Rt { class NetSocketEvent; };


/*
 * @class UdpChannel
 * @brief UDP satellite carrier channel
 */
class UdpChannel
{
public:
	UdpChannel(std::string name,
	           spot_id_t s_id,
	           unsigned int channel_id,
	           bool input, bool output,
	           unsigned short port,
	           bool multicast,
	           const std::string local_ip_addr,
	           const std::string ip_addr,
	           unsigned int stack,
	           unsigned int rmem,
	           unsigned int wmem);

	~UdpChannel();

	bool isInit();

	unsigned int getChannelID();

	bool isInputOk();

	bool isOutputOk();

	/**
	 * @brief Send data on the satellite carrier
	 *
	 * @param data        The data to send
	 * @param length      The length of the data
	 * @return true on success, false otherwise
	 */
	bool send(const unsigned char *data, std::size_t length);
	int receive(const Rt::NetSocketEvent& event, Rt::Ptr<Rt::Data> &buf);

	int getChannelFd();
	
	spot_id_t getSpotId();

	/**
	 * @brief Get the next stacked packet
	 *
	 * @param buf      OUT: the stacked packet
	 * @return true on success, false otherwise
	 */
	bool handleStack(Rt::Ptr<Rt::Data> &buf);


	/**
	 * @brief Get the next stacked packet
	 *
	 * @param buf      OUT: the stacked packet
	 * @param counter  The position in the stack at
	 *                 which the packet is located
	 * @param stack    The stack in which to get packet
	 */
	void handleStack(Rt::Ptr<Rt::Data> &buf,
	                 uint8_t counter, UdpStack &stack);

protected:
	/// the spot id
	spot_id_t spot_id;

	/// the ID of the channel
	int m_channel_id;

	/// if channel accept input
	bool m_input;

	/// if channel accept output
	bool m_output;

	/// is the channel well initialized
	bool init_success;

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
	/// (IP address, counter) map used to check that UDP packets are received in
	/// sequence on every UDP communication channel
	std::map<std::string , uint8_t> udp_counters;

	/// Counter for sending packets
	uint8_t counter;

	/// internal buffer to build and send udp datagramms
	unsigned char send_buffer[MAX_SOCK_SIZE];

	/// sometimes an UDP datagram containing unfragmented IP packet overtake one
	/// containing fragmented IP packets during its reassembly
	/// Thus, we use the stacks per IP sources to keep the UDP datagram arrived too early
	std::map<std::string, UdpStack> stacks;

	/// the IP address of the stack for which we need to send a packet or
	//  empty string if we have nothing to send
	std::string stacked_ip;

	/// The maximum number of packets buffered in the software stack before sending content
	unsigned int max_stack;

	/// Output Log
	std::shared_ptr<OutputLog> log_sat_carrier;
	std::shared_ptr<OutputLog> log_init;
};

/*
 * @class The UDP stack
 * @brief This stack allows UDP packets ordering in order to avoid
 *        sequence desynchronizations
 */
class UdpStack: std::vector<Rt::Ptr<Rt::Data>>
{
public:
	/**
	 * @brief Create the stack
	 *
	 */
	UdpStack();

	~UdpStack();

	/**
	 * @brief Add a packet in the stack
	 *
	 * @param udp_counter  The position of the packet in the stack
	 * @param data         The packet to store
	 */
	void add(uint8_t udp_counter, Rt::Ptr<Rt::Data> data);

	/**
	 * @brief Remove a packet from the stack
	 *
	 * @param udp_counter  The position of the packet in the stack
	 * @param data         OUT: the packet stored in the stack or NULL if there
	 *                          is no packet with this counter
	 */
	void remove(uint8_t udp_counter, Rt::Ptr<Rt::Data>& data);

	/**
	 * @brief Check if we have a packet at a specified counter
	 *
	 * @param udp_counter  The counter for which we need a packet
	 * @return true if we have a packet, false otherwise
	 */
	bool hasNext(uint8_t udp_counter);

	/**
	 * @brief Get the packet counter
	 * @return the counter
	 */
	inline uint8_t getCounter() const { return this->counter; };

	/**
	 * @brief Reset the stack
	 */
	void reset();

private:
	/// A counter that increase each time we receive a packet and decrease each time
	//  we handle a packet
	uint8_t counter;

	// Output log
	std::shared_ptr<OutputLog> log_sat_carrier;
};


#endif

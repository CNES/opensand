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
 * @file sat_carrier_udp_channel.h
 * @brief This implements an udp satellite carrier channel
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 */

#ifndef SAT_CARRIER_UDP_CHANNEL_H
#define SAT_CARRIER_UDP_CHANNEL_H

#include "sat_carrier_channel.h"

#include <sys/types.h>
#include <netinet/in.h>


/// The size of the UDP counter (in bytes), 1 or 2
/// Use 2 when you need to rach high bitrates (> 20Mb/s)
//#define COUNTER_SIZE 2 // for uint16_t
#define COUNTER_SIZE 1 // for uint8_t

/// The maximum packets to keep in stack before considering
//  that missing packets are lost
//  be careful to keep coherence with the counter size
//#define MAX_DATA_STACK 1000 // for uint16_t
#define MAX_DATA_STACK 5 // for uint8_t

/// The number of SoF for stack timeout
#define TIMEOUT_SOF_NBR 2

class UdpStack;

/*
 * @class sat_carrier_udp_channel
 * @brief UDP satellite carrier channel
 */
class sat_carrier_udp_channel: public sat_carrier_channel
{
 public:

	sat_carrier_udp_channel(unsigned int channelID,
	                        bool input, bool output,
	                        bool is_data,
	                        const string local_interface_name,
	                        unsigned short port,
	                        bool multicast,
	                        const string local_ip_addr,
	                        const string ip_addr);

	~sat_carrier_udp_channel();

	int getChannelFd();

	bool send(const unsigned char *data, size_t length);

	int receive(NetSocketEvent *const event,
	            unsigned char **buf, size_t &data_len);

	int receive(unsigned char **buf, size_t &data_len);

	bool sofReceived(void);


 protected:

	int receiveSig(NetSocketEvent *const event,
	               unsigned char **buf, size_t &data_len);

	/**
	 * @brief Get the next stacked packet
	 *
	 * @param buf      OUT: the stacked packet
	 * @param data_len OUT: the length of the packet
	 * @return true on success, false otherwise
	 */
	bool handleStack(unsigned char **buf, size_t &data_len);


	/**
	 * @brief Get the next stacked packet
	 *
	 * @param buf      OUT: the stacked packet
	 * @param data_len OUT: the length of the packet
	 * @param stack    The stack in which to get packet
	 */
	void handleStack(unsigned char **buf, size_t &data_len,
	                 uint16_t counter, UdpStack *stack);

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
	/// (counter ranges from 0 to 65535)
	/// (IP address, counter) map used to check that UDP packets are received in
	/// sequence on every UDP communication channel
	map<string , uint16_t> udp_counters;

	/// Counter for sending packets
	uint16_t counter;

	/// The maximum value for counter
	unsigned int max_counter;

	/// internal buffer to build and send udp datagramms
	unsigned char send_buffer[MAX_SOCK_SIZE];

	/// sometimes an UDP datagram containing unfragmented IP packet overtake one
	/// containing fragmented IP packets during its reassembly
	/// Thus, we use the stacks per IP sources to keep the UDP datagram arrived too early
	map<string, UdpStack *> stacks;

	/// the IP address of the stack for which we need to send a packet or
	//  empty string if we have nothing to send
	string stacked_ip;
};

/*
 * @class The UDP stack
 * @brief This stack allows UDP packets ordering in order to avoid
 *        sequence desynchronizations
 */
class UdpStack: vector<pair<unsigned char *, size_t> > 
{
 public:

	/**
	 * @brief Create the stack
	 *
	 */
	UdpStack():
		counter(0),
		timeout(TIMEOUT_SOF_NBR)
	{
		size_t size = 2 << ((COUNTER_SIZE * 8) - 1);

		// reserve space for all UDP counters
		this->reserve(size);
		for(unsigned int i = 0; i < size; i++)
		{
			this->push_back(make_pair<unsigned char *, size_t>(NULL, 0));
		}
	};

	~UdpStack()
	{
		this->reset();
		this->clear();
	};

	/**
	 * @brief Add a packet in the stack
	 *
	 * @param udp_counter  The position of the packet in the stack
	 * @param data         The packet to store
	 * @param data_length  The packet length
	 */
	void add(uint16_t udp_counter, unsigned char *data, size_t data_length)
	{
		// reset timeout
		this->timeout = TIMEOUT_SOF_NBR;
		if(this->at(udp_counter).first)
		{
			UTI_ERROR("new data for UDP stack at position %u, erase previous data\n",
			          udp_counter);
			this->counter--;
			delete (this->at(udp_counter).first);
		}
		this->at(udp_counter).first = data;
		this->at(udp_counter).second = data_length;
		this->counter++;
	};

	/**
	 * @brief Remove a packet from the stack
	 *
	 * @param udp_counter  The position of the packet in the stack
	 * @param data         OUT: the packet stored in the stack or NULL if there
	 *                          is no packet with this counter
	 * @param data_length  OUT: the packet length or 0 if there is no packet
	 */
	void remove(uint16_t udp_counter, unsigned char **data, size_t &data_length)
	{
		*data = this->at(udp_counter).first;
		if(*data)
		{
			this->counter--;
		}
		data_length = this->at(udp_counter).second;
		this->at(udp_counter).first = NULL;
		this->at(udp_counter).second = 0;
	};

	/**
	 * @brief Check if we have a packet at a specified counter
	 *
	 * @param udp_counter  The counter for which we need a packet
	 * @return true if we have a packet, false otherwise
	 */
	bool hasNext(uint16_t udp_counter)
	{
		return (this->at(udp_counter).first != NULL &&
		        this->at(udp_counter).second != 0);
	};

	/**
	 * @brief Get the packet counter
	 * @return the counter
	 */
	uint16_t getCounter()
	{
		return this->counter;
	};

	/**
	 * @brief Signal that a SoF was received
	 *
	 * @return true if the timer expired, false otherwise
	 */
	bool onTimer(void)
	{
		if(!this->counter)
		{
			// only decrease timeout if there is data in stack
			return false;
		}
		this->timeout--;
		if(this->timeout == 0)
		{
			// Timeout expired
			this->timeout = TIMEOUT_SOF_NBR;
			return true;
		}
		return false;
	};

	/**
	 * @brief Reset the stack
	 */
	void reset()
	{
		vector<pair<unsigned char *, size_t> >::iterator it;
		for(it = this->begin(); it != this->end(); ++it)
		{
			if((*it).first)
			{
				delete (*it).first;
				(*it).first = NULL;
				(*it).second = 0;
			}
			this->counter = 0;
		}
		this->timeout = TIMEOUT_SOF_NBR;
	};

 private:

	/// A counter that increase each time we receive a packet and decrease each time
	//  we handle a packet
	uint16_t counter;

	/// The timeout used to send stack once it expires
	uint8_t timeout;
};


#endif

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

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>
#include <opensand_rt/Rt.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <vector>
#include <linux/if_packet.h>
#include <net/if.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <OpenSandCore.h>


class UdpStack;

/*
 * @class UdpChannel
 * @brief UDP satellite carrier channel
 */
class UdpChannel
{
 public:

	UdpChannel(string name,
	           spot_id_t s_id,
	           unsigned int channel_id,
	           bool input, bool output,
	           unsigned short port,
	           bool multicast,
	           const string local_ip_addr,
	           const string ip_addr,
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
	bool send(const unsigned char *data, size_t length);
	int receive(NetSocketEvent *const event,
	            unsigned char **buf, size_t &data_len);

	int getChannelFd();
	
	spot_id_t getSpotId();

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
	                 uint8_t counter, UdpStack *stack);

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
	map<string , uint8_t> udp_counters;

	/// Counter for sending packets
	uint8_t counter;

	/// internal buffer to build and send udp datagramms
	unsigned char send_buffer[MAX_SOCK_SIZE];

	/// sometimes an UDP datagram containing unfragmented IP packet overtake one
	/// containing fragmented IP packets during its reassembly
	/// Thus, we use the stacks per IP sources to keep the UDP datagram arrived too early
	map<string, UdpStack *> stacks;

	/// the IP address of the stack for which we need to send a packet or
	//  empty string if we have nothing to send
	string stacked_ip;

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
class UdpStack: vector<pair<unsigned char *, size_t> > 
{
 public:

	/**
	 * @brief Create the stack
	 *
	 */
	UdpStack()
	{
		// Output log
		this->log_sat_carrier = Output::Get()->registerLog(LEVEL_WARNING, "SatCarrier.Channel");
		// reserve space for all UDP counters
		this->reserve(256);
		for(unsigned int i = 0; i < 256; i++)
		{
			this->push_back(make_pair<unsigned char *, size_t>(NULL, 0));
		}
		this->counter = 0;
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
	void add(uint8_t udp_counter, unsigned char *data, size_t data_length)
	{
		if(this->at(udp_counter).first)
		{
			LOG(this->log_sat_carrier, LEVEL_ERROR, 
			    "new data for UDP stack at position %u, erase "
			    "previous data\n", udp_counter);
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
	void remove(uint8_t udp_counter, unsigned char **data, size_t &data_length)
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
	bool hasNext(uint8_t udp_counter)
	{
		return (this->at(udp_counter).first != NULL &&
		        this->at(udp_counter).second != 0);
	};

	/**
	 * @brief Get the packet counter
	 * @return the counter
	 */
	uint8_t getCounter()
	{
		return this->counter;
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
	};

 private:

	/// A counter that increase each time we receive a packet and decrease each time
	//  we handle a packet
	uint8_t counter;

	// Output log
  std::shared_ptr<OutputLog> log_sat_carrier;
};


#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file InterconnectChannel.h
 * @brief A channel that allows to send messages via an interconnect block.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef INTERCONNECT_CHANNEL_H
#define INTERCONNECT_CHANNEL_H

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>
#include <opensand_rt/Rt.h>

#include "UdpChannel.h"

/**
 * @brief high level channel classes that implement some functions
 *        used by the interconnect blocks
 */

typedef struct
{
	size_t data_len; // NOTE: sending data lenght may actually be redundant on UDP
	uint8_t msg_type;
	unsigned char msg_data[MAX_SOCK_SIZE];
} __attribute__((__packed__)) interconnect_msg_buffer_t;

class InterconnectChannel
{
 public:
	InterconnectChannel(string name, string iface_name, string iface_addr):
		interconnect_iface(iface_name),
		interconnect_addr(iface_addr),
		channel(nullptr)
	{
		this->log_interconnect = Output::registerLog(LEVEL_WARNING, name);
	};

	~InterconnectChannel()
	{
		// Free the channel if it was created
		if (this->channel)
		{
			delete this->channel;
		}
	};

 protected:

	/**
	 * @brief Initialize the UdpChannel
	 */
	virtual void initUdpChannel(unsigned int port,
	                            string remote_addr,
	                            unsigned int stack,
	                            unsigned int rmem,
	                            unsigned int wmem) = 0;

	/// The interconnect interface name
	string interconnect_iface;
	/// The interconnect interface IP address
	string interconnect_addr;
	/// The channel
	UdpChannel *channel;
	/// Output log
	OutputLog *log_interconnect;
};

class InterconnectChannelSender: public InterconnectChannel
{
 public:
	InterconnectChannelSender(string name, string iface_name, string iface_addr):
		InterconnectChannel(name, iface_name, iface_addr)
	{
	};

	virtual ~InterconnectChannelSender()
	{
	};

 protected:

	/**
	 * @brief Initialize the UdpChannel
	 */
	void initUdpChannel(unsigned int port,
	                    string remote_addr,
	                    unsigned int stack,
	                    unsigned int rmem,
	                    unsigned int wmem);

	/**
	 * @brief Send the message contained in the out_buffer.
	 *        out_buffer. total_length must contain the data length;
	 *        this method will update with the correct total length. 
	 */
	bool sendMessage();

	// The output buffer
	interconnect_msg_buffer_t out_buffer;
};

class InterconnectChannelReceiver: public InterconnectChannel
{
 public:
	InterconnectChannelReceiver(string name, string iface_name, string iface_addr):
		InterconnectChannel(name, iface_name, iface_addr),
		socket_event(-1)
	{
	};

	virtual ~InterconnectChannelReceiver()
	{
	};

 protected:

	/**
	 * @brief Initialize the UdpChannel
	 */
	void initUdpChannel(unsigned int port,
	                    string remote_addr,
	                    unsigned int stack,
	                    unsigned int rmem,
	                    unsigned int wmem);

	/**
	 * @brief Receive a message from the socket
	 */
	int receiveMessage(NetSocketEvent *const event,
	                   interconnect_msg_buffer_t **buf);

	/// socket signal event
	int32_t socket_event;
};
#endif

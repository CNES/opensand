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
 * @file InterconnectChannel.cpp
 * @brief A channel that allows to send messages via an interconnect block.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#include "InterconnectChannel.h"

void InterconnectChannelSender::initUdpChannel(unsigned int port, string remote_addr,
                                               unsigned int stack, unsigned int rmem,
                                               unsigned int wmem)
{
	// Create channel
	this->channel = new UdpChannel(name,
	                               0, // no use for the channel ID
	                               0, // no use for the spot ID
	                               false,
	                               true,
	                               this->interconnect_iface,
	                               port,
	                               false, // this socket is not multicast
	                               this->interconnect_addr,
	                               remote_addr,
	                               stack,
	                               rmem,
	                               wmem);
}

void InterconnectChannelReceiver::initUdpChannel(unsigned int port, string remote_addr,
                                                 unsigned int stack, unsigned int rmem,
                                                 unsigned int wmem)
{
	// Create channel
	this->channel = new UdpChannel(name,
	                               0, // no use for the channel ID
	                               0, // no use for the spot ID
	                               true,
	                               false,
	                               this->interconnect_iface,
	                               port,
	                               false, // this socket is not multicast
	                               this->interconnect_addr,
	                               remote_addr,
	                               stack,
	                               rmem,
	                               wmem);
}

bool InterconnectChannelSender::sendMessage()
{
	// Update total length with correct length
	this->out_buffer.data_len += sizeof(this->out_buffer.msg_type) + 
	                             sizeof(this->out_buffer.data_len);
	// Send the data
	return this->channel->send((const unsigned char *) &this->out_buffer,
	                           this->out_buffer.data_len);
}

int InterconnectChannelReceiver::receiveMessage(NetSocketEvent *const event,
                                                interconnect_msg_buffer_t **buf)
{
	int ret = -1;
	size_t length = 0;
	*buf = nullptr;

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "try to receive a packet from interconnect channel "
	    "associated with the file descriptor %d\n", event->getFd());

	// Check if the channel FD corresponds
	if(*event == this->channel->getChannelFd())
	{
		// Try to receive data from the channel
		ret = this->channel->receive(event, (unsigned char **) buf, length);
	}

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "Receive packet: size %zu\n", length);

	// Check that the total_length is correct, and fix data length
	if(ret >= 0 && length > 0)
	{
		if((*buf)->data_len != length)
		{
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "Data length received (%zu) mismatches with message length (%zu)\n",
			    length, (*buf)->data_len);
			return -1;
		}
		(*buf)->data_len -= (sizeof((*buf)->data_len) + sizeof((*buf)->msg_type));
	}
	// If empty packet, return null pointer
	else if(ret >= 0 && length == 0)
	{
		*buf = nullptr;
	}

	return ret;
}

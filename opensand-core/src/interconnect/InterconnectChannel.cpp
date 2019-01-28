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
 * @file InterconnectChannel.cpp
 * @brief A channel that allows to send messages via an interconnect block.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#include "InterconnectChannel.h"

/*
 * INTERCONNECT_CHANNEL_SENDER
 */

void InterconnectChannelSender::initUdpChannels(unsigned int data_port, unsigned int sig_port,
                                                string remote_addr, unsigned int stack,
                                                unsigned int rmem, unsigned int wmem)
{
	// Create channels
	this->data_channel = new UdpChannel(name + ".data",
	                                    0, // no use for the channel ID
	                                    0, // no use for the spot ID
	                                    false,
	                                    true,
	                                    this->interconnect_iface,
	                                    data_port,
	                                    false, // this socket is not multicast
	                                    this->interconnect_addr,
	                                    remote_addr,
	                                    stack,
	                                    rmem,
	                                    wmem);
	this->sig_channel = new UdpChannel(name + ".sig",
	                                   0, // no use for the channel ID
	                                   0, // no use for the spot ID
	                                   false,
	                                   true,
	                                   this->interconnect_iface,
	                                   sig_port,
	                                   false, // this socket is not multicast
	                                   this->interconnect_addr,
	                                   remote_addr,
	                                   stack,
	                                   rmem,
	                                   wmem);
}

bool InterconnectChannelSender::sendBuffer(bool is_sig)
{
	// Send the data
	if(is_sig)
	{
		return this->sig_channel->send((const unsigned char *) &this->out_buffer,
		                               this->out_buffer.data_len);
	}
	return this->data_channel->send((const unsigned char *) &this->out_buffer,
	                                this->out_buffer.data_len);
}

/*
 * Specific methods for type DvbFrame messages
 */
bool InterconnectChannelSender::send(rt_msg_t &message)
{
	uint32_t data_len;

	switch(message.type)
	{
		case msg_sig:
		case msg_data:
			// Serialize the dvb_frame into the output buffer
			this->serialize((DvbFrame *) message.data,
			                this->out_buffer.msg_data, data_len);
			break;
		case msg_saloha:
			this->serialize((std::list<DvbFrame *> *) message.data,
			                this->out_buffer.msg_data, data_len);
			break;
		default:
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "unknonw type of message received\n");
			return false;
	}
	this->out_buffer.msg_type = message.type;
	this->out_buffer.data_len = data_len;

	// Update total length with correct length
	this->out_buffer.data_len += sizeof(this->out_buffer.msg_type) + 
	                             sizeof(this->out_buffer.data_len);

	// Send the message
	return this->sendBuffer(message.type == msg_sig);
}

void InterconnectChannelSender::serialize(DvbFrame *dvb_frame,
                                          unsigned char *buf,
                                          uint32_t &length)
{
	uint32_t total_len = 0, pos = 0;
	spot_id_t spot;
	uint8_t carrier_id;

	spot = dvb_frame->getSpot();
	carrier_id = dvb_frame->getCarrierId();

	total_len += sizeof(spot);
	total_len += sizeof(carrier_id);
	total_len += dvb_frame->getTotalLength();
	// Copy data to buffer
	memcpy(buf + pos, &spot, sizeof(spot));
	pos += sizeof(spot);
	memcpy(buf + pos, &carrier_id, sizeof(carrier_id));
	pos += sizeof(carrier_id);
	memcpy(buf + pos, dvb_frame->getData().c_str(),
	       dvb_frame->getTotalLength());
	length = total_len;
}

void InterconnectChannelSender::serialize(std::list<DvbFrame *> *dvb_frame_list,
                                          unsigned char *buf,
                                          uint32_t &length)
{
	std::list<DvbFrame *>::iterator it;
	length = 0;
	// Iterate over dvb_frames
	for(it = dvb_frame_list->begin(); it != dvb_frame_list->end(); it++)
	{
		uint32_t partial_len;
		// First serialize dvb_frame
		this->serialize(*it, buf + length + sizeof(partial_len), partial_len);
		// Copy the size of the dvb_frame before the frame itself
		memcpy(buf + length, &partial_len, sizeof(partial_len));
		length += partial_len + sizeof(partial_len);
	}
}

/*
 * INTERCONNECT_CHANNEL_RECEIVER
 */

void InterconnectChannelReceiver::initUdpChannels(unsigned int data_port, unsigned int sig_port,
                                                  string remote_addr, unsigned int stack,
                                                  unsigned int rmem, unsigned int wmem)
{
	// Create channel
	this->data_channel = new UdpChannel(name + ".data",
	                                    0, // no use for the channel ID
	                                    0, // no use for the spot ID
	                                    true,
	                                    false,
	                                    this->interconnect_iface,
	                                    data_port,
	                                    false, // this socket is not multicast
	                                    this->interconnect_addr,
	                                    remote_addr,
	                                    stack,
	                                    rmem,
	                                    wmem);
	this->sig_channel = new UdpChannel(name + ".sig",
	                                   0, // no use for the channel ID
	                                   0, // no use for the spot ID
	                                   true,
	                                   false,
	                                   this->interconnect_iface,
	                                   sig_port,
	                                   false, // this socket is not multicast
	                                   this->interconnect_addr,
	                                   remote_addr,
	                                   stack,
	                                   rmem,
	                                   wmem);
}

int InterconnectChannelReceiver::receiveToBuffer(NetSocketEvent *const event,
                                                 interconnect_msg_buffer_t **buf)
{
	int ret = -1;
	size_t length = 0;
	*buf = NULL;

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "try to receive a packet from interconnect channel "
	    "associated with the file descriptor %d\n", event->getFd());

	// Try to receive data from the channel
	if(*event == this->sig_channel->getChannelFd())
	{
		ret = this->sig_channel->receive(event, (unsigned char **) buf, length);
	}
	else
	{
		ret = this->data_channel->receive(event, (unsigned char **) buf, length);
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
		*buf = NULL;
	}

	return ret;
}

bool InterconnectChannelReceiver::receive(NetSocketEvent *const event,
                                          std::list<rt_msg_t> &messages)
{
	bool status = true;
	int ret;

	// Check if the event corresponds to any of the sockets
	if(*event != this->sig_channel->getChannelFd() &&
	   *event != this->data_channel->getChannelFd())
	{
		LOG(this->log_interconnect, LEVEL_DEBUG,
		    "Event does not correspond to interconnect socket\n");
		return true;
	}

	// Start receiving messages
	do
	{
		interconnect_msg_buffer_t *buf = NULL;

		ret = this->receiveToBuffer(event, &buf);
		if(ret < 0)
		{
			// Problem on reception
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "failed to receive data on input channel\n");
			return false;
		}
		else if(buf)
		{
			rt_msg_t message;

			// A message was received
			LOG(this->log_interconnect, LEVEL_DEBUG,
			    "%zu bytes of data received\n",
			    buf->data_len);

			message.type = buf->msg_type;
			message.length = buf->data_len;

			// Deserialize the message
			switch(buf->msg_type)
			{
				case msg_data:
				case msg_sig:
					// Deserialize the dvb_frame
					this->deserialize(buf->msg_data, buf->data_len,
					                  (DvbFrame **) &message.data);
					break;
				case msg_saloha:
					// Deserialize the list of dvb_frames
					this->deserialize(buf->msg_data, buf->data_len,
					                  (std::list<DvbFrame *> **) &message.data);
					break;
				default:
					LOG(this->log_interconnect, LEVEL_ERROR,
					    "Unknown type of message received\n");
					status = false;
					free(buf);
					continue;
			}
			// Free buf
			free(buf);

			// Insert the message in the list
			messages.push_back(message);
		}
	} while (ret > 0);
	return status;
}

void InterconnectChannelReceiver::deserialize(unsigned char *data, uint32_t len,
                                              DvbFrame **dvb_frame)
{
	spot_id_t spot;
	uint8_t carrier_id;
	uint32_t pos = 0;

	// Extract SpotId and CarrierId
	memcpy(&spot, data + pos, sizeof(spot));
	pos += sizeof(spot);
	memcpy(&carrier_id, data + pos, sizeof(carrier_id));
	pos += sizeof(carrier_id);

	// Create object    
	(*dvb_frame) = new DvbFrame(data + pos, len - pos);
	(*dvb_frame)->setCarrierId(carrier_id);
	(*dvb_frame)->setSpot(spot);
}


void InterconnectChannelReceiver::deserialize(unsigned char *data, uint32_t len,
                                              std::list<DvbFrame *> **dvb_frame_list)
{
	uint32_t pos = 0;

	// Create object
	(*dvb_frame_list) = new std::list<DvbFrame *>();

	// Iterate and create DvbFrame objects
	do
	{
		uint32_t dvb_frame_len;
		DvbFrame *dvb_frame = NULL;

		// Read the length of dvb_frame
		memcpy(&dvb_frame_len, data + pos, sizeof(dvb_frame_len));
		pos += sizeof(dvb_frame_len);

		// Deserialize the new DvbFrame
		this->deserialize(data + pos, dvb_frame_len, &dvb_frame);

		// Insert the new DvbFrame in the list
		(*dvb_frame_list)->push_back(dvb_frame);

		// Update position
		pos += dvb_frame_len;

	} while(pos < len);
}

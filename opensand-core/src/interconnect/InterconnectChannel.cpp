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

#include <opensand_output/Output.h>
#include <opensand_rt/NetSocketEvent.h>


InterconnectChannel::InterconnectChannel(std::string name, std::string iface_addr):
	name(name),
	interconnect_addr(iface_addr),
	data_channel(nullptr),
	sig_channel(nullptr)
{
	this->log_interconnect = Output::Get()->registerLog(LEVEL_WARNING, name + ".common");
}

InterconnectChannel::~InterconnectChannel()
{
	// Free the channel if it was created
	if (this->data_channel)
	{
		delete this->data_channel;
	}
	// Free the channel if it was created
	if (this->sig_channel)
	{
		delete this->sig_channel;
	}
}

/*
 * INTERCONNECT_CHANNEL_SENDER
 */

void InterconnectChannelSender::initUdpChannels(unsigned int data_port, unsigned int sig_port,
                                                std::string remote_addr, unsigned int stack,
                                                unsigned int rmem, unsigned int wmem)
{
	// Create channels
	this->data_channel = new UdpChannel(name + ".data",
	                                    0, // no use for the channel ID
	                                    0, // no use for the spot ID
	                                    false,
	                                    true,
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
	                                   sig_port,
	                                   false, // this socket is not multicast
	                                   this->interconnect_addr,
	                                   remote_addr,
	                                   stack,
	                                   rmem,
	                                   wmem);
}

bool InterconnectChannelSender::sendBuffer(bool is_sig, const interconnect_msg_buffer_t &msg)
{
	auto buffer = reinterpret_cast<const uint8_t *>(&msg);
	auto channel = is_sig ? this->sig_channel : this->data_channel;
	return channel->send(buffer, msg.data_len);
}

/*
 * Specific methods for type DvbFrame messages
 */
bool InterconnectChannelSender::send(rt_msg_t &message)
{
	time_ms_t current_time = getCurrentTime();

	interconnect_msg_buffer_t msg_buffer;
	uint32_t len;

	auto msg_type = to_enum<InternalMessageType>(message.type);
	msg_buffer.msg_type = message.type;

	// Serialize the message
	if (msg_type == InternalMessageType::msg_data || msg_type == InternalMessageType::msg_sig)
	{
		auto frame = std::unique_ptr<DvbFrame>{static_cast<DvbFrame *>(message.data)};
		this->serialize(frame.get(), msg_buffer.msg_data, len);
	}
	else if (msg_type == InternalMessageType::msg_saloha)
	{
		auto dvb_frames = std::unique_ptr<std::list<DvbFrame *>>{static_cast<std::list<DvbFrame *> *>(message.data)};
		this->serialize(dvb_frames.get(), msg_buffer.msg_data, len);
	}
	else
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "unsupported type of message received\n");
		return false;
	}

	// add the length of the other fields
	msg_buffer.data_len = len + sizeof(msg_buffer.msg_type) + sizeof(msg_buffer.data_len);

	// construct a NetContainer to store it in a FifoElement
	auto buf = reinterpret_cast<const uint8_t *>(&msg_buffer);
	std::unique_ptr<NetContainer> container{new NetContainer(buf, msg_buffer.data_len)};
	FifoElement *elem = new FifoElement(std::move(container), current_time, current_time + delay);

	return delay_fifo.pushBack(elem);
}

bool InterconnectChannelSender::onTimerEvent()
{
	time_ms_t current_time = getCurrentTime();

	while (delay_fifo.getCurrentSize() > 0 && ((unsigned long)delay_fifo.getTickOut()) <= current_time)
	{
		FifoElement *elem = delay_fifo.pop();
		assert(elem != nullptr);

		auto container = elem->getElem<NetContainer>();
		auto msg = reinterpret_cast<const interconnect_msg_buffer_t *>(container->getRawData());
		bool is_sig = to_enum<InternalMessageType>(msg->msg_type) == InternalMessageType::msg_sig;
		if (!sendBuffer(is_sig, *msg))
		{
			LOG(this->log_interconnect, LEVEL_ERROR, "failed to send buffer\n");
			return false;
		};
	}
	return true;
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
	memcpy(buf + pos, dvb_frame->getRawData(),
	       dvb_frame->getTotalLength());
	length = total_len;
}

void InterconnectChannelSender::serialize(std::list<DvbFrame *> *dvb_frame_list,
                                          unsigned char *buf,
                                          uint32_t &length)
{
	length = 0;
	// Iterate over dvb_frames
	for (auto dvb_frame: *dvb_frame_list)
	{
		uint32_t partial_len;
		// First serialize dvb_frame
		this->serialize(dvb_frame, buf + length + sizeof(partial_len), partial_len);
		// Copy the size of the dvb_frame before the frame itself
		memcpy(buf + length, &partial_len, sizeof(partial_len));
		length += partial_len + sizeof(partial_len);
	}
}

/*
 * INTERCONNECT_CHANNEL_RECEIVER
 */

void InterconnectChannelReceiver::initUdpChannels(unsigned int data_port, unsigned int sig_port,
                                                  std::string remote_addr, unsigned int stack,
                                                  unsigned int rmem, unsigned int wmem)
{
	// Create channel
	this->data_channel = new UdpChannel(name + ".data",
	                                    0, // no use for the channel ID
	                                    0, // no use for the spot ID
	                                    true,
	                                    false,
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
			switch(to_enum<InternalMessageType>(buf->msg_type))
			{
				case InternalMessageType::msg_data:
				case InternalMessageType::msg_sig:
					// Deserialize the dvb_frame
					this->deserialize(buf->msg_data, buf->data_len,
					                  (DvbFrame **) &message.data);
					break;
				case InternalMessageType::msg_saloha:
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

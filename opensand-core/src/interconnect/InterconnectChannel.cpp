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


#include <cstring>

#include <opensand_output/Output.h>
#include <opensand_rt/Types.h>
#include <opensand_rt/NetSocketEvent.h>

#include "InterconnectChannel.h"
#include "BlockInterconnect.h"
#include "NetBurst.h"
#include "NetPacket.h"
#include "FifoElement.h"
#include "Except.h"


InterconnectChannel::InterconnectChannel(std::string name, const InterconnectConfig &config):
	name{name},
	interconnect_addr{config.interconnect_addr},
	data_channel{nullptr},
	sig_channel{nullptr}
{
	this->log_interconnect = Output::Get()->registerLog(LEVEL_WARNING, name + ".common");
}


/*
 * INTERCONNECT_CHANNEL_SENDER
 */
InterconnectChannelSender::InterconnectChannelSender(std::string name, const InterconnectConfig &config):
	InterconnectChannel{name, config},
	delay{config.delay}
{
}


void InterconnectChannelSender::initUdpChannels(unsigned int data_port, unsigned int sig_port,
                                                std::string remote_addr, unsigned int stack,
                                                unsigned int rmem, unsigned int wmem)
{
	// Create channels
	this->data_channel = std::make_unique<UdpChannel>(name + ".data",
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
	this->sig_channel = std::make_unique<UdpChannel>(name + ".sig",
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
	return (is_sig ? this->sig_channel : this->data_channel)->send(buffer, msg.data_len);
}


/*
 * Specific methods for type DvbFrame messages
 */
bool InterconnectChannelSender::send(Rt::Message message)
{
	interconnect_msg_buffer_t msg_buffer;
	uint32_t len;

	auto msg_type = to_enum<InternalMessageType>(message.type);
	msg_buffer.msg_type = message.type;

	// Serialize the message
	if (msg_type == InternalMessageType::encap_data || msg_type == InternalMessageType::sig)
	{
		this->serialize(message.release<DvbFrame>(), msg_buffer.msg_data, len);
	}
	else if (msg_type == InternalMessageType::saloha)
	{
		this->serialize(message.release<std::list<Rt::Ptr<DvbFrame>>>(), msg_buffer.msg_data, len);
	}
	else if (msg_type == InternalMessageType::decap_data)
	{
		this->serialize(message.release<NetBurst>(), msg_buffer.msg_data, len);
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
	Rt::Ptr<NetContainer> container = Rt::make_ptr<NetContainer>(buf, msg_buffer.data_len);

	if (!delay_fifo.push(std::move(container), delay)) {
		LOG(this->log_interconnect, LEVEL_ERROR, "failed to push the message in the fifo\n");
		return false;
	}
	
	// if no delay, send directly
	if (delay == time_ms_t::zero())
	{
		return onTimerEvent();
	}

	return true;
}


bool InterconnectChannelSender::onTimerEvent()
{
	for (auto &&elem: delay_fifo)
	{
		if (!elem)
		{
			LOG(this->log_interconnect, LEVEL_ERROR, "message to send is NULL\n");
			return false;
		}

		auto container = elem->releaseElem<NetContainer>();
		auto msg = reinterpret_cast<const interconnect_msg_buffer_t *>(container->getRawData());
		bool is_sig = to_enum<InternalMessageType>(msg->msg_type) == InternalMessageType::sig;
		if (!sendBuffer(is_sig, *msg))
		{
			LOG(this->log_interconnect, LEVEL_ERROR, "failed to send buffer\n");
			return false;
		}
	}

	return true;
}


template <typename T>
void serializeField(uint8_t *buf, uint32_t &pos, T *data, uint32_t length = sizeof(T))
{
	memcpy(buf + pos, data, length);
	pos += length;
}


void InterconnectChannelSender::serialize(Rt::Ptr<DvbFrame> dvb_frame,
                                          unsigned char *buf,
                                          uint32_t &length)
{
	uint32_t pos = 0;
	spot_id_t spot = dvb_frame->getSpot();
	uint8_t carrier_id = dvb_frame->getCarrierId();

	serializeField(buf, pos, &spot);
	serializeField(buf, pos, &carrier_id);
	serializeField(buf, pos, dvb_frame->getRawData(), dvb_frame->getTotalLength());
	length = pos;
}

void InterconnectChannelSender::serialize(Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> dvb_frame_list,
                                          unsigned char *buf,
                                          uint32_t &length)
{
	length = 0;
	// Iterate over dvb_frames
	for (auto &&dvb_frame: *dvb_frame_list)
	{
		uint32_t partial_len;
		// First serialize dvb_frame
		this->serialize(std::move(dvb_frame), buf + length + sizeof(partial_len), partial_len);
		// Copy the size of the dvb_frame before the frame itself
		memcpy(buf + length, &partial_len, sizeof(partial_len));
		length += partial_len + sizeof(partial_len);
	}
}

void InterconnectChannelSender::serialize(Rt::Ptr<NetBurst> net_burst,
                                          unsigned char *buf,
                                          uint32_t &length)
{
	length = 0;
	for (auto &&packet: *net_burst)
	{
		uint32_t partial_len;
		// First serialize the packet
		this->serialize(std::move(packet), buf + length + sizeof(partial_len), partial_len);
		// Copy the size of the packet before the packet itself
		memcpy(buf + length, &partial_len, sizeof(partial_len));
		length += partial_len + sizeof(partial_len);
		ASSERT(length <= MAX_SOCK_SIZE, "Too much data to write compared to socket buffer size");
	}
}

void InterconnectChannelSender::serialize(Rt::Ptr<NetPacket> packet,
                                          unsigned char *buf,
                                          uint32_t &length)
{
	uint32_t pos = 0;

	uint8_t src_id = packet->getSrcTalId();
	uint8_t dest_id = packet->getDstTalId();
	uint8_t qos = packet->getQos();
	NET_PROTO type = packet->getType();
	uint32_t header_length = packet->getHeaderLength();

	serializeField(buf, pos, &src_id);
	serializeField(buf, pos, &dest_id);
	serializeField(buf, pos, &qos);
	serializeField(buf, pos, &type);
	serializeField(buf, pos, &header_length);
	serializeField(buf, pos, packet->getRawData(), packet->getTotalLength());
	length = pos;
}


/*
 * INTERCONNECT_CHANNEL_RECEIVER
 */

void InterconnectChannelReceiver::initUdpChannels(unsigned int data_port, unsigned int sig_port,
                                                  std::string remote_addr, unsigned int stack,
                                                  unsigned int rmem, unsigned int wmem)
{
	// Create channel
	this->data_channel = std::make_unique<UdpChannel>(name + ".data",
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
	this->sig_channel = std::make_unique<UdpChannel>(name + ".sig",
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

UdpChannel::ReceiveStatus InterconnectChannelReceiver::receiveToBuffer(
		const Rt::NetSocketEvent& event,
		Rt::Ptr<Rt::Data> &buf)
{
	UdpChannel::ReceiveStatus ret = UdpChannel::ERROR;

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "try to receive a packet from interconnect channel "
	    "associated with the file descriptor %d\n", event.getFd());

	// Try to receive data from the channel
	if(event == this->sig_channel->getChannelFd())
	{
		ret = this->sig_channel->receive(event, buf);
	}
	else
	{
		ret = this->data_channel->receive(event, buf);
	}

	if(!buf)
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "Receive packet failed, no data retrieved\n");
	}

	std::size_t length = buf->length();
	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "Receive packet: size %zu\n", length);

	// Check that the total_length is correct, and fix data length
	if(ret != UdpChannel::ERROR && length > 0)
	{
		interconnect_msg_buffer_t *buffer = reinterpret_cast<interconnect_msg_buffer_t *>(buf->data());
		if(buffer->data_len != length)
		{
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "Data length received (%zu) mismatches with message length (%zu)\n",
			    length, buffer->data_len);
			return UdpChannel::ERROR;
		}
		buffer->data_len -= (sizeof(buffer->data_len) + sizeof(buffer->msg_type));
	}
	// If empty packet, return null pointer
	else if(ret != UdpChannel::ERROR && length == 0)
	{
		buf = Rt::make_ptr<Rt::Data>(nullptr);
	}

	return ret;
}

bool InterconnectChannelReceiver::receive(const Rt::NetSocketEvent& event,
                                          std::list<Rt::Message> &messages)
{
	bool status = true;
	UdpChannel::ReceiveStatus ret;

	// Check if the event corresponds to any of the sockets
	if(!(event == this->sig_channel->getChannelFd() ||
	     event == this->data_channel->getChannelFd()))
	{
		LOG(this->log_interconnect, LEVEL_DEBUG,
		    "Event does not correspond to interconnect socket\n");
		return true;
	}

	// Start receiving messages
	do
	{
		Rt::Ptr<Rt::Data> buffer = Rt::make_ptr<Rt::Data>(nullptr);
		ret = this->receiveToBuffer(event, buffer);
		if(ret == UdpChannel::ERROR)
		{
			// Problem on reception
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "failed to receive data on input channel\n");
			return false;
		}
		else if(buffer)
		{
			Rt::Message message{nullptr};

			// A message was received
			LOG(this->log_interconnect, LEVEL_DEBUG,
			    "%zu bytes of data received\n",
			    buffer->length());

			interconnect_msg_buffer_t *buf = reinterpret_cast<interconnect_msg_buffer_t *>(buffer->data());
			message.type = buf->msg_type;

			// Deserialize the message
			switch(to_enum<InternalMessageType>(buf->msg_type))
			{
				case InternalMessageType::encap_data:
				case InternalMessageType::sig:
				{
					// Deserialize the dvb_frame
					Rt::Ptr<DvbFrame> dvb_data = Rt::make_ptr<DvbFrame>(nullptr);
					this->deserialize(buf->msg_data, buf->data_len, dvb_data);
					message = std::move(dvb_data);
					break;
				}
				case InternalMessageType::saloha:
				{
					// Deserialize the list of dvb_frames
					Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> saloha_data = Rt::make_ptr<std::list<Rt::Ptr<DvbFrame>>>(nullptr);
					this->deserialize(buf->msg_data, buf->data_len, saloha_data);
					message = std::move(saloha_data);
					break;
				}
				case InternalMessageType::decap_data:
				{
					// Deserialize the NetBurst
					Rt::Ptr<NetBurst> burst_data = Rt::make_ptr<NetBurst>(nullptr);
					this->deserialize(buf->msg_data, buf->data_len, burst_data);
					message = std::move(burst_data);
					break;
				}
				default:
					LOG(this->log_interconnect, LEVEL_ERROR,
					    "Unknown type of message received\n");
					status = false;
					continue;
			}
			// Insert the message in the list
			messages.push_back(std::move(message));
		}
	} while (ret == UdpChannel::STACKED);
	return status;
}

template <typename T>
void deserializeField(uint8_t *buf, uint32_t &pos, T &data, uint32_t length = sizeof(T))
{
	memcpy(&data, buf + pos, length);
	pos += length;
}

void InterconnectChannelReceiver::deserialize(unsigned char *data, uint32_t len,
                                              Rt::Ptr<DvbFrame> &dvb_frame)
{
	spot_id_t spot;
	uint8_t carrier_id;
	uint32_t pos = 0;

	// Extract SpotId and CarrierId
	deserializeField(data, pos, spot);
	deserializeField(data, pos, carrier_id);

	// Create object
	dvb_frame = Rt::make_ptr<DvbFrame>(data + pos, len - pos);
	dvb_frame->setCarrierId(carrier_id);
	dvb_frame->setSpot(spot);
}


void InterconnectChannelReceiver::deserialize(unsigned char *data, uint32_t len,
                                              Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> &dvb_frame_list)
{
	uint32_t pos = 0;

	// Create object
	dvb_frame_list = Rt::make_ptr<std::list<Rt::Ptr<DvbFrame>>>();

	// Iterate and create DvbFrame objects
	do
	{
		uint32_t dvb_frame_len;
		Rt::Ptr<DvbFrame> dvb_frame = Rt::make_ptr<DvbFrame>(nullptr);

		// Read the length of dvb_frame
		deserializeField(data, pos, dvb_frame_len);

		// Deserialize the new DvbFrame
		this->deserialize(data + pos, dvb_frame_len, dvb_frame);

		// Insert the new DvbFrame in the list
		dvb_frame_list->push_back(std::move(dvb_frame));

		// Update position
		pos += dvb_frame_len;
	}
	while(pos < len);
	
	ASSERT(pos == len, "Length mismatch between serialized data and extracted DvbFrames");
}

void InterconnectChannelReceiver::deserialize(uint8_t *buf, uint32_t length,
                                              Rt::Ptr<NetBurst> &net_burst)
{
	uint32_t pos = 0;
	net_burst = Rt::make_ptr<NetBurst>();

	do
	{
		uint32_t packet_length;
		Rt::Ptr<NetPacket> packet = Rt::make_ptr<NetPacket>(nullptr);

		// Read the length of the packet
		deserializeField(buf, pos, packet_length);

		// Deserialize the packet
		this->deserialize(buf + pos, packet_length, packet);

		// Insert the new packet in the burst
		net_burst->push_back(std::move(packet));

		// Update position
		pos += packet_length;
	}
	while (pos < length);

	ASSERT(pos == length, "Length mismatch between serialized data and extracted NetBurst");
}

void InterconnectChannelReceiver::deserialize(uint8_t *buf, uint32_t length,
                                              Rt::Ptr<NetPacket> &packet)
{
	uint32_t pos = 0;

	uint8_t src_id;
	uint8_t dest_id;
	uint8_t qos;
	NET_PROTO type;
	uint32_t header_length;

	deserializeField(buf, pos, src_id);
	deserializeField(buf, pos, dest_id);
	deserializeField(buf, pos, qos);
	deserializeField(buf, pos, type);
	deserializeField(buf, pos, header_length);

	packet.reset(new NetPacket{Rt::Data{buf + pos, length - pos},
	                           length - pos,
	                           "interconnect",
	                           type,
	                           qos,
	                           src_id,
	                           dest_id,
	                           header_length});
}

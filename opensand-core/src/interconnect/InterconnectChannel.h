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
 * @file InterconnectChannel.h
 * @brief A channel that allows to send messages via an interconnect block.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef INTERCONNECT_CHANNEL_H
#define INTERCONNECT_CHANNEL_H


#include <list>

#include <opensand_rt/Ptr.h>
#include <opensand_rt/Data.h>

#include "DvbFrame.h"
#include "DelayFifo.h"
#include "UdpChannel.h"
#include "IslPlugin.h"


class OutputLog;
class InterconnectConfig;
class NetBurst;
class NetPacket;
namespace Rt {
	class Message;
	class NetSocketEvent;
};


struct __attribute__((__packed__)) interconnect_msg_buffer_t
{
	uint32_t data_len; // NOTE: sending data lenght may actually be redundant on UDP
	uint8_t msg_type;
	uint8_t msg_data[MAX_SOCK_SIZE];
};


/**
 * @brief high level channel classes that implement some functions
 *        used by the interconnect blocks
 */
class InterconnectChannel
{
 public:
	InterconnectChannel(std::string name, const InterconnectConfig &config);
	virtual ~InterconnectChannel() = default;

 protected:
	/**
	 * @brief Initialize the UdpChannel
	 */
	virtual void initUdpChannels(unsigned int data_port,
	                             unsigned int sig_port,
	                             std::string remote_addr,
	                             unsigned int stack,
	                             unsigned int rmem,
	                             unsigned int wmem) = 0;
	/// This blocks name
	std::string name;
	/// The interconnect interface IP address
	std::string interconnect_addr;
	/// The data channel
	std::unique_ptr<UdpChannel> data_channel;
	/// The signalling channel
	std::unique_ptr<UdpChannel> sig_channel;
	/// Output log
	std::shared_ptr<OutputLog> log_interconnect;
};


class InterconnectChannelSender: public InterconnectChannel
{
public:
	InterconnectChannelSender(std::string name, const InterconnectConfig &config);

	bool onTimerEvent();

protected:
	/**
	 * @brief Initialize the UdpChannel
	 */
	void initUdpChannels(unsigned int data_port,
	                     unsigned int sig_port,
	                     std::string remote_addr,
	                     unsigned int stack,
	                     unsigned int rmem,
	                     unsigned int wmem) override;

	/**
	 * @brief Send a RtMessage via the interconnect channel.
	 * @return false on error, true elsewise.
	 */
	bool send(Rt::Message message);

	/**
	 * @brief Sends a message. total_length must contain the data length;
	 * @param is_sig indicates if the message must be sent via the sig channel
	 * @param msg the message to send
	 * @return false on error, true elsewise.
	 */
	bool sendBuffer(bool is_sig, const interconnect_msg_buffer_t &msg);

	std::shared_ptr<IslDelayPlugin> delay = nullptr;

private:
	/**
	 * @brief Serialize a Dvb Frame to be sent via the
	 *        interconnect channel.
	 */
	void serialize(Rt::Ptr<DvbFrame> dvb_frame,
	               unsigned char *buf,
	               uint32_t &length);

	/**
	 * @brief Serialize a list of Dvb Frames to be sent
	 *        via the interconnect channel.
	 */
	void serialize(Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> dvb_frame_list,
	               unsigned char *buf,
	               uint32_t &length);

	/**
	 * @brief Serialize a NetBurst to be sent
	 *        via the interconnect channel.
	 */
	void serialize(Rt::Ptr<NetBurst> net_burst,
	               unsigned char *buf,
	               uint32_t &length);

	/**
	 * @brief Serialize a NetPacket to be sent
	 *        via the interconnect channel.
	 */
	void serialize(Rt::Ptr<NetPacket> packet,
	               unsigned char *buf,
	               uint32_t &length);

	DelayFifo delay_fifo;
};


class InterconnectChannelReceiver: public InterconnectChannel
{
public:
	using InterconnectChannel::InterconnectChannel;

protected:
	/**
	 * @brief Initialize the UdpChannel
	 */
	void initUdpChannels(unsigned int data_port,
	                     unsigned int sig_port,
	                     std::string remote_addr,
	                     unsigned int stack,
	                     unsigned int rmem,
	                     unsigned int wmem) override;

	/**
	 * @brief Receive a message from the socket
	 * @return -1 on error, 1 if more packets can be read, 0 if last packet.
	 */
	UdpChannel::ReceiveStatus receiveToBuffer(const Rt::NetSocketEvent &event, Rt::Ptr<Rt::Data> &buf);

	/**
	 * @brief Receive RtMessages
	 * @return false on error, true elsewise.
	 */
	bool receive(const Rt::NetSocketEvent &event,
	             std::list<Rt::Message> &messages);

private:
	/**
	 * @brief Create a DvbFrame from serialized data
	 */
	void deserialize(unsigned char *data, uint32_t len,
	                 Rt::Ptr<DvbFrame> &dvb_frame);

	/**
	 * @brief Create a DvbFrame list from serialized data
	 */
	void deserialize(unsigned char *data, uint32_t len,
	                 Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> &dvb_frame_list);

	/**
	 * @brief Create a NetBurst from serialized data
	 */
	void deserialize(uint8_t *buf, uint32_t length,
	                 Rt::Ptr<NetBurst> &net_burst);

	/**
	 * @brief Create a NetPacket from serialized data
	 */
	void deserialize(uint8_t *buf, uint32_t length,
	                 Rt::Ptr<NetPacket> &packet);
};
#endif

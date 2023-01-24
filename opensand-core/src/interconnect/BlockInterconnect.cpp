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
 * @file BlockInterconnect.cpp
 * @brief This file implements two blocks that commucate using an InterconnectChannel.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */


#include <opensand_output/Output.h>
#include <opensand_rt/TimerEvent.h>
#include <opensand_rt/MessageEvent.h>
#include <opensand_rt/NetSocketEvent.h>

#include "BlockInterconnect.h"
#include "OpenSandModelConf.h"
#include "UdpChannel.h"


Rt::UpwardChannel<BlockInterconnectDownward>::UpwardChannel(const std::string &name, const InterconnectConfig &config):
	Channels::Upward<UpwardChannel<BlockInterconnectDownward>>{name},
	InterconnectChannelReceiver{name + ".Upward", config},
	isl_index{config.isl_index}
{
}


Rt::DownwardChannel<BlockInterconnectDownward>::DownwardChannel(const std::string &name, const InterconnectConfig &config):
	Channels::Downward<DownwardChannel<BlockInterconnectDownward>>{name},
	InterconnectChannelSender{name + ".Downward", config},
	isl_index{config.isl_index}
{
	if (config.delay == 0)
	{
		// No need to poll, messages are sent directly
		polling_rate = 0;
	}
	else if (!OpenSandModelConf::Get()->getDelayTimer(polling_rate))
	{
		LOG(log_init, LEVEL_ERROR, "Cannot get the polling rate for the delay timer");
	}
}


bool Rt::DownwardChannel<BlockInterconnectDownward>::onEvent(const Event &event)
{
	LOG(this->log_interconnect, LEVEL_ERROR,
	    "unknown event received %s",
	    event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<BlockInterconnectDownward>::onEvent(const TimerEvent &event)
{
	if (event == delay_timer)
	{
		onTimerEvent();
		return true;
	}

	LOG(this->log_interconnect, LEVEL_ERROR,
	    "unknown timer event received %s",
	    event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<BlockInterconnectDownward>::onEvent(const MessageEvent &event)
{
	Message message{event.getMessage<void>()};
	message.type = event.getMessageType();

	// Check if object inside
	if(!this->send(std::move(message)))
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "error when sending data\n");
		return false;
	}

	return true;
}


bool Rt::UpwardChannel<BlockInterconnectDownward>::onEvent(const Event &event)
{
	LOG(this->log_interconnect, LEVEL_ERROR,
	    "unknown event received %s\n",
	    event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<BlockInterconnectDownward>::onEvent(const NetSocketEvent &event)
{
	bool status = true;

	std::list<Message> messages;

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "NetSocket event received\n");

	// Receive messages
	if(!this->receive(event, messages))
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "error when receiving data on input channel\n");
		status = false;
	}
	// Iterate over received messages
	for(auto&& message : messages)
	{
		// Send message to the next block
		if(!this->enqueueMessage(message.release<void>(), message.type))
		{
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "failed to send message to next block\n");
			status = false;
		}
	}

	return status;
}


bool BlockInterconnectDownward::onInit()
{
	// Register log
	this->log_interconnect = Output::Get()->registerLog(LEVEL_WARNING, "InterconnectDownward.block");
	return true;
}


bool Rt::UpwardChannel<BlockInterconnectDownward>::onInit()
{
	std::string name="UpwardInterconnectChannel";
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	std::string remote_addr("");
	int32_t socket_event;

	auto Conf = OpenSandModelConf::Get();
	if(!Conf->getInterconnectCarrier(true, remote_addr, data_port, sig_port, stack, rmem, wmem, isl_index))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Entity infrastructure is missing interconnect data\n");
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	// Add NetSocketEvents
	socket_event = this->addNetSocketEvent(name + "_data",
	                                       this->data_channel->getChannelFd(),
	                                       MAX_SOCK_SIZE);
	if(socket_event < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add data socket event to Upward channel\n");
		return false;
	}
	socket_event = this->addNetSocketEvent(name + "_sig",
	                                       this->sig_channel->getChannelFd(),
	                                       MAX_SOCK_SIZE);
	if(socket_event < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add sig socket event to Upward channel\n");
		return false;
	}
	return true;
}


bool Rt::DownwardChannel<BlockInterconnectDownward>::onInit()
{
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	std::string remote_addr("");

	auto Conf = OpenSandModelConf::Get();
	if(!Conf->getInterconnectCarrier(false, remote_addr, data_port, sig_port, stack, rmem, wmem, isl_index))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Entity infrastructure is missing interconnect data\n");
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	delay_timer = this->addTimerEvent(name + ".delay_timer", polling_rate);

	return true;
}


Rt::UpwardChannel<BlockInterconnectUpward>::UpwardChannel(const std::string &name, const InterconnectConfig &config):
	Channels::Upward<UpwardChannel<BlockInterconnectUpward>>{name},
	InterconnectChannelSender{name + ".Upward", config},
	isl_index{config.isl_index}
{
	if (config.delay == 0)
	{
		// No need to poll, messages are sent directly
		polling_rate = 0;
	}
	else if (!OpenSandModelConf::Get()->getDelayTimer(polling_rate))
	{
		LOG(log_init, LEVEL_ERROR, "Cannot get the polling rate for the delay timer");
	}
}


Rt::DownwardChannel<BlockInterconnectUpward>::DownwardChannel(const std::string &name, const InterconnectConfig &config):
	Channels::Downward<DownwardChannel<BlockInterconnectUpward>>{name},
	InterconnectChannelReceiver{name + ".Downward", config},
	isl_index{config.isl_index}
{
}


bool Rt::DownwardChannel<BlockInterconnectUpward>::onEvent(const Event& event)
{
	LOG(this->log_interconnect, LEVEL_ERROR,
	    "unknown event received %s\n",
	    event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<BlockInterconnectUpward>::onEvent(const NetSocketEvent& event)
{
	bool status = true;

	std::list<Message> messages;

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "NetSocket event received\n");

	// Receive messages
	if(!this->receive(event, messages))
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "error when receiving data on input channel\n");
		status = false;
	}
	// Iterate over received messages
	for(auto &&message : messages)
	{
		// Send message to the next block
		if(!this->enqueueMessage(message.release<void>(), message.type))
		{
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "failed to send message to next block\n");
			status = false;
		}
	}

	return status;
}


bool Rt::UpwardChannel<BlockInterconnectUpward>::onEvent(const Event &event)
{
	if (event == delay_timer)
	{
		onTimerEvent();
		return true;
	}

	LOG(this->log_interconnect, LEVEL_ERROR,
	    "unknown timer event received %s",
	    event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<BlockInterconnectUpward>::onEvent(const TimerEvent &event)
{
	LOG(this->log_interconnect, LEVEL_ERROR,
	    "unknown event received %s",
	    event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<BlockInterconnectUpward>::onEvent(const MessageEvent &event)
{
	Message message{event.getMessage<void>()};
	message.type = event.getMessageType();

	// Check if object inside
	if(!this->send(std::move(message)))
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "error when sending data\n");
		return false;
	}
	return true;
}


bool BlockInterconnectUpward::onInit()
{
	// Register log 
	this->log_interconnect = Output::Get()->registerLog(LEVEL_WARNING, "InterconnectUpward.block");
	return true;
}


bool Rt::UpwardChannel<BlockInterconnectUpward>::onInit()
{
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	std::string remote_addr("");

	auto Conf = OpenSandModelConf::Get();
	if(!Conf->getInterconnectCarrier(true, remote_addr, data_port, sig_port, stack, rmem, wmem, isl_index))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Entity infrastructure is missing interconnect data\n");
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	delay_timer = this->addTimerEvent(name + ".delay_timer", polling_rate);

	return true;
}


bool Rt::DownwardChannel<BlockInterconnectUpward>::onInit()
{
	std::string name="DownwardInterconnectChannel";
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	std::string remote_addr("");
	int32_t socket_event;

	auto Conf = OpenSandModelConf::Get();
	if(!Conf->getInterconnectCarrier(false, remote_addr, data_port, sig_port, stack, rmem, wmem, isl_index))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Entity infrastructure is missing interconnect data\n");
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	// Add NetSocketEvents
	socket_event = this->addNetSocketEvent(name + "_data",
	                                       this->data_channel->getChannelFd(),
	                                       MAX_SOCK_SIZE);
	if(socket_event < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add data socket event to Downward channel\n");
		return false;
	}
	socket_event = this->addNetSocketEvent(name + "_sig",
	                                       this->sig_channel->getChannelFd(),
	                                        MAX_SOCK_SIZE);
	if(socket_event < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add data socket event to Downward channel\n");
		return false;
	}

	return true;
}

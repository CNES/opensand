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

#include "BlockInterconnect.h"
#include "OpenSandModelConf.h"

#include <opensand_rt/MessageEvent.h>

constexpr double POLLING_RATE = 10.0;

BlockInterconnectDownward::BlockInterconnectDownward(const std::string &name,
                                                     const std::string &):
	Block(name)
{
}

BlockInterconnectDownward::~BlockInterconnectDownward()
{
}

void BlockInterconnectDownward::generateConfiguration()
{
}

bool BlockInterconnectDownward::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
    case EventType::Message:
		{
			auto msg_event = static_cast<const MessageEvent*>(event);
			rt_msg_t message = msg_event->getMessage();

			// Check if object inside
			if(!this->send(message))
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "error when sending data\n");
				return false;
			}
		}
		break;

		case EventType::Timer:
			if (event->getFd() == delay_timer)
			{
				onTimerEvent();
			}
			break;

		default:
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}
	return true;
}

bool BlockInterconnectDownward::Upward::onEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
    case EventType::NetSocket:
		{
			std::list<rt_msg_t> messages;

			LOG(this->log_interconnect, LEVEL_DEBUG,
			    "NetSocket event received\n");

			// Receive messages
			if(!this->receive((NetSocketEvent *)event, messages))
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "error when receiving data on input channel\n");
				status = false;
			}
			// Iterate over received messages
			for(std::list<rt_msg_t>::iterator it = messages.begin();
			    it != messages.end(); it++)
			{
				// Send message to the next block
				if(!this->enqueueMessage((void **)(&it->data),
				                          it->length, it->type))
				{
					LOG(this->log_interconnect, LEVEL_ERROR,
					    "failed to send message to next block\n");
					status = false;
				}
			}
		}
		break;

		default:
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "unknown event received %s\n",
			    event->getName().c_str());
			return false;
	}

	return status;
}

bool BlockInterconnectDownward::onInit(void)
{
	// Register log
	this->log_interconnect = Output::Get()->registerLog(LEVEL_WARNING, "InterconnectDownward.block");
	return true;
}

bool BlockInterconnectDownward::Upward::onInit(void)
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
	if(!Conf->getInterconnectCarrier(true, remote_addr, data_port, sig_port, stack, rmem, wmem))
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

bool BlockInterconnectDownward::Downward::onInit()
{
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	std::string remote_addr("");

	auto Conf = OpenSandModelConf::Get();
	if(!Conf->getInterconnectCarrier(false, remote_addr, data_port, sig_port, stack, rmem, wmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Entity infrastructure is missing interconnect data\n");
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	delay_timer = this->addTimerEvent(name + ".delay_timer", POLLING_RATE);

	return true;
}

BlockInterconnectUpward::BlockInterconnectUpward(const std::string &name,
                                                 const std::string &):
	Block(name)
{
}

BlockInterconnectUpward::~BlockInterconnectUpward()
{
}

void BlockInterconnectUpward::generateConfiguration()
{
}

bool BlockInterconnectUpward::Downward::onEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
		case EventType::NetSocket:
		{
			std::list<rt_msg_t> messages;

			LOG(this->log_interconnect, LEVEL_DEBUG,
			    "NetSocket event received\n");

			// Receive messages
			if(!this->receive((NetSocketEvent *)event, messages))
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "error when receiving data on input channel\n");
				status = false;
			}
			// Iterate over received messages
			for(std::list<rt_msg_t>::iterator it = messages.begin();
			    it != messages.end(); it++)
			{
				// Send message to the next block
				if(!this->enqueueMessage((void **)(&it->data),
				                         it->length, it->type))
				{
					LOG(this->log_interconnect, LEVEL_ERROR,
					    "failed to send message to next block\n");
					status = false;
				}
			}
		}
		break;

		default:
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "unknown event received %s\n",
			    event->getName().c_str());
			return false;
	}

	return status;
}

bool BlockInterconnectUpward::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case EventType::Message:
		{
			auto msg_event = static_cast<const MessageEvent*>(event);
			rt_msg_t message = msg_event->getMessage();

			// Check if object inside
			if(!this->send(message))
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "error when sending data\n");
				return false;
			}
		}
		break;

		case EventType::Timer:
			if (event->getFd() == delay_timer)
			{
				onTimerEvent();
			}
			break;

		default:
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}
	return true;
}

bool BlockInterconnectUpward::onInit(void)
{
	// Register log 
	this->log_interconnect = Output::Get()->registerLog(LEVEL_WARNING, "InterconnectUpward.block");
	return true;
}

bool BlockInterconnectUpward::Upward::onInit(void)
{
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	std::string remote_addr("");

	auto Conf = OpenSandModelConf::Get();
	if(!Conf->getInterconnectCarrier(true, remote_addr, data_port, sig_port, stack, rmem, wmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Entity infrastructure is missing interconnect data\n");
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	delay_timer = this->addTimerEvent(name + ".delay_timer", POLLING_RATE);

	return true;
}

bool BlockInterconnectUpward::Downward::onInit()
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
	if(!Conf->getInterconnectCarrier(false, remote_addr, data_port, sig_port, stack, rmem, wmem))
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

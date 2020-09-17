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

#include <opensand_old_conf/conf.h>

bool BlockInterconnectDownward::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			rt_msg_t message = ((MessageEvent *)event)->getMessage();

			// Check if object inside
			if(!this->send(message))
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "error when sending data\n");
				return false;
			}
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
		case evt_net_socket:
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
	string name="UpwardInterconnectChannel";
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	string remote_addr("");
	int32_t socket_event;

	// Get configuration
	// NOTE: this works now that only one division is made per component. If we
	// wanted to split one component in more than three, dedicated configuration
	// is needed, to tell different configurations apart.
	// get remote IP address
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                    INTERCONNECT_LOWER_IP, remote_addr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_LOWER_IP);
		return false;
	}
	// get data port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UPWARD_DATA_PORT, data_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UPWARD_DATA_PORT);
		return false;
	}
	// get sig port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UPWARD_SIG_PORT, sig_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UPWARD_SIG_PORT);
		return false;
	}
	// get UDP stack
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_STACK, stack))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_STACK);
		return false;
	}
	// get rmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_RMEM, rmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_RMEM);
		return false;
	}
	// get wmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_WMEM, wmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_WMEM);
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
	string remote_addr("");

	// Get configuration
	// NOTE: this works now that only one division is made per component. If we
	// wanted to split one component in more than three, dedicated configuration
	// is needed, to tell different configurations apart.
	// get remote IP address
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_LOWER_IP, remote_addr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_LOWER_IP);
		return false;
	}
	// get data port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_DOWNWARD_DATA_PORT, data_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_DOWNWARD_DATA_PORT);
		return false;
	}
	// get sig port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_DOWNWARD_SIG_PORT, sig_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_DOWNWARD_SIG_PORT);
		return false;
	}
	// get UDP stack
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_STACK, stack))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_STACK);
		return false;
	}
	// get rmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_RMEM, rmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_RMEM);
		return false;
	}
	// get wmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_WMEM, wmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_WMEM);
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	return true;
}

bool BlockInterconnectUpward::Downward::onEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
		case evt_net_socket:
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
		case evt_message:
		{
			rt_msg_t message = ((MessageEvent *)event)->getMessage();

			// Check if object inside
			if(!this->send(message))
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "error when sending data\n");
				return false;
			}
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
	string remote_addr("");

	// Get configuration
	// NOTE: this works now that only one division is made per component. If we
	// wanted to split one component in more than three, dedicated configuration
	// is needed, to tell different configurations apart.
	// get remote IP address
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UPPER_IP, remote_addr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UPPER_IP);
		return false;
	}
	// get data port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UPWARD_DATA_PORT, data_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UPWARD_DATA_PORT);
		return false;
	}
	// get sig port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UPWARD_SIG_PORT, sig_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UPWARD_SIG_PORT);
		return false;
	}
	// get UDP stack
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_STACK, stack))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_STACK);
		return false;
	}
	// get rmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_RMEM, rmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_RMEM);
		return false;
	}
	// get wmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_WMEM, wmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_WMEM);
		return false;
	}

	// Create channel
	this->initUdpChannels(data_port, sig_port, remote_addr, stack, rmem, wmem);

	return true;
}

bool BlockInterconnectUpward::Downward::onInit()
{
	string name="DownwardInterconnectChannel";
	unsigned int stack;
	unsigned int rmem;
	unsigned int wmem;
	unsigned int data_port;
	unsigned int sig_port;
	string remote_addr("");
	int32_t socket_event;

	// Get configuration
	// NOTE: this works now that only one division is made per component. If we
	// wanted to split one component in more than three, dedicated configuration
	// is needed, to tell different configurations apart.
	// get remote IP address
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UPPER_IP, remote_addr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UPPER_IP);
		return false;
	}
	// get data port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_DOWNWARD_DATA_PORT, data_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_DOWNWARD_DATA_PORT);
		return false;
	}
	// get data port
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_DOWNWARD_SIG_PORT, sig_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_DOWNWARD_SIG_PORT);
		return false;
	}
	// get UDP stack
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_STACK, stack))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_STACK);
		return false;
	}
	// get rmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_RMEM, rmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_RMEM);
		return false;
	}
	// get wmem
	if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
	                   INTERCONNECT_UDP_WMEM, wmem))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    INTERCONNECT_SECTION, INTERCONNECT_UDP_WMEM);
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

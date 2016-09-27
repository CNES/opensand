/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file BlockInterconnectDownward.cpp
 * @brief This bloc implements an interconnection block facing downwards.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */



#include <opensand_output/Output.h>

#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "interconnect_channel.h"

#include <unistd.h>
#include <signal.h>
#include <iostream>

/**
 * Constructor
 */
template <class T>
BlockInterconnectDownwardTpl<T>::BlockInterconnectDownwardTpl(const string &name,
										   struct icd_specific UNUSED(specific)):
	Block(name)
{
}

template <class T>
BlockInterconnectDownwardTpl<T>::~BlockInterconnectDownwardTpl()
{
}

template <class T>
template <class O>
bool BlockInterconnectDownwardTpl<T>::DownwardTpl<O>::onEvent(const RtEvent *const event)
{
    int ret;

	switch(event->getType())
	{
		case evt_message:
		{
			size_t total_len;
			rt_msg_t message = ((MessageEvent *)event)->getMessage();
			O *object = (O *) message.data;

			// Check if object inside
			if (message.length == 0 && message.type == 0)
			{
				message.type = msg_object;
				O::toInterconnect(object, 
								  (unsigned char **) &message.data,
								  total_len);
				message.length = total_len;
				// delete original object
				delete object;
			}

			//LOG(this->log_interconnect, LEVEL_DEBUG,
			//	"%lu-bytes message event received\n",
			//	message.length);

			ret = this->out_channel.sendPacket(message);
            if (ret > 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
					"error when sending data\n");
			}
            else if (ret < 0)
            {
				LOG(this->log_interconnect, LEVEL_ERROR,
					"Problem with connection\n");
                // close socket
				this->out_channel.close();
				// TODO: the block should notify the following block in the chain
				// to decide what to do (send message)
				// send message
				LOG(this->log_interconnect, LEVEL_ERROR,
					"terminating...\n");
				kill(getpid(), SIGTERM);
            }
			free(message.data);
		}
		break;
		case evt_tcp_listen:
		{
			if(this->out_channel.getFd() >= 0)
			{
				LOG(this->log_interconnect, LEVEL_WARNING,
						"connection with interconnect already established.");
				return true;
			}
			this->out_channel.setChannelSock(((TcpListenEvent *)event)->getSocketClient());
			// Set the socket to blocking, since TcpListenEvent sets it to non blocking
			this->out_channel.setSocketBlocking();
			LOG(this->log_interconnect, LEVEL_NOTICE,
				"event received on downward channel listen socket\n");
			LOG(this->log_interconnect, LEVEL_NOTICE,
				"InterconnectBlock downward channel is now connected\n");
			this->timer_event = this->addTimerEvent("DownwardInterconnectChannel",
													500.0);
			if (this->timer_event < 0)
			{
				LOG(this->log_init, LEVEL_ERROR,
					"Cannot add timer event to downward channel\n");
			}
			this->out_channel.flush();
		}
		break;
		case evt_timer:
		{
			// check if socket is open
            if (!this->out_channel.isOpen())
			{
                // close socket
				this->out_channel.close();
				// TODO: the block should notify the following block in the chain
				// to decide what to do (send message)
				// send message
				LOG(this->log_interconnect, LEVEL_ERROR,
					"terminating...\n");
				kill(getpid(), SIGTERM);
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

template <class T>
template <class O>
bool BlockInterconnectDownwardTpl<T>::UpwardTpl<O>::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_net_socket:
		{
			// Data to read in InterconnectChannel socket buffer
			O *object;
			unsigned char *buf = NULL;
			size_t length;
			uint8_t type;
			int ret;

			LOG(this->log_interconnect, LEVEL_DEBUG,
				"NetSocket event received\n");

			// store data in recv_buffer
			ret = this->in_channel.receive((NetSocketEvent *)event);
			if(ret < 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
					"failed to receive data on to "
					"receive buffer\n");
			}
			else
			{
				LOG(this->log_interconnect, LEVEL_DEBUG,
					"packets stored in buffer\n");
				// try to fech entire packets
				while(this->in_channel.getPacket(&buf, length, type))
				{
					// reconstruct object
					if (type == msg_object)
					{	
						O::newFromInterconnect(buf, length, &object);
						if(!this->enqueueMessage((void **)(&object)))
							LOG(this->log_interconnect, LEVEL_ERROR,
								"failed to send message upwards\n");
					}
					else
					{
						if(!this->enqueueMessage((void **) &buf, length, type))
							LOG(this->log_interconnect, LEVEL_ERROR,
								"failed to send message upwards\n");
						free(buf); // no need 'cause enqueueMessage sets NULL
					}
				}
			}
		}
		break;
		case evt_tcp_listen:
		{
			if(this->in_channel.getFd() >= 0)
			{
				LOG(this->log_interconnect, LEVEL_WARNING,
					"connection with interconnect already established.");
				return true;
			}
			this->in_channel.setChannelSock(((TcpListenEvent *)event)->getSocketClient());
            this->in_channel.setSocketBlocking();
			LOG(this->log_interconnect, LEVEL_NOTICE,
				"event received on downward channel listen socket\n");
			LOG(this->log_interconnect, LEVEL_NOTICE,
				"InterconnectBlock downward channel is now connected\n");
			// Add net socket event
			string name="UpwardInterconnectChannel";
			this->socket_event = this->addNetSocketEvent(name, this->in_channel.getFd());
			if (this->socket_event < 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
					"Cannot add event to Upward channel\n");
				return false;
			}
			this->timer_event = this->addTimerEvent("UpwardInterconnectChannel",
													500.0);
			if (this->timer_event < 0)
			{
				LOG(this->log_init, LEVEL_ERROR,
					"Cannot add timer event to Upward channel\n");
				return false;
			}
		}
		break;
		case evt_timer:
		{
			// check if socket is open
            if (!this->in_channel.isOpen())
			{
				// remove event
				this->removeEvent(this->socket_event);
				// close socket
				this->in_channel.close();
				// TODO: the block should notify the following block in the chain
				// to decide what to do (send message)
				// send message
				LOG(this->log_interconnect, LEVEL_ERROR,
					"terminating...\n");
				kill(getpid(), SIGTERM);
			}
		}
		break;

		default:
			LOG(this->log_interconnect, LEVEL_ERROR,
				"unknown event received %s\n", 
				event->getName().c_str());
			return false;
	}

	return true;
}

template <class T>
bool BlockInterconnectDownwardTpl<T>::onInit(void)
{
	// Register log
	this->log_interconnect = Output::registerLog(LEVEL_DEBUG, "InterconnectDownward.block");
	return true;
}

template <class T>
template <class O>
bool BlockInterconnectDownwardTpl<T>::UpwardTpl<O>::onInit(void)
{
	string name="UpwardInterconnectChannel";
	// Register log
	this->log_interconnect = Output::registerLog(LEVEL_DEBUG, "InterconnectDownward.upward");
	// Start Listening
	if(!this->in_channel.listen(this->port))
	{
		LOG(this->log_init, LEVEL_ERROR,
			"Cannot create listen socket\n");
		return false;
	}
	// Add TcpListenEvent
	if(this->addTcpListenEvent(name, this->in_channel.getListenFd()) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
			"Cannot add event to Upward channel\n");
		return false;
	}
	return true;
}

template <class T>
template <class O>
bool BlockInterconnectDownwardTpl<T>::DownwardTpl<O>::onInit()
{
	string name="DownwardInterconnectChannel";
	// Register log
	this->log_interconnect = Output::registerLog(LEVEL_DEBUG, "InterconnectDownward.downward");
	// Start listening
	if(!this->out_channel.listen(this->port))
	{
		LOG(this->log_init, LEVEL_ERROR,
			"Cannot connect to remote socket\n");
		return false;
	}
	// Add TcpListenEvent
	if(this->addTcpListenEvent(name, this->out_channel.getListenFd()) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
			"Cannot add event to Upward channel\n");
		return false;
	}
	return true;
}


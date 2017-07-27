/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file BlockInterconnectUpward.h
 * @brief This bloc implements an interconnection block facing upwards.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef BlockInterconnectUpward_H
#define BlockInterconnectUpward_H

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include "interconnect_channel.h"
#include "OpenSandFrames.h"
#include "DvbFrame.h"

#include <unistd.h>
#include <signal.h>

struct icu_specific
{
	string ip_addr; // IP of the remote BlockInterconnect
	uint16_t port_upward; // TCP port for the upward channel
	uint16_t port_downward; // TCP port for the downward channel
};

/**
 * @class BlockInterconnectUpward
 * @brief This bloc implements an interconnection block facing upwards
 */
template <class T = DvbFrame>
class BlockInterconnectUpwardTpl: public Block
{
 public:

	/**
	 * @brief The interconnect block, placed below
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	BlockInterconnectUpwardTpl(const string &name,
	                           struct icu_specific specific):
		Block(name)
	{};

	~BlockInterconnectUpwardTpl() {};

	template <class O = T>
	class UpwardTpl: public RtUpward
	{
	 public:
		UpwardTpl(const string &name, struct icu_specific specific):
			RtUpward(name),
			ip_addr(specific.ip_addr),
			port(specific.port_upward),
			out_channel(false,true)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the IP address of the remote BlockInterconnect
		string ip_addr;
		/// the port of the socket created by the Block above
		uint16_t port;
		/// TCP out channel
		interconnect_channel out_channel;
		// Output log 
		OutputLog *log_interconnect;
		// the timer event
		int32_t timer_event;
	};
	typedef UpwardTpl<> Upward;

	template <class O = T>
	class DownwardTpl: public RtDownward
	{
	 public:
		DownwardTpl(const string &name, struct icu_specific specific):
			RtDownward(name),
			ip_addr(specific.ip_addr),
			port(specific.port_downward),
			in_channel(true,false)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the IP address of the remote BlockInterconnect
		string ip_addr;
		/// the port of the socket created by the Block above
		uint16_t port;
		/// TCP in channel
		interconnect_channel in_channel;
		// Output log 
		OutputLog *log_interconnect;
		// The signal event 
		int32_t socket_event;
		// The timer event
		int32_t timer_event;
	};
	typedef DownwardTpl<> Downward;

 protected:
	// Output log 
	OutputLog *log_interconnect;

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();
};

typedef BlockInterconnectUpwardTpl<> BlockInterconnectUpward;

template <class T>
template <class O>
bool BlockInterconnectUpwardTpl<T>::DownwardTpl<O>::onEvent(const RtEvent *const event)
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
							    "failed to send message downwards\n");
					}
					else
					{
						if(!this->enqueueMessage((void **) &buf, length, type))
							LOG(this->log_interconnect, LEVEL_ERROR,
							    "failed to send message downwards\n");
						free(buf); //no need 'cause enqueueMessage sets NULL
					}
				}
			}
		}
		break;
		case evt_timer:
		{
			// check if socket is opened
			if (!this->in_channel.isOpen())
			{
				// remove event
				this->removeEvent(this->socket_event);
				// close socket
				this->in_channel.close();
				// TODO: the block should notify the following block in the chain
				// to decide what to do (send message) 
				// send message
				LOG(this->log_interconnect, LEVEL_INFO,
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
bool BlockInterconnectUpwardTpl<T>::UpwardTpl<O>::onEvent(const RtEvent *const event)
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

			LOG(this->log_interconnect, LEVEL_DEBUG,
			    "%lu-bytes message event received \n", message.length);

			ret = this->out_channel.sendPacket(message);
			if (ret > 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "error when sending data\n");
			}
			else if (ret < 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "Problem with connection...\n");
				// close socket
				this->out_channel.close();
				// TODO: the block should notify the following block in the chain
				// to decide what to do (send message) 
				// send message
				LOG(this->log_interconnect, LEVEL_INFO,
				    "terminating...\n");
				kill(getpid(), SIGTERM);
			}
			free(message.data);
		}
		break;
		case evt_timer:
		{
			// check if socket is opened
			if (!this->out_channel.isOpen())
			{
				// close socket
				this->out_channel.close();
				// TODO: the block should notify the following block in the chain
				// to decide what to do (send message) 
				// send message
				LOG(this->log_interconnect, LEVEL_INFO,
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
bool BlockInterconnectUpwardTpl<T>::onInit(void)
{
	// Register log	
	this->log_interconnect = Output::registerLog(LEVEL_WARNING, "InterconnectUpward.block");
	return true;
}

template <class T>
template <class O>
bool BlockInterconnectUpwardTpl<T>::UpwardTpl<O>::onInit(void)
{
	int n_retries = 0;
	string name="UpwardInterconnectChannel";
	// Register log
	this->log_interconnect = Output::registerLog(LEVEL_WARNING, "InterconnectUpward.upward");
	// Connect in_channel to BlockInterconnectDownward
	while(!this->out_channel.connect(this->ip_addr, this->port))
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "Cannot connect to remote socket. Retrying...\n");
		if(n_retries++ == 5)
			break;
		sleep(5);
	}
	if(n_retries>5)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot connect to remote socket. Abort.\n");
		return false;
	}
	// Add timer event to check on connection
	this->timer_event = this->addTimerEvent(name, 500.0);
	if (this->timer_event < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add timer event to Downward channel\n");
		return false;
	}
	return true;
}

template <class T>
template <class O>
bool BlockInterconnectUpwardTpl<T>::DownwardTpl<O>::onInit()
{
	int n_retries = 0;
	string name="DownwardInterconnectChannel";
	// Register log
	this->log_interconnect = Output::registerLog(LEVEL_WARNING, "InterconnectUpward.downward");
	// Connect in_channel to BlockInterconnectDownward
	// Try a few times
	while(!this->in_channel.connect(this->ip_addr, this->port))
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "Cannot connect to remote socket. Retrying...\n");
		if(n_retries++ == 5)
			break;
		sleep(5);
	}
	if(n_retries>5)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot connect to remote socket. Abort.\n");
		return false;
	}
	this->in_channel.setSocketBlocking();
	// Add TcpSocketEvent
	this->socket_event = this->addNetSocketEvent(name, this->in_channel.getFd());
	if (this->socket_event < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add event to Downward channel\n");
		return false;
	}
	// Add timer event to check on connection
	this->timer_event = this->addTimerEvent(name, 500.0);
	if (this->timer_event < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add timer event to Downward channel\n");
		return false;
	}
	return true;
}

#endif

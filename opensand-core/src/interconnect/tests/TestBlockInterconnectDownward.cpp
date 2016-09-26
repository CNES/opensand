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


#include "TestBlockInterconnectDownward.h"

#include <opensand_output/Output.h>

#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "interconnect_channel_test.h"
#include "TestInterconnectBlock.h"
#include <fcntl.h>

/**
 * Constructor
 */
TestBlockInterconnectDownward::TestBlockInterconnectDownward(const string &name,
                                                     struct icd_specific specific):
	Block(name),
    top_block(specific.top_block)
{
}

TestBlockInterconnectDownward::~TestBlockInterconnectDownward()
{
}

void TestBlockInterconnectDownward::notifyIfReady() const
{
    if ( (((Upward *)this->upward)->isReady() ) &&
            (((Downward *)this->downward)->isReady() ) )
    {
        ((TopBlock *)this->top_block)->startReading();
    }
}

// TODO remove this functions once every channels will be correctly separated
bool TestBlockInterconnectDownward::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}

bool TestBlockInterconnectDownward::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
        case evt_tcp_listen:
        {
            int flags;

            this->out_channel.setChannelSock(((TcpListenEvent *)event)->getSocketClient());
            printf("event received on downward channel listen socket\n");
            printf("InterconnectBlock downward channel is now connected\n");
            // Set new socket on blocking mode
            flags = fcntl(this->out_channel.getFd(), F_GETFL);
            if(fcntl(this->out_channel.getFd(), F_SETFL, flags & (~O_NONBLOCK)))
            {
                printf("failed to set the socket on blocking mode\n");
            }
            // Add net socket event
            string name="UpwardInterconnectChannel";
            if(this->addNetSocketEvent(name, this->out_channel.getFd()) < 0)
            {
                printf("Cannot add event to Upward channel\n");
                return false;
            }
            this->out_channel.flush();
    
            // Tell top block if ready (ONLY TEST)
            ((TestBlockInterconnectDownward *)this->block)->notifyIfReady();
                
        }
        break;
        case evt_message:
		{
            rt_msg_t message = ((MessageEvent *)event)->getMessage();

            if (DEBUG)
			    printf("%lu-bytes message event received\n", message.length);

            if(!this->out_channel.isConnected())
            {
				printf("InterConnect channel is not connected\n");
            } 

			if(!this->out_channel.send((const unsigned char *)message.data,
			                           message.length))
			{
				printf("error when sending data\n");
			}
            free(message.data);
		}
		break;

		default:
			printf("unknown event received %s",
			    event->getName().c_str());
			return false;
	}
	return true;
}


bool TestBlockInterconnectDownward::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}

bool TestBlockInterconnectDownward::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_net_socket:
		{
			// Data to read in InterconnectChannel socket buffer
			unsigned char *buf = NULL;
            size_t length;
			int ret;

            if (DEBUG)
			    printf("NetSocket event received\n");

            // store data in recv_buffer
			ret = this->in_channel.receive((NetSocketEvent *)event);
            if(ret < 0)
            {
                printf("failed to receive data on to receive buffer");
            }
            else
            {
                if (DEBUG)
                    printf("data received\n");
                // try to fech entire packet
                if (DEBUG)
                    printf("D: used space %zu \n", this->in_channel.getUsedSpace());
                while(this->in_channel.getPacket(&buf, length))
                {
                    if (DEBUG)
                        printf("D:%zu packet received, now %zu used\n", length, this->in_channel.getUsedSpace());
                    if (DEBUG)
                        printf("packet received\n");
                    this->enqueueMessage((void **) &buf, length);
                }
            }
        }
		break;
        case evt_tcp_listen:
        {
            int flags;

            this->in_channel.setChannelSock(((TcpListenEvent *)event)->getSocketClient());
            printf("event received on downward channel listen socket\n");
            printf("InterconnectBlock downward channel is now connected\n");
            // Set new socket on blocking mode
            flags = fcntl(this->in_channel.getFd(), F_GETFL);
            if(fcntl(this->in_channel.getFd(), F_SETFL, flags & (~O_NONBLOCK)))
            {
                printf("failed to set the socket on blocking mode\n");
            }
            // Add net socket event
            string name="UpwardInterconnectChannel";
            if(this->addNetSocketEvent(name, this->in_channel.getFd()) < 0)
            {
                printf("Cannot add event to Upward channel\n");
                return false;
            }
            
            // Tell top block if ready (ONLY TEST)
            ((TestBlockInterconnectDownward *)this->block)->notifyIfReady();
        }
        break;

		default:
			printf("unknown event received %s\n", 
			    event->getName().c_str());
			return false;
	}

	return true;
}


bool TestBlockInterconnectDownward::onInit(void)
{
	return true;
}

bool TestBlockInterconnectDownward::Upward::onInit(void)
{
    string name="UpwardInterconnectChannel";
    // Start Listening
    if(!this->in_channel.listen(this->port))
    {
        printf("Cannot create listen socket\n");
        return false;
    }
    // Add TcpListenEvent
    if(this->addTcpListenEvent(name, this->in_channel.getListenFd()) < 0)
    {
        printf("Cannot add event to Upward channel\n");
        return false;
    }
    return true;
}

bool TestBlockInterconnectDownward::Downward::onInit()
{
    string name="DownwardInterconnectChannel";
    // Start Listening
    if(!this->out_channel.listen(this->port))
    {
        printf("Cannot create listen socket\n");
        return false;
    }
    // Add TcpListenEvent
    if(this->addTcpListenEvent(name, this->out_channel.getListenFd()) < 0)
    {
        printf("Cannot add event to Downward channel\n");
        return false;
    }
    return true;
}


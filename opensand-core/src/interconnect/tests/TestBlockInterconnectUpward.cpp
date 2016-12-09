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
 * @file BlockInterconnectUpward.cpp
 * @brief This bloc implements an interconnection block facing upwards.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "TestBlockInterconnectUpward.h"

#include <opensand_output/Output.h>

#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "interconnect_channel_test.h"
#include <signal.h>
#include <unistd.h>

/**
 * Constructor
 */
TestBlockInterconnectUpward::TestBlockInterconnectUpward(const string &name,
                                                 struct icu_specific UNUSED(specific)):
	Block(name)
{
}

TestBlockInterconnectUpward::~TestBlockInterconnectUpward()
{
}


// TODO remove this functions once every channels will be correctly separated
bool TestBlockInterconnectUpward::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}

bool TestBlockInterconnectUpward::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_net_socket:
		{
			// Data to read in InterconnectChannel socket buffer
			unsigned char *buf = NULL;
            size_t length;
			int ret;

            this->last_t = 0;

            if (DEBUG)
			    printf("NetSocket event received\n");

            // store data in recv_buffer
			ret = this->in_channel.receive((NetSocketEvent *)event);
            if(ret < 0)
            {
                printf("failed to receive data on to receive buffer\n");
            }
            else
            {
                if (DEBUG)
                    printf("data received\n");
                // try to fech entire packets
                if (DEBUG)
                    printf("U: used space %zu \n", this->in_channel.getUsedSpace());
                while(this->in_channel.getPacket(&buf, length))
                {
                    if (DEBUG)
                        printf("U:%zu packet received, now %zu used\n", length, this->in_channel.getUsedSpace());
                    if (DEBUG)
                        printf("packet received\n");
                    this->enqueueMessage((void **) &buf, length);
                }
            }
        }
		break;
        case evt_timer:
        {
            // kill PID: consider connection terminated if haven't received data
            // during 1 sec (ONLY TEST!)
            if (this->last_t++ > 10)
            {
                printf("Connection terminated. Kill PID.");
                kill(getpid(), SIGTERM);
            }
        }
		break;

		default:
			printf("unknown event received %s",
			    event->getName().c_str());
			return false;
	}
	return true;
}


bool TestBlockInterconnectUpward::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}

bool TestBlockInterconnectUpward::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
        case evt_message:
		{
            rt_msg_t message = ((MessageEvent *)event)->getMessage();

            if (DEBUG)
                printf("%lu-bytes message event received\n", message.length);
            
            if(!this->out_channel.isConnected())
            {
                printf("InterConnect channel is not connected\n");
                return false;
            }

            if(!this->out_channel.send((const unsigned char *)message.data,
                                        message.length))
            {
                printf("error when sending data \n");
            }
            free(message.data);
		}
        break;

		default:
			printf("unknown event received %s\n", 
			    event->getName().c_str());
			return false;
	}

	return true;
}


bool TestBlockInterconnectUpward::onInit(void)
{
	return true;
}

bool TestBlockInterconnectUpward::Upward::onInit(void)
{
    string name="UpwardInterconnectChannel";
    // Connect out_channel to BlockInterconnectDownward
    if(!this->out_channel.connect(this->ip_addr, this->port))
    {
        printf("Cannot connect to remote socket\n");
        return false;
    }
    if(this->addNetSocketEvent(name, this->out_channel.getFd()) < 0)
    {
        printf("Cannot add event to Upward channel\n");
        return false;
    }
    return true;
}

bool TestBlockInterconnectUpward::Downward::onInit()
{
    string name="DownwardInterconnectChannel";
    // Connect out_channel to BlockInterconnectDownward
    if(!this->in_channel.connect(this->ip_addr, this->port))
    {
        printf("Cannot connect to remote socket\n");
        return false;
    }
    if(this->addNetSocketEvent(name, this->in_channel.getFd()) < 0)
    {
        printf("Cannot add event to Downward channel\n");
        return false;
    }
    // Add timer event to see if channel was disconnected
    if(this->addTimerEvent("DownwardTimer", (double) 100) < 0)
    {
        printf("Cannot add Timer event\n");
        return false;
    }
    return true;
}


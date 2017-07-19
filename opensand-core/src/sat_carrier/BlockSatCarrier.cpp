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
 * @file BlockSatCarrier.cpp
 * @brief This bloc implements a satellite carrier emulation.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "BlockSatCarrier.h"

#include <opensand_output/Output.h>

#include "DvbFrame.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"

/**
 * Constructor
 */
BlockSatCarrier::BlockSatCarrier(const string &name,
                                 struct sc_specific UNUSED(specific)):
	Block(name)
{
}

BlockSatCarrier::~BlockSatCarrier()
{
}


bool BlockSatCarrier::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			LOG(this->log_receive, LEVEL_DEBUG,
			    "%u-bytes %s message event received\n",
			    dvb_frame->getMessageLength(),
			    event->getName().c_str());

			if(!this->out_channel_set.send(dvb_frame->getCarrierId(),
			                               dvb_frame->getData().c_str(),
			                               dvb_frame->getTotalLength()))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "error when sending data\n");
			}
			delete dvb_frame;
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}
	return true;
}

bool BlockSatCarrier::Upward::onEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
		case evt_net_socket:
		{
			// Data to read in Sat_Carrier socket buffer
			size_t length;
			unsigned char *buf = NULL;

			unsigned int carrier_id;
			spot_id_t spot_id;
			int ret;

			LOG(this->log_receive, LEVEL_DEBUG,
			    "FD event received\n");

			// for UDP we need to retrieve potentially desynchronized
			// datagrams => loop on receive function
			do
			{
				ret = this->in_channel_set.receive((NetSocketEvent *)event,
				                                    carrier_id,
				                                    spot_id,
				                                    &buf, length);
				if(ret < 0)
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to receive data on any "
					    "input channel (code = %zu)\n", length);
					status = false;
				}
				else
				{
					LOG(this->log_receive, LEVEL_DEBUG,
					    "%zu bytes of data received on carrier ID %u\n",
					    length, carrier_id);

					if(length > 0)
					{
						this->onReceivePktFromCarrier(carrier_id, spot_id,  
						                              buf, length);
					}
				}
			} while(ret > 0);
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s\n", 
			    event->getName().c_str());
			return false;
	}

	return status;
}


bool BlockSatCarrier::onInit(void)
{
	return true;
}

bool BlockSatCarrier::Upward::onInit(void)
{
	vector<sat_carrier_udp_channel *>::iterator it;
	sat_carrier_udp_channel *channel;

	// initialize all channels from the configuration file
	if(!this->in_channel_set.readInConfig(this->ip_addr,
	                                      this->interface_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Wrong channel set configuration\n");
		return false;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for(it = this->in_channel_set.begin(); it != this->in_channel_set.end(); it++)
	{
		channel = *it;

		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			ostringstream name;

			LOG(this->log_init, LEVEL_NOTICE,
			    "Listen on fd %d for channel %d\n",
			    channel->getChannelFd(), channel->getChannelID());
			name << "Channel_" << channel->getChannelID();
			this->addNetSocketEvent(name.str(),
			                        channel->getChannelFd(),
			                        MSG_BBFRAME_SIZE_MAX);
		}
	}
	return true;
}

bool BlockSatCarrier::Downward::onInit()
{
	// initialize all channels from the configuration file
	if(!this->out_channel_set.readOutConfig(this->ip_addr,
	                                        this->interface_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Wrong channel set configuration\n");
		return false;
	}
	return true;
}


void BlockSatCarrier::Upward::onReceivePktFromCarrier(uint8_t carrier_id,
                                                      spot_id_t spot_id,
                                                      unsigned char *data,
                                                      size_t length)
{
	DvbFrame *dvb_frame = new DvbFrame(data, length);
	free(data);

	dvb_frame->setCarrierId(carrier_id);
	dvb_frame->setSpot(spot_id);
	
	if(!this->enqueueMessage((void **)(&dvb_frame)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send frame from carrier %u to upper layer\n",
		    carrier_id);
		goto release;
	}

	LOG(this->log_receive, LEVEL_DEBUG,
	    "Message from carrier %u sent to upper layer\n", carrier_id);

	return;

release:
	delete dvb_frame;
}

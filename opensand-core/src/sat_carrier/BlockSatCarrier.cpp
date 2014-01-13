/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 */


// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_SAT_CARRIER
#include <opensand_conf/uti_debug.h>

#include "BlockSatCarrier.h"

#include "DvbFrame.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"

/**
 * Constructor
 */
BlockSatCarrier::BlockSatCarrier(const string &name,
                                 struct sc_specific specific):
	Block(name),
	ip_addr(specific.ip_addr),
	interface_name(specific.emu_iface)
{
	// TODO we need a mutex here because some parameters may be used in upward and downward
	this->enableChannelMutex();
}

BlockSatCarrier::~BlockSatCarrier()
{
}


bool BlockSatCarrier::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			UTI_DEBUG_L3("%u-bytes %s message event received\n",
			              dvb_frame->getMessageLength(),
			              event->getName().c_str());

			if(!m_channelSet.send(dvb_frame->getCarrierId(),
			                      dvb_frame->getData().c_str(),
			                      dvb_frame->getTotalLength()))
			{
				UTI_ERROR("error when sending data\n");
			}
			delete dvb_frame;
		}
		break;

		default:
			UTI_ERROR("unknown event received %s", event->getName().c_str());
			return false;
	}
	return true;
}


bool BlockSatCarrier::onUpwardEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
		case evt_net_socket:
		{
			// Data to read in Sat_Carrier socket buffer
			size_t l_lg;
			unsigned char *l_buf = NULL;

			unsigned int carrier_id;
			int ret;

			UTI_DEBUG_L3("FD event received\n");

			// for UDP we need to retrieve potentially desynchronized
			// datagrams => loop on receive function
			do
			{
				ret = this->m_channelSet.receive((NetSocketEvent *)event,
				                                 carrier_id,
				                                 &l_buf, l_lg);
				if(ret < 0)
				{
					UTI_ERROR("failed to receive data on any "
					          "input channel (code = %zu)\n", l_lg);
					status = false;
				}
				else
				{
					UTI_DEBUG_L3("%zu bytes of data received on carrier ID %u\n",
					             l_lg, carrier_id);

					if(l_lg > 0)
					{
						this->onReceivePktFromCarrier(carrier_id, l_buf, l_lg);
					}
				}
			} while(ret > 0);
		}
		break;

		default:
			UTI_ERROR("unknown event received %s", event->getName().c_str());
			return false;
	}

	return status;
}


bool BlockSatCarrier::onInit()
{
	return true;
}

bool BlockSatCarrier::Upward::onInit()
{
	std::vector < sat_carrier_channel * >::iterator it;
	sat_carrier_channel *channel;

	// initialize all channels from the configuration file
	if(m_channelSet.readConfig(this->ip_addr,
	                           this->interface_name) < 0)
	{
		UTI_ERROR("Wrong channel set configuration\n");
		return false;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for(it = m_channelSet.begin(); it != m_channelSet.end(); it++)
	{
		channel = *it;

		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			ostringstream name;

			UTI_INFO("Listen on fd %d for channel %d\n",
			         channel->getChannelFd(), channel->getChannelID());
			name << "Channel_" << channel->getChannelFd();
			this->upward->addNetSocketEvent(name.str(),
			                                channel->getChannelFd(),
			                                MSG_BBFRAME_SIZE_MAX);
		}
	}

	return true;
}

void BlockSatCarrier::onReceivePktFromCarrier(uint8_t carrier_id,
                                              unsigned char *data,
                                              size_t length)
{
	DvbFrame *dvb_frame = new DvbFrame(data, length);
	free(data);

	dvb_frame->setCarrierId(carrier_id);

	if(!this->sendUp((void **)(&dvb_frame)))
	{
		UTI_ERROR("failed to send frame from carrier %u to upper layer\n",
		          carrier_id);
		goto release_meta;
	}

	UTI_DEBUG("Message from carrier %u sent to upper layer", carrier_id);

	return;

release_meta:
	delete dvb_frame;
	free(data);
}

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
 * @file BlockSatCarrier.cpp
 * @brief This bloc implements a satellite carrier emulation.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "BlockSatCarrier.h"

#include <opensand_rt/MessageEvent.h>
#include <opensand_rt/NetSocketEvent.h>
#include <opensand_output/Output.h>

#include "DvbFrame.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"


/**
 * Constructor
 */
Rt::UpwardChannel<BlockSatCarrier>::UpwardChannel(const std::string &name, sc_specific specific):
	Channels::Upward<UpwardChannel<BlockSatCarrier>>{name},
	ip_addr{std::move(specific.ip_addr)},
	tal_id{specific.tal_id},
	in_channel_set{specific.tal_id},
	destination_host{specific.destination_host},
	spot_id{specific.spot_id}
{
}


Rt::DownwardChannel<BlockSatCarrier>::DownwardChannel(const std::string &name, sc_specific specific):
	Channels::Downward<DownwardChannel<BlockSatCarrier>>{name},
	ip_addr{std::move(specific.ip_addr)},
	tal_id{specific.tal_id},
	out_channel_set{specific.tal_id},
	destination_host{specific.destination_host},
	spot_id{specific.spot_id}
{
}


bool Rt::DownwardChannel<BlockSatCarrier>::onEvent(const Event& event)
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "unknown event received %s",
	    event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<BlockSatCarrier>::onEvent(const MessageEvent& event)
{
	Rt::Ptr<DvbFrame> dvb_frame = event.getMessage<DvbFrame>();

	LOG(this->log_receive, LEVEL_DEBUG,
	    "%u-bytes %s message event received\n",
	    dvb_frame->getMessageLength(),
	    event.getName().c_str());

	if(!this->out_channel_set.send(dvb_frame->getCarrierId(),
	                               dvb_frame->getRawData(),
	                               dvb_frame->getTotalLength()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "error when sending data\n");
	}
	return true;
}


bool Rt::UpwardChannel<BlockSatCarrier>::onEvent(const Event &event)
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "unknown event received %s\n", 
	    event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<BlockSatCarrier>::onEvent(const NetSocketEvent &event)
{
	LOG(this->log_receive, LEVEL_DEBUG, "FD event received\n");

	// for UDP we need to retrieve potentially desynchronized
	// datagrams => loop on receive function
	int ret;
	bool status = true;
	do
	{
		// Data to read in Sat_Carrier socket buffer
		spot_id_t spot_id;
		unsigned int carrier_id;
		Ptr<Data> buf = make_ptr<Data>(nullptr);
		ret = this->in_channel_set.receive(event, carrier_id, spot_id, buf);
		if(ret < 0)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to receive data on any "
			    "input channel (code = %zu)\n",
			    buf->length());
			status = false;
		}
		else
		{
			LOG(this->log_receive, LEVEL_DEBUG,
			    "%zu bytes of data received on carrier ID %u\n",
			    buf->length(), carrier_id);

			if(buf->length() > 0)
			{
				this->onReceivePktFromCarrier(carrier_id, spot_id, std::move(buf));
			}
		}
	} while(ret > 0);

	return status;
}


bool Rt::UpwardChannel<BlockSatCarrier>::onInit()
{
	// initialize all channels from the configuration file
	if(!this->in_channel_set.readInConfig(this->ip_addr, destination_host, spot_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Wrong channel set configuration\n");
		return false;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for(auto &&channel : this->in_channel_set)
	{
		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			std::ostringstream name;

			LOG(this->log_init, LEVEL_NOTICE,
			    "Listen on fd %d for channel %d\n",
			    channel->getChannelFd(),
			    channel->getChannelID());
			name << "Channel_" << channel->getChannelID();
			this->addNetSocketEvent(name.str(),
			                        channel->getChannelFd(),
			                        MSG_BBFRAME_SIZE_MAX + 1); // consider byte used for sequencing
		}
	}
	return true;
}

bool Rt::DownwardChannel<BlockSatCarrier>::onInit()
{
	// initialize all channels from the configuration file
	if(!this->out_channel_set.readOutConfig(this->ip_addr, destination_host, spot_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Wrong channel set configuration\n");
		return false;
	}
	return true;
}


void Rt::UpwardChannel<BlockSatCarrier>::onReceivePktFromCarrier(uint8_t carrier_id,
                                                                 spot_id_t spot_id,
																 Ptr<Data> data)
{
	Ptr<DvbFrame> dvb_frame = make_ptr<DvbFrame>(*data);

	dvb_frame->setCarrierId(carrier_id);
	dvb_frame->setSpot(spot_id);

	if (!this->enqueueMessage(std::move(dvb_frame), to_underlying(InternalMessageType::unknown)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send frame from carrier %u to upper layer\n",
		    carrier_id);
	}
	else
	{
		LOG(this->log_receive, LEVEL_DEBUG,
		    "Message from carrier %u sent to upper layer\n", carrier_id);
	}
}

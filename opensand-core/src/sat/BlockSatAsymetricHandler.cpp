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
 * @file BlockSatAsymetricHandler.cpp
 * @brief Block that routes control messages to the DVB stack and data messages to
 *        whatever regen level is requested by the user.
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 */


#include <opensand_output/Output.h>
#include <opensand_rt/MessageEvent.h>

#include "OpenSandFrames.h"
#include "BlockSatAsymetricHandler.h"
#include "DvbFrame.h"


constexpr uint8_t DATA_IN_ST_ID = 6;
constexpr uint8_t DATA_OUT_ST_ID = 7;
constexpr uint8_t DATA_IN_GW_ID = 8;
constexpr uint8_t DATA_OUT_GW_ID = 9;


BlockSatAsymetricHandler::BlockSatAsymetricHandler(const std::string& name, AsymetricConfig):
	Block{name}
{
}


bool BlockSatAsymetricHandler::onInit()
{
	return true;
}


BlockSatAsymetricHandler::Upward::Upward(const std::string& name, AsymetricConfig specific):
	RtUpwardDemux<bool>{name},
	split_traffic{specific.is_transparent}
{
}


bool BlockSatAsymetricHandler::Upward::onEvent(const RtEvent *const event)
{
	if (event->getType() != EventType::Message)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Wrong event type received. Only messages are expected by this block.");
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);
	auto frame = static_cast<DvbFrame *>(msg_event->getData());
	const uint8_t id = frame->getCarrierId() % 10;
	const bool is_data = (id == DATA_IN_GW_ID) || (id == DATA_IN_ST_ID);

	if (!this->enqueueMessage(split_traffic && is_data,
	                          (void**)&frame,
	                          msg_event->getLength(),
	                          msg_event->getMessageType()))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send data to upper layer");
		delete frame;
		return false;
	}
	return true;
}


BlockSatAsymetricHandler::Downward::Downward(const std::string& name, AsymetricConfig specific):
	GroundPhysicalChannel{specific.phy_config},
	RtDownwardMux{name},
	is_regenerated_traffic{!specific.is_transparent}
{
}


bool BlockSatAsymetricHandler::Downward::onEvent(const RtEvent *const event)
{
	if (event->getType() != EventType::Message)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Wrong event type received. Only messages are expected by this block.");
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);
	auto frame = static_cast<DvbFrame *>(msg_event->getData());
	const uint8_t id = frame->getCarrierId() % 10;
	const bool is_control = (id != DATA_OUT_GW_ID) && (id != DATA_OUT_ST_ID);
	if ((is_control || this->is_regenerated_traffic) && IsCnCapableFrame(frame->getMessageType()))
	{
		frame->setCn(this->getCurrentCn());
	}
	return this->forwardPacket(frame);
}


bool BlockSatAsymetricHandler::Downward::forwardPacket(DvbFrame *dvb_frame)
{
	// Send frame to lower layer
	if (!this->enqueueMessage((void **)&dvb_frame, 0, to_underlying(InternalMessageType::unknown)))
	{
		LOG(this->log_send, LEVEL_ERROR, 
		    "Failed to send burst of packets to lower layer");
		delete dvb_frame;
		return false;
	}
	return true;	
}

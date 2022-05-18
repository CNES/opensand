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
 * @file BlockTransp.cpp
 * @brief
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#include "BlockTransp.h"

BlockTransp::BlockTransp(const std::string &name):
    Block(name) {}

BlockTransp::Upward::Upward(const std::string &name):
    RtUpwardMux(name) {}

bool BlockTransp::Upward::onEvent(const RtEvent *const event)
{
	if (event->getType() != EventType::Message)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
		    event->getName().c_str());
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);

	if (to_enum<InternalMessageType>(msg_event->getMessageType()) != InternalMessageType::msg_data)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected message received: %s",
		    msg_event->getName().c_str());
		return false;
	}

	auto frame = static_cast<const DvbFrame *>(msg_event->getData());
	return handleDvbFrame(std::unique_ptr<const DvbFrame>(frame));
}

bool BlockTransp::Upward::handleDvbFrame(std::unique_ptr<const DvbFrame> frame)
{
	LOG(log_send, LEVEL_INFO, "Sending a DvbFrame to the opposite channel");

	auto frame_ptr = frame.release();
	bool ok = shareMessage((void **)&frame_ptr, sizeof(DvbFrame), to_underlying(InternalMessageType::msg_data));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to transmit message to the opposite channel");
		delete frame_ptr;
		return false;
	}
	return true;
}

BlockTransp::Downward::Downward(const std::string &name):
    RtDownwardDemux<SatDemuxKey>(name) {}

bool BlockTransp::Downward::onEvent(const RtEvent *const event)
{
	if (event->getType() != EventType::Message)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
		    event->getName().c_str());
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);

	if (to_enum<InternalMessageType>(msg_event->getMessageType()) != InternalMessageType::msg_data)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected message received: %s",
		    msg_event->getName().c_str());
		return false;
	}

	auto frame = static_cast<DvbFrame *>(msg_event->getData());
	return handleDvbFrame(std::unique_ptr<DvbFrame>(frame));
}

bool BlockTransp::Downward::handleDvbFrame(std::unique_ptr<DvbFrame> frame)
{
	LOG(log_receive, LEVEL_INFO, "Received a message (carrier id %d, msg type %d)",
	    frame->getCarrierId(), frame->getMessageType());
	auto spot_id = frame->getSpot();
	uint8_t id = frame->getCarrierId() % 10;
	if(id % 2 != 0) {
		LOG(this->log_receive, LEVEL_ERROR,
		    "Received a message from an output carried id (%d)", frame->getCarrierId());
		return false;
	}

	Component dest = (id == 4 || id == 8) ? Component::terminal : Component::gateway;
	// add one to the input carrier id to get the corresponding output carrier id
	frame->setCarrierId(frame->getCarrierId() + 1);
	return sendToLowerBlock({spot_id, dest}, std::move(frame));
}

bool BlockTransp::Downward::sendToLowerBlock(SatDemuxKey key, std::unique_ptr<const DvbFrame> frame)
{
	LOG(log_send, LEVEL_INFO, "Sending a DvbFrame to the lower block, %s side", key.dest == Component::gateway ? "GW" : "ST");
	auto frame_ptr = frame.release();
	bool ok = enqueueMessage(key, (void **)&frame_ptr, sizeof(DvbFrame), to_underlying(InternalMessageType::msg_data));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to transmit message to the opposite channel");
		delete frame_ptr;
		return false;
	}
	return true;
}

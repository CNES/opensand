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

#include "OpenSandModelConf.h"
#include <opensand_rt/MessageEvent.h>

BlockTransp::BlockTransp(const std::string &name, TranspConfig transp_config):
    Block(name), entity_id{transp_config.entity_id} {}

bool BlockTransp::onInit()
{
	auto conf = OpenSandModelConf::Get();
	auto downward = dynamic_cast<Downward *>(this->downward);
	auto upward = dynamic_cast<Upward *>(this->upward);

	auto handled_spots = conf->getSpotsByEntity(entity_id);
	upward->handled_spots = handled_spots;
	downward->handled_spots = std::move(handled_spots);
	return true;
}

BlockTransp::Upward::Upward(const std::string &name, TranspConfig transp_config):
    RtUpwardMux(name), isl_enabled{transp_config.isl_enabled} {}

bool BlockTransp::Upward::onEvent(const RtEvent *const event)
{
	if (event->getType() != EventType::Message)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
		    event->getName().c_str());
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);
	auto frame = static_cast<DvbFrame *>(msg_event->getData());
	return handleDvbFrame(std::unique_ptr<DvbFrame>(frame));
}

bool BlockTransp::Upward::handleDvbFrame(std::unique_ptr<DvbFrame> frame)
{
	auto spot_id = frame->getSpot();
	auto carrier_id = frame->getCarrierId();
	auto log_level = carrier_id % 10 >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level, "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	if (!isl_enabled)
	{
		return sendToOppositeChannel(std::move(frame));
	}

	if (handled_spots.find(spot_id) == handled_spots.end())
	{
		// send by ISL
		spot_id_t old_spot_id = carrier_id / 10;
		frame->setSpot(spot_id);
		frame->setCarrierId(carrier_id + 10 * (spot_id - old_spot_id));
		return sendToUpperBlock(std::move(frame));
	}
	else
	{
		return sendToOppositeChannel(std::move(frame));
	}
	// Unreachable
	return true;
}

bool BlockTransp::Upward::sendToUpperBlock(std::unique_ptr<const DvbFrame> frame)
{
	LOG(log_send, LEVEL_INFO, "Sending a DvbFrame to the upper block");
	auto frame_ptr = frame.release();
	bool ok = enqueueMessage((void **)&frame_ptr, sizeof(DvbFrame), to_underlying(InternalMessageType::msg_data));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the upper block");
		delete frame_ptr;
		return false;
	}
	return true;
}

bool BlockTransp::Upward::sendToOppositeChannel(std::unique_ptr<const DvbFrame> frame)
{
	auto log_level = frame->getCarrierId() % 10 >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_send, log_level, "Sending a DvbFrame to the opposite channel");
	auto frame_ptr = frame.release();
	bool ok = shareMessage((void **)&frame_ptr, sizeof(DvbFrame), to_underlying(InternalMessageType::msg_data));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		delete frame_ptr;
		return false;
	}
	return true;
}

BlockTransp::Downward::Downward(const std::string &name, TranspConfig transp_config):
    RtDownwardDemux<SatDemuxKey>(name), isl_enabled{transp_config.isl_enabled} {}

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
	auto spot_id = frame->getSpot();
	auto carrier_id = frame->getCarrierId();
	auto log_level = carrier_id % 10 >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level, "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	if (handled_spots.find(spot_id) == handled_spots.end())
	{
		// Forward by ISL
		return sendToOppositeChannel(std::move(frame));
	}
	else
	{
		uint8_t id = carrier_id % 10;
		if (id % 2 != 0)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Received a message from an output carried id (%d)", carrier_id);
			return false;
		}

		Component dest = (id == 4 || id == 8) ? Component::terminal : Component::gateway;
		// add one to the input carrier id to get the corresponding output carrier id
		frame->setCarrierId(carrier_id + 1);
		return sendToLowerBlock({spot_id, dest}, std::move(frame));
	}
}

bool BlockTransp::Downward::sendToLowerBlock(SatDemuxKey key, std::unique_ptr<const DvbFrame> frame)
{
	auto log_level = frame->getCarrierId() % 10 >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_send, log_level, "Sending a DvbFrame to the lower block, %s side", key.dest == Component::gateway ? "GW" : "ST");
	auto frame_ptr = frame.release();
	bool ok = enqueueMessage(key, (void **)&frame_ptr, sizeof(DvbFrame), to_underlying(InternalMessageType::msg_data));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the lower block");
		delete frame_ptr;
		return false;
	}
	return true;
}

bool BlockTransp::Downward::sendToOppositeChannel(std::unique_ptr<const DvbFrame> frame)
{
	auto log_level = frame->getCarrierId() % 10 >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_send, log_level, "Sending a DvbFrame to the opposite channel");
	auto frame_ptr = frame.release();
	bool ok = shareMessage((void **)&frame_ptr, sizeof(DvbFrame), to_underlying(InternalMessageType::msg_data));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		delete frame_ptr;
		return false;
	}
	return true;
}

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

constexpr uint8_t DATA_IN_GW_ID = 8;
constexpr uint8_t CTRL_IN_GW_ID = 4;

BlockTransp::BlockTransp(const std::string &name, TranspConfig transp_config):
    Block(name),
    entity_id{transp_config.entity_id},
    isl_enabled{transp_config.isl_enabled} {}

bool BlockTransp::onInit()
{
	const auto conf = OpenSandModelConf::Get();
	auto downward = dynamic_cast<Downward *>(this->downward);
	auto upward = dynamic_cast<Upward *>(this->upward);

	std::unordered_map<SpotComponentPair, tal_id_t> routes;
	for (auto &&spot: conf->getSpotsTopology())
	{
		const SpotTopology &topo = spot.second;
		routes[{topo.spot_id, Component::gateway}] = topo.sat_id_gw;
		routes[{topo.spot_id, Component::terminal}] = topo.sat_id_st;

		// Check that ISL are enabled when they should be
		if (topo.sat_id_gw != topo.sat_id_st &&
		    (topo.sat_id_gw == entity_id || topo.sat_id_st == entity_id) &&
		    !isl_enabled)
		{
			LOG(log_init, LEVEL_ERROR,
			    "The gateway of the spot %d is connected to sat %d and the "
			    "terminals are connected to sat %d, but no ISL is configured on sat %d",
			    topo.spot_id, topo.sat_id_gw, topo.sat_id_st, entity_id);
			return false;
		}
	}
	upward->routes = routes;
	downward->routes = routes;
	return true;
}

BlockTransp::Upward::Upward(const std::string &name, TranspConfig transp_config):
    RtUpwardMux(name),
    entity_id{transp_config.entity_id} {}

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
	const spot_id_t spot_id = frame->getSpot();
	const uint8_t carrier_id = frame->getCarrierId();
	const uint8_t id = carrier_id % 10;
	const log_level_t log_level = id >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level, "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	const Component dest = (id == CTRL_IN_GW_ID || id == DATA_IN_GW_ID) ? Component::terminal : Component::gateway;

	const auto dest_sat_id_it = routes.find({spot_id, dest});
	if (dest_sat_id_it == routes.end())
	{
		LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
		    dest == Component::gateway ? "GW" : "ST", spot_id);
		return false;
	}
	const tal_id_t dest_sat_id = dest_sat_id_it->second;

	if (dest_sat_id == entity_id)
	{
		return sendToOppositeChannel(std::move(frame));
	}
	else
	{
		// send by ISL
		return sendToUpperBlock(std::move(frame));
	}
	// Unreachable
	return true;
}

bool BlockTransp::Upward::sendToUpperBlock(std::unique_ptr<const DvbFrame> frame)
{
	auto msg_type = frame->getCarrierId() % 10 >= 6 ? InternalMessageType::encap_data : InternalMessageType::sig;
	const auto log_level = msg_type == InternalMessageType::encap_data ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_send, log_level, "Sending a DvbFrame to the upper block");
	const auto frame_ptr = frame.release();
	const bool ok = enqueueMessage((void **)&frame_ptr, sizeof(DvbFrame), to_underlying(msg_type));
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
	auto msg_type = frame->getCarrierId() % 10 >= 6 ? InternalMessageType::encap_data : InternalMessageType::sig;
	const auto log_level = msg_type == InternalMessageType::encap_data ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_send, log_level, "Sending a DvbFrame to the opposite channel");
	const auto frame_ptr = frame.release();
	const bool ok = shareMessage((void **)&frame_ptr, sizeof(DvbFrame), to_underlying(msg_type));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		delete frame_ptr;
		return false;
	}
	return true;
}

BlockTransp::Downward::Downward(const std::string &name, TranspConfig transp_config):
    RtDownwardDemux<SpotComponentPair>(name),
    entity_id{transp_config.entity_id} {}

bool BlockTransp::Downward::onEvent(const RtEvent *const event)
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

bool BlockTransp::Downward::handleDvbFrame(std::unique_ptr<DvbFrame> frame)
{
	const spot_id_t spot_id = frame->getSpot();
	const uint8_t carrier_id = frame->getCarrierId();
	const uint8_t id = carrier_id % 10;
	const log_level_t log_level = id >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level, "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	const Component dest = (id == CTRL_IN_GW_ID || id == DATA_IN_GW_ID) ? Component::terminal : Component::gateway;

	const auto dest_sat_id_it = routes.find({spot_id, dest});
	if (dest_sat_id_it == routes.end())
	{
		LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
		    dest == Component::gateway ? "GW" : "ST", spot_id);
		return false;
	}
	const tal_id_t dest_sat_id = dest_sat_id_it->second;

	if (dest_sat_id == entity_id)
	{
		if (id % 2 != 0)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Received a message from an output carried id (%d)", carrier_id);
			return false;
		}

		// add one to the input carrier id to get the corresponding output carrier id
		frame->setCarrierId(carrier_id + 1);
		return sendToLowerBlock({spot_id, dest}, std::move(frame));
	}
	else
	{
		// send by ISL
		return sendToOppositeChannel(std::move(frame));
	}
}

bool BlockTransp::Downward::sendToLowerBlock(SpotComponentPair key, std::unique_ptr<const DvbFrame> frame)
{
	auto msg_type = frame->getCarrierId() % 10 >= 6 ? InternalMessageType::encap_data : InternalMessageType::sig;
	const auto log_level = msg_type == InternalMessageType::encap_data ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_send, log_level, "Sending a DvbFrame to the lower block, %s side", key.dest == Component::gateway ? "GW" : "ST");
	const auto frame_ptr = frame.release();
	const bool ok = enqueueMessage(key, (void **)&frame_ptr, sizeof(DvbFrame), to_underlying(msg_type));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the lower block (%s, spot %d)",
		    key.dest == Component::gateway ? "GW" : "ST", key.spot_id);
		delete frame_ptr;
		return false;
	}
	return true;
}

bool BlockTransp::Downward::sendToOppositeChannel(std::unique_ptr<const DvbFrame> frame)
{
	auto msg_type = frame->getCarrierId() % 10 >= 6 ? InternalMessageType::encap_data : InternalMessageType::sig;
	const auto log_level = msg_type == InternalMessageType::encap_data ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_send, log_level, "Sending a DvbFrame to the opposite channel");
	const auto frame_ptr = frame.release();
	const bool ok = shareMessage((void **)&frame_ptr, sizeof(DvbFrame), to_underlying(msg_type));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		delete frame_ptr;
		return false;
	}
	return true;
}

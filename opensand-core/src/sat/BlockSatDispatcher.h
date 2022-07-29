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
 * @file BlockSatDispatcher.h
 * @brief Block that routes DvbFrames or NetBursts to the right lower stack or to ISL
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#ifndef BLOCK_SAT_DISPATCHER_H
#define BLOCK_SAT_DISPATCHER_H

#include <memory>

#include <opensand_rt/Rt.h>
#include <opensand_rt/RtChannelDemux.h>
#include <opensand_rt/RtChannelMux.h>
#include <unordered_set>

#include "DvbFrame.h"
#include "NetBurst.h"
#include "SpotComponentPair.h"

struct SatDispatcherConfig
{
	tal_id_t entity_id;

	// If true, the messages for spots that are not handled by 
	// this satellite will be sent to the upper block
	bool isl_enabled;
};

/**
 * @class BlockSatDispatcher
 * @brief Block that routes DvbFrames or NetBursts to the right lower stack or to ISL
 */
class BlockSatDispatcher: public Block
{
  public:
	BlockSatDispatcher(const std::string &name, SatDispatcherConfig config);

	bool onInit();

	class Upward: public RtUpwardMux
	{
	  public:
		Upward(const std::string &name, SatDispatcherConfig config);

	  private:
		friend class BlockSatDispatcher;

		bool onEvent(const RtEvent *const event) override;
		bool handleDvbFrame(std::unique_ptr<DvbFrame> frame);
		bool handleNetBurst(std::unique_ptr<NetBurst> burst);

		template <typename T>
		bool sendToUpperBlock(std::unique_ptr<T> msg, InternalMessageType msg_type);
		template <typename T>
		bool sendToOppositeChannel(std::unique_ptr<T> msg, InternalMessageType msg_type);

		tal_id_t entity_id;
		std::unordered_map<SpotComponentPair, tal_id_t> routes;
		std::unordered_map<tal_id_t, spot_id_t> spot_by_entity;
		std::unordered_map<SpotComponentPair, RegenLevel> regen_levels;
	};

	class Downward: public RtDownwardDemux<SpotComponentPair>
	{
	  public:
		Downward(const std::string &name, SatDispatcherConfig config);

	  private:
		friend class BlockSatDispatcher;

		bool onEvent(const RtEvent *const event) override;
		bool handleDvbFrame(std::unique_ptr<DvbFrame> frame);
		bool handleNetBurst(std::unique_ptr<NetBurst> burst);

		template <typename T>
		bool sendToLowerBlock(SpotComponentPair key, std::unique_ptr<T> msg, InternalMessageType msg_type);
		template <typename T>
		bool sendToOppositeChannel(std::unique_ptr<T> msg, InternalMessageType msg_type);

		tal_id_t entity_id;
		std::unordered_map<SpotComponentPair, tal_id_t> routes;
		std::unordered_map<SpotComponentPair, RegenLevel> regen_levels;
		std::unordered_map<tal_id_t, spot_id_t> spot_by_entity;
	};

  private:
	tal_id_t entity_id;
	bool isl_enabled;
};

template <typename T>
bool BlockSatDispatcher::Upward::sendToUpperBlock(std::unique_ptr<T> msg, InternalMessageType msg_type)
{
	const auto log_level = msg_type == InternalMessageType::sig ? LEVEL_DEBUG : LEVEL_INFO;
	LOG(log_send, log_level, "Sending a message to the upper block");
	const auto msg_ptr = msg.release();
	const bool ok = enqueueMessage((void **)&msg_ptr, sizeof(T), to_underlying(msg_type));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the upper block");
		delete msg_ptr;
		return false;
	}
	return true;
}

template <typename T>
bool BlockSatDispatcher::Upward::sendToOppositeChannel(std::unique_ptr<T> msg, InternalMessageType msg_type)
{
	const auto log_level = msg_type == InternalMessageType::sig ? LEVEL_DEBUG : LEVEL_INFO;
	LOG(log_send, log_level, "Sending a message to the opposite channel");
	const auto msg_ptr = msg.release();
	const bool ok = shareMessage((void **)&msg_ptr, sizeof(T), to_underlying(msg_type));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		delete msg_ptr;
		return false;
	}
	return true;
}

template <typename T>
bool BlockSatDispatcher::Downward::sendToLowerBlock(SpotComponentPair key, std::unique_ptr<T> msg, InternalMessageType msg_type)
{
	const auto log_level = msg_type == InternalMessageType::sig ? LEVEL_DEBUG : LEVEL_INFO;
	LOG(log_send, log_level, "Sending a message to the lower block, %s side", key.dest == Component::gateway ? "GW" : "ST");
	const auto msg_ptr = msg.release();
	const bool ok = enqueueMessage(key, (void **)&msg_ptr, sizeof(T), to_underlying(msg_type));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the lower block (%s, spot %d)",
		    key.dest == Component::gateway ? "GW" : "ST", key.spot_id);
		delete msg_ptr;
		return false;
	}
	return true;
}

template <typename T>
bool BlockSatDispatcher::Downward::sendToOppositeChannel(std::unique_ptr<T> msg, InternalMessageType msg_type)
{
	const auto log_level = msg_type == InternalMessageType::sig ? LEVEL_DEBUG : LEVEL_INFO;
	LOG(log_send, log_level, "Sending a message to the opposite channel");
	const auto msg_ptr = msg.release();
	const bool ok = shareMessage((void **)&msg_ptr, sizeof(T), to_underlying(msg_type));
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		delete msg_ptr;
		return false;
	}
	return true;
}

#endif

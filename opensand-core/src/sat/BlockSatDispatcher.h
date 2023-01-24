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

#include <unordered_set>
#include <memory>

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannelMuxDemux.h>

#include "DvbFrame.h"
#include "SpotComponentPair.h"


class NetBurst;


struct SatDispatcherConfig
{
	tal_id_t entity_id;

	// If true, the messages for spots that are not handled by 
	// this satellite will be sent to the upper block
	bool isl_enabled;
};


class SpotByEntity
{
public:
	SpotByEntity();

	void addEntityInSpot(tal_id_t entity, spot_id_t spot);

	void setDefaultSpot(spot_id_t spot);

	spot_id_t getSpotForEntity(tal_id_t entity) const;

private:
	std::unordered_map<tal_id_t, spot_id_t> spot_by_entity;
	spot_id_t default_spot;
};


template<>
class Rt::UpwardChannel<class BlockSatDispatcher>: public Channels::UpwardMuxDemux<UpwardChannel<BlockSatDispatcher>, IslComponentPair>
{
 public:
	UpwardChannel(const std::string &name, SatDispatcherConfig config);

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const MessageEvent &event) override;

	void initDispatcher(const SpotByEntity& spot_by_entity,
	                    const std::unordered_map<SpotComponentPair, tal_id_t> &routes,
	                    const std::unordered_map<SpotComponentPair, RegenLevel> &regen_levels);

 private:
	friend class BlockSatDispatcher;

	bool handleDvbFrame(Ptr<DvbFrame> frame);
	bool handleNetBurst(Ptr<NetBurst> burst);

	bool sendToUpperBlock(IslComponentPair key, Ptr<void> msg, InternalMessageType msg_type);
	bool sendToOppositeChannel(Ptr<void> msg, InternalMessageType msg_type);

	tal_id_t entity_id;
	SpotByEntity spot_by_entity;
	std::unordered_map<SpotComponentPair, tal_id_t> routes;
	std::unordered_map<SpotComponentPair, RegenLevel> regen_levels;
};


template<>
class Rt::DownwardChannel<class BlockSatDispatcher>: public Channels::DownwardMuxDemux<DownwardChannel<BlockSatDispatcher>, RegenerativeSpotComponent>
{
 public:
	DownwardChannel(const std::string &name, SatDispatcherConfig config);

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const MessageEvent &event) override;

	void initDispatcher(const SpotByEntity& spot_by_entity,
	                    const std::unordered_map<SpotComponentPair, tal_id_t> &routes,
	                    const std::unordered_map<SpotComponentPair, RegenLevel> &regen_levels);

 private:
	friend class BlockSatDispatcher;

	bool handleDvbFrame(Ptr<DvbFrame> frame);
	bool handleNetBurst(Ptr<NetBurst> burst);

	bool sendToLowerBlock(RegenerativeSpotComponent key, Ptr<void> msg, InternalMessageType msg_type);
	bool sendToOppositeChannel(Ptr<void> msg, InternalMessageType msg_type);

	tal_id_t entity_id;
	SpotByEntity spot_by_entity;
	std::unordered_map<SpotComponentPair, tal_id_t> routes;
	std::unordered_map<SpotComponentPair, RegenLevel> regen_levels;
};


/**
 * @class BlockSatDispatcher
 * @brief Block that routes DvbFrames or NetBursts to the right lower stack or to ISL
 */
class BlockSatDispatcher: public Rt::Block<BlockSatDispatcher, SatDispatcherConfig>
{
 public:
	BlockSatDispatcher(const std::string &name, SatDispatcherConfig config);

	bool onInit() override;

 private:
	tal_id_t entity_id;
	bool isl_enabled;
};


#endif

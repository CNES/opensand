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
 * @file BlockSatAsymetricHandler.h
 * @brief Block that routes control messages to the DVB stack and data messages to
 *        whatever regen level is requested by the user.
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 */


#ifndef BLOCK_SAT_ASYMETRIC_HANDLER
#define BLOCK_SAT_ASYMETRIC_HANDLER


#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannelMux.h>
#include <opensand_rt/RtChannelDemux.h>

#include "GroundPhysicalChannel.h"


struct AsymetricConfig
{
	bool upward_transparent;
	bool downward_transparent;
	PhyLayerConfig phy_config;
};


template<>
class Rt::UpwardChannel<class BlockSatAsymetricHandler>: public Channels::UpwardDemux<UpwardChannel<BlockSatAsymetricHandler>, bool>
{
 public:
	UpwardChannel(const std::string& name, AsymetricConfig specific);

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const MessageEvent &event) override;

 private:
	bool split_traffic;
};


template<>
class Rt::DownwardChannel<class BlockSatAsymetricHandler>: public GroundPhysicalChannel, public Channels::DownwardMux<DownwardChannel<BlockSatAsymetricHandler>>
{
 public:
	DownwardChannel(const std::string& name, AsymetricConfig specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const TimerEvent &event) override;
	bool onEvent(const MessageEvent &event) override;

 private:
	bool forwardPacket(Ptr<DvbFrame> frame) override;

	bool is_regenerated_traffic;
};


class BlockSatAsymetricHandler : public Rt::Block<BlockSatAsymetricHandler, AsymetricConfig>
{
 public:
	using Rt::Block<BlockSatAsymetricHandler, AsymetricConfig>::Block;
};


#endif  /* BLOCK_SAT_ASYMETRIC_HANDLER */

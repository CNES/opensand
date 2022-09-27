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


#include <opensand_rt/Rt.h>
#include <opensand_rt/RtChannelMux.h>
#include <opensand_rt/RtChannelDemux.h>

#include "GroundPhysicalChannel.h"


struct AsymetricConfig
{
	bool is_transparent;
	PhyLayerConfig phy_config;
};


class BlockSatAsymetricHandler : public Block
{
public:
	BlockSatAsymetricHandler(const std::string& name, AsymetricConfig specific);

	bool onInit();

	class Upward : public RtUpwardDemux<bool>
	{
	public:
		Upward(const std::string& name, AsymetricConfig specific);

	private:
		bool onEvent(const RtEvent *const event) override;

		bool split_traffic;
	};

	class Downward : public GroundPhysicalChannel, public RtDownwardMux
	{
	public:
		Downward(const std::string& name, AsymetricConfig specific);

		bool onInit() override;

	private:
		bool onEvent(const RtEvent *const event) override;
		bool forwardPacket(DvbFrame *frame) override;

		bool is_regenerated_traffic;
	};
};


#endif  /* BLOCK_SAT_ASYMETRIC_HANDLER */

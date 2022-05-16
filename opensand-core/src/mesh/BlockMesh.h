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
 * @file BlockMesh.h
 * @brief Block that handles mesh or star architecture on satellites
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#ifndef BLOCK_MESH_H
#define BLOCK_MESH_H

#include "NetBurst.h"
#include "OpenSandModelConf.h"
#include "SatDemuxKey.h"
#include "UdpChannel.h"
#include <memory>
#include <opensand_rt/Rt.h>
#include <string>
#include <unordered_set>

/**
 * @class BlockMesh
 * @brief Block that handles mesh or star architecture on satellites
 */
class BlockMesh: public Block
{
  public:
	BlockMesh(const std::string &name, tal_id_t sat_id);

	class Upward: public RtUpwardMux
	{
	  public:
		Upward(const std::string &name, tal_id_t sat_id);

	  private:
		friend class BlockMesh;

		bool onInit() override;
		bool onEvent(const RtEvent *const event) override;
		bool handleNetBurst(std::unique_ptr<const NetBurst> burst);
		bool sendToOppositeChannel(std::unique_ptr<const NetBurst> burst);
		bool sendViaIsl(std::unique_ptr<const NetBurst> burst);

		OpenSandModelConf::carrier_socket isl_out;
		bool mesh_architecture;
		tal_id_t default_entity;
		std::unordered_set<tal_id_t> handled_entities;
		std::unique_ptr<UdpChannel> isl_out_channel;
	};

	class Downward: public RtDownwardDemux<SatDemuxKey>
	{
	  public:
		Downward(const std::string &name, tal_id_t sat_id);

	  private:
		friend class BlockMesh;

		bool onInit() override;
		bool onEvent(const RtEvent *const event) override;
		bool handleMessageEvent(const MessageEvent *event);
		bool handleNetSocketEvent(NetSocketEvent *event);
		bool handleNetBurst(std::unique_ptr<const NetBurst> burst);
		bool sendToOppositeChannel(std::unique_ptr<const NetBurst> burst);
		bool sendToLowerBlock(SatDemuxKey dest, std::unique_ptr<const NetBurst> burst);

		OpenSandModelConf::carrier_socket isl_in;
		bool mesh_architecture;
		tal_id_t default_entity;
		std::unordered_set<tal_id_t> handled_entities;
		std::unique_ptr<UdpChannel> isl_in_channel;
	};

  private:
	bool onInit() override;

	tal_id_t entity_id;
};

#endif

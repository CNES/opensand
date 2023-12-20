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
 * @file BlockSatDispatcher.cpp
 * @brief Block that routes DvbFrames or NetBursts to the right lower stack or to ISL
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */


#include <tuple>

#include <opensand_rt/MessageEvent.h>

#include "BlockSatDispatcher.h"

#include "OpenSandModelConf.h"
#include "CarrierType.h"
#include "NetBurst.h"


constexpr uint8_t DATA_IN_GW_ID = 8;
constexpr uint8_t CTRL_IN_GW_ID = 4;


SpotByEntity::SpotByEntity():
	spot_by_entity{},
	default_spot{0}
{
	OpenSandModelConf::Get()->getDefaultSpotId(default_spot);
}


void SpotByEntity::addEntityInSpot(tal_id_t entity, spot_id_t spot)
{
	spot_by_entity[entity] = spot;
}


void SpotByEntity::setDefaultSpot(spot_id_t spot)
{
	default_spot = spot;
}


spot_id_t SpotByEntity::getSpotForEntity(tal_id_t entity) const
{
	auto found_spot = spot_by_entity.find(entity);
	if (found_spot == spot_by_entity.end())
	{
		return default_spot;
	}
	return found_spot->second;
}


BlockSatDispatcher::BlockSatDispatcher(const std::string &name, SatDispatcherConfig config):
	Rt::Block<BlockSatDispatcher, SatDispatcherConfig>{name, config},
	entity_id{config.entity_id},
	isl_enabled{config.isl_enabled}
{
}


bool BlockSatDispatcher::onInit()
{
	const auto conf = OpenSandModelConf::Get();

	SpotByEntity spot_by_entity;
	std::unordered_map<SpotComponentPair, tal_id_t> routes;
	std::unordered_map<SpotComponentPair, RegenLevel> regen_levels;

	for (auto &&spot: conf->getSpotsTopology())
	{
		const SpotTopology &topo = spot.second;

		spot_by_entity.addEntityInSpot(topo.gw_id, topo.spot_id);
		for (tal_id_t tal_id: topo.st_ids)
		{
			spot_by_entity.addEntityInSpot(tal_id, topo.spot_id);
		}

		routes[{topo.spot_id, Component::gateway}] = topo.sat_id_gw;
		routes[{topo.spot_id, Component::terminal}] = topo.sat_id_st;

		regen_levels[{topo.spot_id, Component::terminal}] = topo.forward_regen_level;
		regen_levels[{topo.spot_id, Component::gateway}] = topo.return_regen_level;

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

		LOG(log_init, LEVEL_NOTICE,
		    "Configured routes on spot #%d: spot id %d connected to terminals and gateways.",
		    spot.first, topo.spot_id);
	}

	upward.initDispatcher(spot_by_entity, routes, regen_levels);
	downward.initDispatcher(spot_by_entity, routes, regen_levels);
	return true;
}


Rt::UpwardChannel<BlockSatDispatcher>::UpwardChannel(const std::string &name, SatDispatcherConfig config):
	Channels::UpwardMuxDemux<UpwardChannel<BlockSatDispatcher>, IslComponentPair>{name},
	entity_id{config.entity_id}
{
}


void Rt::UpwardChannel<BlockSatDispatcher>::initDispatcher(const SpotByEntity& spot_by_entity,
                                                           const std::unordered_map<SpotComponentPair, tal_id_t> &routes,
                                                           const std::unordered_map<SpotComponentPair, RegenLevel> &regen_levels)
{
	this->spot_by_entity = spot_by_entity;
	this->routes = routes;
	this->regen_levels = regen_levels;
}


bool Rt::UpwardChannel<BlockSatDispatcher>::onEvent(const Event &event)
{
	LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
	    event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<BlockSatDispatcher>::onEvent(const MessageEvent &event)
{
	auto msg_type = event.getMessageType();
	LOG(log_receive, LEVEL_DEBUG, "Message received (type: %d)", msg_type);
	switch (to_enum<InternalMessageType>(msg_type))
	{
		// sent by SatCarrier
		case InternalMessageType::unknown:
		case InternalMessageType::sig:
		case InternalMessageType::encap_data:
		{
			return handleDvbFrame(event.getMessage<DvbFrame>());
		}
		// sent by Encap
		case InternalMessageType::decap_data:
		{
			return handleNetBurst(event.getMessage<NetBurst>());
		}
		case InternalMessageType::link_up:
		{
			bool success = true;
			auto link_up_msg = event.getMessage<T_LINK_UP>();
			for (auto&& [key, regen_level] : regen_levels)
			{
				if (regen_level == RegenLevel::IP)
				{
					Ptr<T_LINK_UP> link_up_copy = make_ptr<T_LINK_UP>();
					link_up_copy->group_id = link_up_msg->group_id;
					link_up_copy->tal_id = link_up_msg->tal_id;

					IslComponentPair link_up_key{
						.connected_sat = routes.at(key),
						.is_data_channel = true,
					};
					if (!this->enqueueMessage(link_up_key,
					                          std::move(link_up_copy),
					                          to_underlying(InternalMessageType::link_up)))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "cannot forward 'link up' message\n");
						success = false;
					}
				}
			}
			return success;
		}
		default:
			LOG(log_receive, LEVEL_ERROR,
			    "Unexpected message type received: %d",
			    event.getMessageType());
			return false;
	}
}


bool Rt::UpwardChannel<BlockSatDispatcher>::handleDvbFrame(Ptr<DvbFrame> frame)
{
	const spot_id_t spot_id = frame->getSpot();
	const uint8_t carrier_id = frame->getCarrierId();
	const auto carrier_type = extractCarrierType(carrier_id);
	const auto msg_type = isDataCarrier(carrier_type) ? InternalMessageType::encap_data : InternalMessageType::sig;
	const log_level_t log_level = isDataCarrier(carrier_type) ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level,
	    "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	const Component dest = isGatewayCarrier(carrier_type) ? Component::terminal : Component::gateway;

	const auto dest_sat_id_it = routes.find({spot_id, dest});
	if (dest_sat_id_it == routes.end())
	{
		LOG(log_receive, LEVEL_ERROR,
		    "No route found for %s in spot %d",
		    dest == Component::gateway ? "GW" : "ST", spot_id);
		return false;
	}
	const tal_id_t dest_sat_id = dest_sat_id_it->second;

	if (dest_sat_id == entity_id)
	{
		return sendToOppositeChannel(std::move(frame), msg_type);
	}
	else
	{
		// send by ISL
		IslComponentPair key{
			.connected_sat = dest_sat_id,
			.is_data_channel = false,
		};
		return sendToUpperBlock(key, std::move(frame), msg_type);
	}
}


bool Rt::UpwardChannel<BlockSatDispatcher>::handleNetBurst(Ptr<NetBurst> in_burst)
{
	// Separate the packets by destination
	std::unordered_map<SpotComponentPair, Ptr<NetBurst>> bursts{};
	for (auto &&pkt: *in_burst)
	{
		const auto dest_id = pkt->getDstTalId();
		const auto src_id = pkt->getSrcTalId();
		const spot_id_t spot_id = spot_by_entity.getSpotForEntity(src_id);
		LOG(log_receive, LEVEL_INFO, "Received a NetBurst (%d->%d, spot_id %d)", src_id, dest_id, spot_id);

		Component dest = OpenSandModelConf::Get()->getEntityType(dest_id);
		SpotComponentPair spot_dest{spot_id, dest};
		auto burst_it = bursts.find(spot_dest);
		if (burst_it == bursts.end())
		{
			auto result = bursts.emplace(spot_dest, make_ptr<NetBurst>());
			burst_it = result.first;
		}
		burst_it->second->push_back(std::move(pkt));
	}

	// Send all bursts to their respective destination
	bool ok = true;
	for (auto &&[dest, burst]: bursts)
	{
		const auto dest_sat_id_it = routes.find(dest);
		if (dest_sat_id_it == routes.end())
		{
			LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
			    dest.dest == Component::gateway ? "GW" : "ST", dest.spot_id);
			ok = false;
			continue;
		}

		const tal_id_t dest_sat_id = dest_sat_id_it->second;
		auto regen_level = regen_levels.at(dest);
		if (dest_sat_id == entity_id && regen_level != RegenLevel::IP)
		{
			ok &= sendToOppositeChannel(std::move(burst), InternalMessageType::decap_data);
		}
		else
		{
			// send by ISL or to LanAdaptation for IP regen
			IslComponentPair key{
				.connected_sat = dest_sat_id,
				.is_data_channel = regen_level == RegenLevel::IP,
			};
			ok &= sendToUpperBlock(key, std::move(burst), InternalMessageType::decap_data);
		}
	}
	return ok;
}


bool Rt::UpwardChannel<BlockSatDispatcher>::sendToUpperBlock(IslComponentPair key,
                                                             Ptr<void> msg,
                                                             InternalMessageType msg_type)
{
	LOG(log_send, LEVEL_INFO, "Sending a message to the upper block");
	if (!enqueueMessage(key, std::move(msg), to_underlying(msg_type)))
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the upper block");
		return false;
	}
	return true;
}


bool Rt::UpwardChannel<BlockSatDispatcher>::sendToOppositeChannel(Ptr<void> msg, InternalMessageType msg_type)
{
	const auto log_level = msg_type == InternalMessageType::sig ? LEVEL_DEBUG : LEVEL_INFO;
	LOG(log_send, log_level, "Sending a message to the opposite channel");
	if (!shareMessage(std::move(msg), to_underlying(msg_type)))
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		return false;
	}
	return true;
}


Rt::DownwardChannel<BlockSatDispatcher>::DownwardChannel(const std::string &name, SatDispatcherConfig config):
	Channels::DownwardMuxDemux<DownwardChannel<BlockSatDispatcher>, RegenerativeSpotComponent>{name},
	entity_id{config.entity_id}
{
}


void Rt::DownwardChannel<BlockSatDispatcher>::initDispatcher(const SpotByEntity& spot_by_entity,
                                                             const std::unordered_map<SpotComponentPair, tal_id_t> &routes,
                                                             const std::unordered_map<SpotComponentPair, RegenLevel> &regen_levels)
{
	this->spot_by_entity = spot_by_entity;
	this->routes = routes;
	this->regen_levels = regen_levels;
}


bool Rt::DownwardChannel<BlockSatDispatcher>::onEvent(const Event &event)
{
	LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
	    event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<BlockSatDispatcher>::onEvent(const MessageEvent &event)
{
	auto msg_type = event.getMessageType();
	LOG(log_receive, LEVEL_DEBUG, "Message received (type: %d)", msg_type);
	switch (to_enum<InternalMessageType>(msg_type))
	{
		// sent by SatCarrier
		case InternalMessageType::unknown:
		case InternalMessageType::sig:
		case InternalMessageType::encap_data:
		{
			return handleDvbFrame(event.getMessage<DvbFrame>());
		}
		// sent by Encap
		case InternalMessageType::decap_data:
		{
			return handleNetBurst(event.getMessage<NetBurst>());
		}
		case InternalMessageType::link_up:
			// ignore
			return true;
		default:
			LOG(log_receive, LEVEL_ERROR,
			    "Unexpected message type received: %d",
			    event.getMessageType());
			return false;
	}
}


bool Rt::DownwardChannel<BlockSatDispatcher>::handleDvbFrame(Ptr<DvbFrame> frame)
{
	const spot_id_t spot_id = frame->getSpot();
	const uint8_t carrier_id = frame->getCarrierId();
	const auto carrier_type = extractCarrierType(carrier_id);
	const bool is_data_carrier = isDataCarrier(carrier_type);
	const auto msg_type = is_data_carrier ? InternalMessageType::encap_data : InternalMessageType::sig;
	const log_level_t log_level = is_data_carrier ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level, "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	const auto [dest, src] = isGatewayCarrier(carrier_type)
	                       ? std::make_tuple(Component::terminal, Component::gateway)
	                       : std::make_tuple(Component::gateway, Component::terminal);

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
		if (isOutputCarrier(carrier_type))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Received a message from an output carried id (%d)", carrier_id);
			return false;
		}

		// add one to the input carrier id to get the corresponding output carrier id
		frame->setCarrierId(carrier_id + 1);
		bool is_transparent = regen_levels.at({spot_id, dest}) == RegenLevel::Transparent
		                   && (is_data_carrier || regen_levels.at({spot_id, src}) == RegenLevel::Transparent);
		return sendToLowerBlock({spot_id, dest, is_transparent}, std::move(frame), msg_type);
	}
	else
	{
		// send by ISL
		return sendToOppositeChannel(std::move(frame), msg_type);
	}
}


bool Rt::DownwardChannel<BlockSatDispatcher>::handleNetBurst(Ptr<NetBurst> in_burst)
{
	// Separate the packets by destination
	std::unordered_map<SpotComponentPair, Ptr<NetBurst>> bursts{};
	for (auto &&pkt: *in_burst)
	{
		const auto dest_id = pkt->getDstTalId();
		const auto src_id = pkt->getSrcTalId();
		const spot_id_t spot_id = spot_by_entity.getSpotForEntity(src_id);
		LOG(log_receive, LEVEL_INFO, "Received a NetBurst (%d->%d, spot_id %d)", src_id, dest_id, spot_id);

		Component dest = OpenSandModelConf::Get()->getEntityType(dest_id);
		SpotComponentPair spot_dest{spot_id, dest};
		auto burst_it = bursts.find(spot_dest);
		if (burst_it == bursts.end())
		{
			auto result = bursts.emplace(spot_dest, make_ptr<NetBurst>());
			burst_it = result.first;
		}
		burst_it->second->push_back(std::move(pkt));
	}

	// Send all bursts to their respective destination
	bool ok = true;
	for (auto &&[dest, burst]: bursts)
	{
		const auto dest_sat_id_it = routes.find(dest);
		if (dest_sat_id_it == routes.end())
		{
			LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
			    dest.dest == Component::gateway ? "GW" : "ST", dest.spot_id);
			ok = false;
			continue;
		}
		const tal_id_t dest_sat_id = dest_sat_id_it->second;
		if (dest_sat_id == entity_id || regen_levels.at(dest) == RegenLevel::IP)
		{
			ok &= sendToLowerBlock({dest.spot_id, dest.dest, false},
			                       std::move(burst),
			                       InternalMessageType::decap_data);
		}
		else
		{
			// send by ISL
			ok &= sendToOppositeChannel(std::move(burst), InternalMessageType::decap_data);
		}
	}
	return ok;
}


bool Rt::DownwardChannel<BlockSatDispatcher>::sendToLowerBlock(RegenerativeSpotComponent key,
                                                               Ptr<void> msg,
                                                               InternalMessageType msg_type)
{
	const auto log_level = msg_type == InternalMessageType::sig ? LEVEL_DEBUG : LEVEL_INFO;
	LOG(log_send, log_level, "Sending a message to the lower block, %s side", key.dest == Component::gateway ? "GW" : "ST");
	if (!enqueueMessage(key, std::move(msg), to_underlying(msg_type)))
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the lower block (%s, spot %d)",
		    key.dest == Component::gateway ? "GW" : "ST", key.spot_id);
		return false;
	}
	return true;
}


bool Rt::DownwardChannel<BlockSatDispatcher>::sendToOppositeChannel(Ptr<void> msg, InternalMessageType msg_type)
{
	const auto log_level = msg_type == InternalMessageType::sig ? LEVEL_DEBUG : LEVEL_INFO;
	LOG(log_send, log_level, "Sending a message to the opposite channel");
	if (!shareMessage(std::move(msg), to_underlying(msg_type)))
	{
		LOG(this->log_send, LEVEL_ERROR, "Failed to transmit message to the opposite channel");
		return false;
	}
	return true;
}

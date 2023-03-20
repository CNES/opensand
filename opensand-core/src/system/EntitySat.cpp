/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file EntitySat.cpp
 * @brief Regenerative satellite with ISL support
 * @author Yohan Simard <yohan.simard@viveris.fr>
 *
 * <pre>
 *
 *  ┌───────────────────────┐
 *  │         ISLs          │   Collection of LanAdaptation and/or Interconnect
 *  └─────┬─┬─┬────▲─▲─▲────┘
 *  ┌─────▼─▼─▼────┴─┴─┴────┐
 *  │  BlockSatDispatcher   │
 *  └──┬────▲───────┬────▲──┘
 *  ┌──▼────┴──┐ ┌──▼────┴──┐
 *  │  Encap   │ │  Encap   │
 *  └──┬────▲──┘ └──┬────▲──┘
 *  ┌──▼────┴──┐ ┌──▼────┴──┐
 *  │  DvbNcc  │ │  DvbTal  │   Two stacks are created per spot
 *  └──┬────▲──┘ └──┬────▲──┘
 *  ┌──▼────┴──┐ ┌──▼────┴──┐
 *  │SatCarrier│ │SatCarrier│
 *  └──────────┘ └──────────┘
 *
 *   terminals     gateways
 *
 * </pre>
 *
 */

#include <sstream>
#include <numeric>
#include <functional>

#include <opensand_rt/Rt.h>

#include "EntitySat.h"
#include "OpenSandModelConf.h"

#include "PacketSwitch.h"
#include "SpotUpward.h"
#include "SpotDownward.h"
#include "BlockDvbNcc.h"
#include "BlockDvbTal.h"
#include "BlockEncap.h"
#include "BlockInterconnect.h"
#include "BlockLanAdaptation.h"
#include "BlockPhysicalLayer.h"
#include "BlockSatCarrier.h"
#include "BlockSatDispatcher.h"
#include "BlockSatAsymetricHandler.h"
#include "DvbS2Std.h"


EntitySat::EntitySat(tal_id_t instance_id):
	Entity("sat" + std::to_string(instance_id), instance_id),
	instance_id{instance_id},
	isl_enabled{false}
{
}


bool EntitySat::createSpecificBlocks()
{
	try
	{
		auto Conf = OpenSandModelConf::Get();
		const auto &spot_topo = Conf->getSpotsTopology();

		SatDispatcherConfig sat_dispatch_cfg;
		sat_dispatch_cfg.entity_id = instance_id;
		sat_dispatch_cfg.isl_enabled = this->isl_enabled;
		auto& block_sat_dispatch = Rt::Rt::createBlock<BlockSatDispatcher>("Sat_Dispatch", sat_dispatch_cfg);

		uint32_t isl_delay = 0;
		if (isl_enabled)
		{
			auto isl_conf = Conf->getProfileData("isl");
			if (!OpenSandModelConf::extractParameterData(isl_conf, "delay", isl_delay))
			{
				return false;
			}
		}

		std::size_t index = 0;
		for (auto&& cfg : isl_config)
		{
			switch (cfg.type)
			{
				case IslType::Interconnect:
				{
					InterconnectConfig interco_cfg{
						.interconnect_addr = cfg.interco_addr,
						.delay = time_ms_t(isl_delay),
						.isl_index = index,
					};
					auto& block_interco = Rt::Rt::createBlock<BlockInterconnectUpward>("Interconnect.Isl", interco_cfg);
					Rt::Rt::connectBlocks(block_interco, block_sat_dispatch, {.connected_sat = cfg.linked_sat_id, .is_data_channel = false});
				}
					break;
				case IslType::LanAdaptation:
				{
					bool is_used_for_isl = instance_id != cfg.linked_sat_id;
					la_specific la_cfg{
						.tap_iface = cfg.tap_iface,
						.delay = time_ms_t(isl_delay),
						.connected_satellite = cfg.linked_sat_id,
						.is_used_for_isl = is_used_for_isl,
						.packet_switch = std::make_shared<SatellitePacketSwitch>(instance_id, is_used_for_isl, getIslEntities(spot_topo)),
					};
					auto& block_lan_adapt = Rt::Rt::createBlock<BlockLanAdaptation>(is_used_for_isl ? "Lan_Adaptation.Isl" : "Lan_Adaptation", la_cfg);
					Rt::Rt::connectBlocks(block_lan_adapt, block_sat_dispatch, {.connected_sat = cfg.linked_sat_id, .is_data_channel = true});
				}
					break;
				case IslType::None:
					break;
				default:
					DFLTLOG(LEVEL_ERROR,
					        "%s: error during block creation: ISL configuration #%ld has unknown type",
					        this->getName().c_str(), index);
					return false;
			}
			++index;
		}

		for (auto &&[spot_id, topo]: spot_topo)
		{
			if (topo.sat_id_gw == instance_id)
			{
				if (!createStack<BlockDvbTal>(block_sat_dispatch, spot_id, Component::gateway,
				                              topo.forward_regen_level, topo.return_regen_level))
				{
					DFLTLOG(LEVEL_CRITICAL,
					        "%s: error during block creation: could not "
					        "create DvbTal stack to communicate with the GW",
					        this->getName().c_str());
					return false;
				}
			}

			if (topo.sat_id_st == instance_id)
			{
				if (!createStack<BlockDvbNcc>(block_sat_dispatch, spot_id, Component::terminal,
				                              topo.forward_regen_level, topo.return_regen_level))
				{
					DFLTLOG(LEVEL_CRITICAL,
					        "%s: error during block creation: could not "
					        "create DvbTal stack to communicate with the GW",
					        this->getName().c_str());
					return false;
				}
			}
		}
	}
	catch (const std::bad_alloc &e)
	{
		DFLTLOG(LEVEL_CRITICAL, "%s: error during block creation: could not allocate memory: %s",
		        this->getName().c_str(), e.what());
		return false;
	}

	return true;
}

template <typename Dvb>
bool EntitySat::createStack(BlockSatDispatcher &block_sat_dispatch,
                            spot_id_t spot_id,
                            Component destination,
                            RegenLevel forward_regen_level,
                            RegenLevel return_regen_level)
{
	bool is_transparent{};
	std::ostringstream suffix_builder;
	switch (destination)
	{
		case Component::gateway:
			suffix_builder << "GW";
			is_transparent = return_regen_level == RegenLevel::Transparent;
			break;
		case Component::terminal:
			suffix_builder << "ST";
			is_transparent = forward_regen_level == RegenLevel::Transparent;
			break;
		default:
			DFLTLOG(LEVEL_ERROR,
			        "%s: error during block creation: Invalid destination for satellite stack",
			        this->getName().c_str());
			return false;
	}
	suffix_builder << spot_id;
	auto suffix = suffix_builder.str();

	sc_specific specific;
	specific.ip_addr = ip_address;
	specific.tal_id = instance_id;
	specific.spot_id = spot_id;
	specific.destination_host = destination;
	auto& block_sc = Rt::Rt::createBlock<BlockSatCarrier>("Sat_Carrier." + suffix, specific);

	if (forward_regen_level != RegenLevel::Transparent || return_regen_level != RegenLevel::Transparent)
	{
		dvb_specific dvb_spec;
		dvb_spec.disable_acm_loop = false;
		dvb_spec.mac_id = instance_id;
		dvb_spec.spot_id = spot_id;
		if (!OpenSandModelConf::Get()->getControlPlaneDisabled(dvb_spec.disable_control_plane))
		{
			DFLTLOG(LEVEL_ERROR,
			        "%s: error during block creation: Cannot retrieve disabled control plane parameter",
			        this->getName().c_str());
			return false;
		}

		EncapConfig encap_config;
		encap_config.entity_id = instance_id;
		encap_config.entity_type = destination == Component::gateway ? Component::terminal : Component::gateway;
		encap_config.filter_packets = false;
		encap_config.scpc_enabled = true;

		PhyLayerConfig phy_config;
		phy_config.mac_id = instance_id;
		phy_config.spot_id = spot_id;
		phy_config.entity_type = destination;

		AsymetricConfig asym_config;
		asym_config.phy_config = phy_config;
		asym_config.is_transparent = is_transparent;

		auto& block_encap = Rt::Rt::createBlock<BlockEncap>("Encap." + suffix, encap_config);
		auto& block_dvb = Rt::Rt::createBlock<Dvb>("Dvb." + suffix, dvb_spec);
		auto& block_asym = Rt::Rt::createBlock<BlockSatAsymetricHandler>("Asymetric_Handler." + suffix, asym_config);

		Rt::Rt::connectBlocks(block_sat_dispatch, block_encap, {spot_id, destination, false});
		Rt::Rt::connectBlocks(block_encap, block_dvb);
		Rt::Rt::connectBlocks(block_dvb, block_asym, false);
		Rt::Rt::connectBlocks(block_sat_dispatch, block_asym, true, {spot_id, destination, true});
		Rt::Rt::connectBlocks(block_asym, block_sc);
	}
	else
	{
		Rt::Rt::connectBlocks(block_sat_dispatch, block_sc, {spot_id, destination, true});
	}

	return true;
}

void defineProfileMetaModel()
{
	auto conf = OpenSandModelConf::Get();
	auto types = conf->getModelTypesDefinition();
	auto ctrl_plane = conf->getOrCreateComponent("control_plane", "Control plane", "Control plane configuration");
	auto disable_ctrl_plane = ctrl_plane->addParameter("disable_control_plane", "Disable control plane", types->getType("bool"));

	BlockDvbNcc::generateConfiguration(disable_ctrl_plane);
	BlockDvbTal::generateConfiguration(disable_ctrl_plane);
	BlockEncap::generateConfiguration();
	BlockLanAdaptation::generateConfiguration();
	GroundPhysicalChannel::generateConfiguration();

	auto isl = conf->getOrCreateComponent("isl", "ISL", "Inter-satellite links");
	auto isl_delay = isl->addParameter("delay", "Delay", types->getType("uint"), "Propagation delay for output ISL packets");
	isl_delay->setUnit("ms");
}

bool EntitySat::loadConfiguration(const std::string &profile_path)
{
	defineProfileMetaModel();
	auto Conf = OpenSandModelConf::Get();

	if (!Conf->getSatInfrastructure(this->ip_address, this->isl_config))
	{
		return false;
	}

	this->isl_enabled = std::transform_reduce(isl_config.cbegin(),
	                                          isl_config.cend(),
	                                          false,
	                                          std::logical_or<>(),
	                                          [](auto cfg){return cfg.type != IslType::None;});

	bool needs_profile = this->isl_enabled;
	if (!this->isl_enabled)
	{
		auto spots = Conf->getSpotsTopology();
		for (auto &&[spot_id, spot] : spots)
		{
			if ((spot.sat_id_gw == this->instance_id && spot.forward_regen_level != RegenLevel::Transparent) ||
			    (spot.sat_id_st == this->instance_id && spot.return_regen_level != RegenLevel::Transparent))
			{
				needs_profile = true;
				break;
			}
		}
	}

	if (needs_profile && !Conf->readProfile(profile_path))
	{
		return false;
	}

	return true;
}

bool EntitySat::createSpecificConfiguration(const std::string &filepath) const
{
	auto Conf = OpenSandModelConf::Get();
	Conf->createModels();
	defineProfileMetaModel();
	return Conf->writeProfileModel(filepath);
}

std::unordered_set<tal_id_t> EntitySat::getIslEntities(const std::unordered_map<spot_id_t, SpotTopology> &spot_topo) const
{
	std::unordered_set<tal_id_t> isl_entities;
	for (auto &&[spot_id, topo]: spot_topo)
	{
		if (topo.sat_id_st == instance_id && topo.sat_id_gw != instance_id)
		{
			isl_entities.insert(topo.gw_id);
		}
		if (topo.sat_id_st != instance_id && topo.sat_id_gw == instance_id)
		{
			std::copy(topo.st_ids.begin(), topo.st_ids.end(), std::inserter(isl_entities, isl_entities.end()));
		}
	}
	return isl_entities;
}

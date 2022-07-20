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
 * @file EntitySatRegen.cpp
 * @brief Entity regenerative satellite process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 * @author Yohan Simard <yohan.simard@viveris.fr>
 *
 * <pre>
 *
 *  ┌───────────────────────┐
 *  │ LanAdaptation/Interco │
 *  └─────┬──────────▲──────┘
 *  ┌─────▼──────────┴──────┐
 *  │      BlockMesh        │
 *  └──┬────▲───────┬────▲──┘
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

#include "EntitySatRegen.h"
#include "OpenSandModelConf.h"

#include "BlockDvbNcc.h"
#include "BlockDvbTal.h"
#include "BlockEncap.h"
#include "BlockInterconnect.h"
#include "BlockLanAdaptation.h"
#include "BlockPhysicalLayer.h"
#include "BlockSatCarrier.h"
#include "BlockTransp.h"

EntitySatRegen::EntitySatRegen(tal_id_t instance_id):
    Entity("sat_regen" + std::to_string(instance_id), instance_id),
    instance_id{instance_id}
{
}

bool EntitySatRegen::createSpecificBlocks()
{
	try
	{
		auto conf = OpenSandModelConf::Get();
		const auto &spot_topo = conf->getSpotsTopology();

		bool disable_ctrl_plane;
		if (!conf->getControlPlaneDisabled(disable_ctrl_plane)) return false;

		RegenLevel regen_level = conf->getRegenLevel();

		if (regen_level == RegenLevel::IP)
		{
			DFLTLOG(LEVEL_CRITICAL, "IP regeneration on satellite is not yet implemented");
			return false;
		}

		TranspConfig transp_config;
		transp_config.entity_id = instance_id;
		transp_config.isl_enabled = isl_config.type != IslType::None;
		auto block_transp = Rt::createBlock<BlockTransp>("Transp", transp_config);

		if (isl_config.type == IslType::Interconnect)
		{
			InterconnectConfig interco_cfg;
			interco_cfg.interconnect_addr = isl_config.interco_addr;
			interco_cfg.delay = isl_delay;
			auto block_interco = Rt::createBlock<BlockInterconnectUpward>("Interconnect", interco_cfg);
			Rt::connectBlocks(block_interco, block_transp);
		}
		else if (isl_config.type == IslType::LanAdaptation)
		{
			DFLTLOG(LEVEL_CRITICAL, "ISL by LanAdaptation are not yet implemented");
			return false;
		}

		for (auto &&spot: spot_topo)
		{
			const SpotTopology &topo = spot.second;
			const spot_id_t spot_id = spot.first;
			auto spot_id_str = std::to_string(spot_id);

			if (topo.sat_id_gw == instance_id)
			{
				createStack<BlockDvbTal>(block_transp, spot_id, Component::gateway, regen_level, disable_ctrl_plane);
			}

			if (topo.sat_id_st == instance_id)
			{
				createStack<BlockDvbNcc>(block_transp, spot_id, Component::terminal, regen_level, disable_ctrl_plane);
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
void EntitySatRegen::createStack(BlockTransp *block_transp,
                                 spot_id_t spot_id,
                                 Component destination,
                                 RegenLevel regen_level,
                                 bool disable_ctrl_plane)
{
	auto spot_id_str = std::to_string(spot_id);
	auto dest_str = destination == Component::gateway ? "GW" : "ST";
	auto suffix = dest_str + spot_id_str;

	sc_specific specific;
	specific.ip_addr = ip_address;
	specific.tal_id = instance_id;
	specific.spot_id = spot_id;
	specific.destination_host = destination;
	auto block_sc = Rt::createBlock<BlockSatCarrier>("SatCarrier" + suffix, specific);

	if (regen_level == RegenLevel::BBFrame)
	{
		dvb_specific dvb_spec;
		dvb_spec.disable_control_plane = disable_ctrl_plane;
		dvb_spec.disable_acm_loop = false;
		dvb_spec.mac_id = instance_id;
		dvb_spec.spot_id = spot_id;

		EncapConfig encap_config;
		encap_config.entity_id = instance_id;
		encap_config.entity_type = destination == Component::gateway ? Component::terminal : Component::gateway;
		encap_config.filter_packets = false;
		encap_config.scpc_enabled = true;

		PhyLayerConfig phy_config;
		phy_config.mac_id = instance_id;
		phy_config.spot_id = spot_id;
		phy_config.entity_type = destination;

		// Not a typo, the DVB Tal block communicates with the gateway
		auto block_encap = Rt::createBlock<BlockEncap>("Encap" + suffix, encap_config);
		auto block_dvb = Rt::createBlock<Dvb>("Dvb" + suffix, dvb_spec);
		auto block_phy = Rt::createBlock<BlockPhysicalLayer>("Phy" + suffix, phy_config);

		Rt::connectBlocks(block_transp, block_encap, {spot_id, destination});
		Rt::connectBlocks(block_encap, block_dvb);

		auto &dvb_upward = dynamic_cast<typename Dvb::Upward &>(*block_dvb->upward);
		auto &dvb_downward = dynamic_cast<typename Dvb::Downward &>(*block_dvb->downward);
		auto &sc_upward = dynamic_cast<typename BlockSatCarrier::Upward &>(*block_sc->upward);
		auto &sc_downward = dynamic_cast<typename BlockSatCarrier::Downward &>(*block_sc->downward);
		auto &phy_downward = dynamic_cast<typename BlockPhysicalLayer::Downward &>(*block_phy->downward);

		Rt::connectChannels(sc_upward, dvb_upward);
		Rt::connectChannels(dvb_downward, phy_downward);
		Rt::connectChannels(phy_downward, sc_downward);
	}
	else
	{
		Rt::connectBlocks(block_transp, block_sc, {spot_id, destination});
	}
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
	BlockPhysicalLayer::generateConfiguration();

	auto isl = conf->getOrCreateComponent("isl", "ISL", "Inter-satellite links");
	auto isl_delay = isl->addParameter("delay", "Delay", types->getType("int"), "Propagation delay for output ISL packets");
	isl_delay->setUnit("ms");
}

bool EntitySatRegen::loadConfiguration(const std::string &profile_path)
{
	defineProfileMetaModel();
	auto Conf = OpenSandModelConf::Get();
	if (!Conf->readProfile(profile_path))
	{
		return false;
	}
	auto isl_conf = Conf->getProfileData("isl");
	return OpenSandModelConf::extractParameterData(isl_conf, "delay", isl_delay) &&
	       Conf->getSatInfrastructure(this->ip_address) &&
	       Conf->getIslConfig(this->isl_config);
}

bool EntitySatRegen::createSpecificConfiguration(const std::string &filepath) const
{
	auto Conf = OpenSandModelConf::Get();
	Conf->createModels();
	defineProfileMetaModel();
	return Conf->writeProfileModel(filepath);
}

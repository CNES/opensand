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
 * SE uses the following stack of mgl blocs installed over 1 NIC:
 *
 * <pre>
 *  ┌───────────────────────┐
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
#include "BlockMesh.h"
#include "BlockSatCarrier.h"

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
		std::vector<tal_id_t> spot_ids{};
		conf->getGwIds(spot_ids);

		bool disable_ctrl_plane;
		if (!conf->getControlPlaneDisabled(disable_ctrl_plane)) return false;

		auto block_transp = Rt::createBlock<BlockMesh>("Mesh", instance_id);

		for (auto &&gw_id: spot_ids)
		{
			auto spot_id = static_cast<spot_id_t>(gw_id);
			auto spot_id_str = std::to_string(spot_id);

			dvb_specific dvb_spec;
			dvb_spec.disable_control_plane = disable_ctrl_plane;
			dvb_spec.mac_id = instance_id;
			auto block_dvb_ncc = Rt::createBlock<BlockDvbNcc>("DvbNcc" + spot_id_str, dvb_spec);
			auto block_dvb_tal = Rt::createBlock<BlockDvbTal>("DvbTal" + spot_id_str, dvb_spec);

			sc_specific specific;
			specific.ip_addr = ip_address;
			specific.tal_id = instance_id;
			specific.spot_id = spot_id;
			specific.destination_host = Component::gateway;
			auto block_sc_gw = Rt::createBlock<BlockSatCarrier>("SatCarrierGw" + spot_id_str, specific);

			specific.destination_host = Component::terminal;
			auto block_sc_st = Rt::createBlock<BlockSatCarrier>("SatCarrierSt" + spot_id_str, specific);

			// Not a typo:
			// The DVB NCC block communicates with the terminals
			Rt::connectBlocks(block_transp, block_dvb_ncc, {spot_id, Component::terminal});
			// The DVB Tal block communicates with the gateways
			Rt::connectBlocks(block_transp, block_dvb_tal, {spot_id, Component::gateway});
			Rt::connectBlocks(block_dvb_tal, block_sc_gw);
			Rt::connectBlocks(block_dvb_ncc, block_sc_st);
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

void defineProfileMetaModel()
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	auto ctrl_plane = Conf->getOrCreateComponent("control_plane", "Control plane", "Control plane configuration");
	auto disable_ctrl_plane = ctrl_plane->addParameter("disable_control_plane", "Disable control plane", types->getType("bool"));

	BlockDvbNcc::generateConfiguration(true);
	BlockDvbTal::generateConfiguration(disable_ctrl_plane);
}

bool EntitySatRegen::loadConfiguration(const std::string &profile_path)
{
	defineProfileMetaModel();
	auto Conf = OpenSandModelConf::Get();
	if (!Conf->readProfile(profile_path))
	{
		return false;
	}
	return Conf->getSatInfrastructure(this->ip_address);
}

bool EntitySatRegen::createSpecificConfiguration(const std::string &filepath) const
{
	auto Conf = OpenSandModelConf::Get();
	Conf->createModels();
	defineProfileMetaModel();
	return Conf->writeProfileModel(filepath);
}

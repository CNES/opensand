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
 * @file EntityGw.cpp
 * @brief Entity gateway process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 *
 * Gateway uses the following stack of blocks installed over 2 NICs
 * (nic1 on user network side and nic2 on satellite network side):
 *
 * <pre>
 *
 *                     eth nic 1
 *                         |
 *                   Lan Adaptation  ---------
 *                         |                  |
 *                   Encap/Desencap      IpMacQoSInteraction
 *                         |                  |
 *                      Dvb Ncc  -------------
 *                 [Dama Controller]
 *                         |
 *                  Sat Carrier Eth
 *                         |
 *                     eth nic 2
 *
 * </pre>
 *
 */


#include <opensand_rt/Rt.h>

#include "EntityGw.h"
#include "OpenSandModelConf.h"

#include "BlockLanAdaptation.h"
#include "BlockDvbNcc.h"
#include "BlockSatCarrier.h"
#include "BlockPhysicalLayer.h"
#include "SpotUpward.h"
#include "SpotDownward.h"
#include "PacketSwitch.h"
#include "Ethernet.h"


EntityGw::EntityGw(tal_id_t instance_id, bool check_mode):
	Entity("gw" + std::to_string(instance_id), instance_id, check_mode)
{
}


EntityGw::~EntityGw()
{
}


bool EntityGw::createSpecificBlocks()
{
	try {
		auto Conf = OpenSandModelConf::Get();
		const auto &spot_topo = Conf->getSpotsTopology();
		bool isRegen = false;
		for (auto &&[spot_id, topo]: spot_topo)
		{
			if (topo.gw_id == this->instance_id)
			{
				isRegen = topo.forward_regen_level != RegenLevel::Transparent
				       && topo.return_regen_level != RegenLevel::Transparent;
				break;
			}
		}

		struct la_specific laspecific;
		laspecific.tap_iface = this->tap_iface;
		laspecific.packet_switch = isRegen ? std::make_shared<RegenGatewayPacketSwitch>(this->instance_id) : std::make_shared<GatewayPacketSwitch>(this->instance_id);

		dvb_specific dvb_spec;
		dvb_spec.disable_control_plane = false;
		dvb_spec.mac_id = instance_id;
		dvb_spec.spot_id = instance_id;
		dvb_spec.is_ground_entity = true;
		dvb_spec.upper_encap = Ethernet::constructPlugin();

		struct sc_specific scspecific;
		scspecific.ip_addr = this->ip_address;
		scspecific.tal_id = this->instance_id;

		PhyLayerConfig phy_config;
		phy_config.mac_id = instance_id;
		phy_config.spot_id = instance_id;
		phy_config.entity_type = Component::gateway;

		auto& block_lan_adaptation = Rt::Rt::createBlock<BlockLanAdaptation>("Lan_Adaptation", laspecific);	
		auto& block_dvb = Rt::Rt::createBlock<BlockDvbNcc>("Dvb", dvb_spec);
		auto& block_phy_layer = Rt::Rt::createBlock<BlockPhysicalLayer>("Physical_Layer", phy_config);
		auto& block_sat_carrier = Rt::Rt::createBlock<BlockSatCarrier>("Sat_Carrier", scspecific);

		Rt::Rt::connectBlocks(block_lan_adaptation, block_dvb);
		Rt::Rt::connectBlocks(block_dvb, block_phy_layer);
		Rt::Rt::connectBlocks(block_phy_layer, block_sat_carrier);
	}
	catch (const std::bad_alloc &e)
	{
		DFLTLOG(LEVEL_CRITICAL, "%s: error during block creation: could not allocate memory: %s",
		        this->getName().c_str(), e.what());
		return false;
	}
	
	return true;
}

bool EntityGw::loadConfiguration(const std::string &profile_path)
{
	this->defineProfileMetaModel();
	auto Conf = OpenSandModelConf::Get();
	if(!Conf->readProfile(profile_path))
	{
		return false;
	}
	return Conf->getGroundInfrastructure(this->ip_address, this->tap_iface);
}

bool EntityGw::createSpecificConfiguration(const std::string &filepath) const
{
	auto Conf = OpenSandModelConf::Get();
	Conf->createModels();
	this->defineProfileMetaModel();
	return Conf->writeProfileModel(filepath);
}

void EntityGw::defineProfileMetaModel() const
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	auto ctrl_plane = Conf->getOrCreateComponent("control_plane", "Control plane", "Control plane configuration");
	auto disable_ctrl_plane = ctrl_plane->addParameter("disable_control_plane", "Disable control plane", types->getType("bool"));

	BlockLanAdaptation::generateConfiguration();
	BlockDvb::generateConfiguration();
	BlockDvbNcc::generateConfiguration(disable_ctrl_plane);
	BlockPhysicalLayer::generateConfiguration();
}

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
 * @file EntitySt.cpp
 * @brief Entity satellite terminal process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 *
 * ST uses the following stack of RT blocs installed over 2 NICs
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
 *                      Dvb Tal  -------------
 *                    [Dama Agent]
 *                         |
 *                  Sat Carrier Eth
 *                         |
 *                     eth nic 2
 *
 * </pre>
 *
 */


#include "EntitySt.h"
#include "OpenSandModelConf.h"

#include "BlockLanAdaptation.h"
#include "BlockEncap.h"
#include "BlockDvbTal.h"
#include "BlockSatCarrier.h"
#include "BlockPhysicalLayer.h"

#include "PacketSwitch.h"

EntitySt::EntitySt(tal_id_t instance_id): Entity("st" + std::to_string(instance_id), instance_id)
{
}

EntitySt::~EntitySt()
{
}

bool EntitySt::createSpecificBlocks()
{
	try
	{
		auto Conf = OpenSandModelConf::Get();
	
		la_specific laspecific;
		laspecific.tap_iface = this->tap_iface;
		tal_id_t gw_id;
	 	Conf->getGwWithTalId(this->instance_id, gw_id);
		laspecific.packet_switch = new TerminalPacketSwitch(this->instance_id, gw_id);

		EncapConfig encap_cfg;
		encap_cfg.entity_id = this->instance_id;
		encap_cfg.entity_type = Component::terminal;
		encap_cfg.filter_packets = true;
		encap_cfg.scpc_enabled = scpc_enabled;

		dvb_specific dvb_spec;
		dvb_spec.disable_control_plane = false;
		dvb_spec.mac_id = instance_id;
		dvb_spec.spot_id = gw_id;

		PhyLayerConfig phy_config;
		phy_config.mac_id = instance_id;
		phy_config.spot_id = gw_id;
		phy_config.entity_type = Component::terminal;

		sc_specific scspecific;
		scspecific.ip_addr = this->ip_address;
		scspecific.tal_id = this->instance_id;	

		bool disable_ctrl_plane;
		if (!Conf->getControlPlaneDisabled(disable_ctrl_plane)) return false;

		auto block_lan_adaptation = Rt::createBlock<BlockLanAdaptation>("Lan_Adaptation", laspecific);
		auto block_encap = Rt::createBlock<BlockEncap>("Encap", encap_cfg);
		auto block_dvb = Rt::createBlock<BlockDvbTal>("Dvb", dvb_spec);
		auto block_phy_layer = Rt::createBlock<BlockPhysicalLayer>("Physical_Layer", phy_config);
		auto block_sat_carrier = Rt::createBlock<BlockSatCarrier>("Sat_Carrier", scspecific);
	
		Rt::connectBlocks(block_lan_adaptation, block_encap);
		Rt::connectBlocks(block_encap, block_dvb);
		Rt::connectBlocks(block_dvb, block_phy_layer);
		Rt::connectBlocks(block_phy_layer, block_sat_carrier);
	}
	catch (const std::bad_alloc &e)
	{
		DFLTLOG(LEVEL_CRITICAL, "%s: error during block creation: could not allocate memory: %s",
		        this->getName().c_str(), e.what());
		return false;
	}
	return true;
}

bool EntitySt::loadConfiguration(const std::string &profile_path)
{
	this->defineProfileMetaModel();
	auto Conf = OpenSandModelConf::Get();
	if(!Conf->readProfile(profile_path))
	{
		return false;
	}
	return Conf->getGroundInfrastructure(this->ip_address, this->tap_iface) &&
	       Conf->getScpcEnabled(scpc_enabled);
}

bool EntitySt::createSpecificConfiguration(const std::string &filepath) const
{
	auto Conf = OpenSandModelConf::Get();
	Conf->createModels();
	this->defineProfileMetaModel();
	return Conf->writeProfileModel(filepath);
}

void EntitySt::defineProfileMetaModel() const
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	auto ctrl_plane = Conf->getOrCreateComponent("control_plane", "Control plane", "Control plane configuration");
	auto disable_ctrl_plane = ctrl_plane->addParameter("disable_control_plane", "Disable control plane", types->getType("bool"));

	BlockLanAdaptation::generateConfiguration();
	BlockEncap::generateConfiguration();
	BlockDvbTal::generateConfiguration(disable_ctrl_plane);
	BlockPhysicalLayer::generateConfiguration();
}

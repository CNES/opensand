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
 * @file EntityGwNetAcc.cpp
 * @brief Entity network/access gateway process
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
 *           Block Interconnect Downward
 *                         :
 *
 * </pre>
 *
 */


#include "EntityGwNetAcc.h"
#include "OpenSandModelConf.h"

#include "BlockInterconnect.h"
#include "BlockLanAdaptation.h"
#include "BlockDvbNcc.h"
#include "BlockEncap.h"

#include "PacketSwitch.h"

EntityGwNetAcc::EntityGwNetAcc(tal_id_t instance_id): Entity("gw_net_acc" + std::to_string(instance_id), instance_id)
{
}

EntityGwNetAcc::~EntityGwNetAcc()
{
}

bool EntityGwNetAcc::createSpecificBlocks()
{
	try
	{
		struct la_specific spec_la;
		spec_la.tap_iface = this->tap_iface;
		spec_la.packet_switch = new GatewayPacketSwitch(this->instance_id);

		auto block_lan_adaptation = Rt::createBlock<BlockLanAdaptation>("LanAdaptation", spec_la);
		auto block_encap = Rt::createBlock<BlockEncap>("Encap", this->instance_id);
		auto block_dvb = Rt::createBlock<BlockDvbNcc>("Dvb", dvb_specific{this->instance_id, false});
		auto block_interconnect = Rt::createBlock<BlockInterconnectDownward>("InterconnectDownward", this->interconnect_address);
		
		Rt::connectBlocks(block_lan_adaptation, block_encap);
		Rt::connectBlocks(block_encap, block_dvb);
		Rt::connectBlocks(block_dvb, block_interconnect);
	}
	catch (const std::bad_alloc &e)
	{
		DFLTLOG(LEVEL_CRITICAL, "%s: error during block creation: could not allocate memory: %s",
		        this->getName().c_str(), e.what());
		return false;
	}
	return true;
}

bool EntityGwNetAcc::loadConfiguration(const std::string &profile_path)
{
	this->defineProfileMetaModel();
	auto Conf = OpenSandModelConf::Get();
	if(!Conf->readProfile(profile_path))
	{
		return false;
	}

	return Conf->getGroundInfrastructure(this->interconnect_address, this->tap_iface);
}

bool EntityGwNetAcc::createSpecificConfiguration(const std::string &filepath) const
{
	auto Conf = OpenSandModelConf::Get();
	Conf->createModels();
	this->defineProfileMetaModel();
	return Conf->writeProfileModel(filepath);
}

void EntityGwNetAcc::defineProfileMetaModel() const
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	auto ctrl_plane = Conf->getOrCreateComponent("control_plane", "Control plane", "Control plane configuration");
	auto disable_ctrl_plane = ctrl_plane->addParameter("disable_control_plane", "Disable control plane", types->getType("bool"));

	BlockLanAdaptation::generateConfiguration();
	BlockEncap::generateConfiguration();
	BlockDvbNcc::generateConfiguration(disable_ctrl_plane);
	BlockInterconnectDownward::generateConfiguration();
}

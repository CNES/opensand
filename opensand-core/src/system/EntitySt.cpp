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
	auto Conf = OpenSandModelConf::Get();

	// instantiate all blocs
	struct la_specific laspecific;
	laspecific.tap_iface = this->tap_iface;
	tal_id_t gw_id;
 	Conf->getGwWithTalId(this->instance_id, gw_id);
	laspecific.packet_switch = new TerminalPacketSwitch(this->instance_id, gw_id);
	auto block_lan_adaptation = Rt::createBlock<BlockLanAdaptation>("LanAdaptation", laspecific);
	if(!block_lan_adaptation)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the LanAdaptation block",
		        this->getName().c_str());
		return false;
	}

	auto block_encap = Rt::createBlock<BlockEncap>("Encap", this->instance_id);
	if(!block_encap)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the Encap block",
            this->getName().c_str());
		return false;
	}

	auto block_dvb = Rt::createBlock<BlockDvbTal>("Dvb", dvb_specific{this->instance_id, false});
	if(!block_dvb)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the DvbTal block",
            this->getName().c_str());
		return false;
	}

	auto block_phy_layer = Rt::createBlock<BlockPhysicalLayer>("PhysicalLayer", this->instance_id);
	if(!block_phy_layer)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the PhysicalLayer block",
		        this->getName().c_str());
		return false;
	}

	struct sc_specific scspecific;
	scspecific.ip_addr = this->ip_address;
	scspecific.tal_id = this->instance_id;
	auto block_sat_carrier = Rt::createBlock<BlockSatCarrier>("SatCarrier", scspecific);
	if(!block_sat_carrier)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the SatCarrier block",
            this->getName().c_str());
		return false;
	}

	Rt::connectBlocks(block_lan_adaptation, block_encap);
	Rt::connectBlocks(block_encap, block_dvb);
	Rt::connectBlocks(block_dvb, block_phy_layer);
	Rt::connectBlocks(block_phy_layer, block_sat_carrier);

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
	return Conf->getGroundInfrastructure(this->ip_address, this->tap_iface);
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
	BlockLanAdaptation::generateConfiguration();
	BlockEncap::generateConfiguration();
	BlockDvbTal::generateConfiguration();
	BlockPhysicalLayer::generateConfiguration();
}

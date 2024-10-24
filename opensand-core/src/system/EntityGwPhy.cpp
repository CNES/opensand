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
 * @file EntityGwPhy.cpp
 * @brief Entity physical gateway process
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
 *                         :
 *                         :
 *              Block Interconnect Upward
 *                         |
 *                  Sat Carrier Eth
 *                         |
 *                     eth nic 2
 *
 * </pre>
 *
 */


#include <opensand_rt/Rt.h>

#include "EntityGwPhy.h"
#include "OpenSandModelConf.h"

#include "BlockSatCarrier.h"
#include "BlockPhysicalLayer.h"
#include "BlockInterconnect.h"


EntityGwPhy::EntityGwPhy(tal_id_t instance_id, bool check_mode):
	Entity("gw_phy" + std::to_string(instance_id), instance_id, check_mode)
{
}

EntityGwPhy::~EntityGwPhy()
{
}

bool EntityGwPhy::createSpecificBlocks()
{
	try
	{
		sc_specific specific;
		specific.ip_addr = this->ip_address;
		specific.tal_id = this->instance_id;

		PhyLayerConfig phy_config;
		phy_config.mac_id = instance_id;
		phy_config.spot_id = instance_id;
		phy_config.entity_type = Component::gateway;

		InterconnectConfig interco_cfg;
		interco_cfg.interconnect_addr = this->interconnect_address;
		interco_cfg.delay = time_ms_t::zero();

		auto& block_interconnect = Rt::Rt::createBlock<BlockInterconnectUpward>("Interconnect.Upward", interco_cfg);
		auto& block_phy_layer = Rt::Rt::createBlock<BlockPhysicalLayer>("Physical_Layer", phy_config);
		auto& block_sat_carrier = Rt::Rt::createBlock<BlockSatCarrier>("Sat_Carrier", specific);

		Rt::Rt::connectBlocks(block_interconnect, block_phy_layer);
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

bool EntityGwPhy::loadConfiguration(const std::string &profile_path)
{
	this->defineProfileMetaModel();
	auto Conf = OpenSandModelConf::Get();
	if(!Conf->readProfile(profile_path))
	{
		return false;
	}

	return Conf->getGroundInfrastructure(this->ip_address, this->interconnect_address);
}

bool EntityGwPhy::createSpecificConfiguration(const std::string &filepath) const
{
	auto Conf = OpenSandModelConf::Get();
	Conf->createModels();
	this->defineProfileMetaModel();
	return Conf->writeProfileModel(filepath);
}

void EntityGwPhy::defineProfileMetaModel() const
{
	BlockPhysicalLayer::generateConfiguration();
}

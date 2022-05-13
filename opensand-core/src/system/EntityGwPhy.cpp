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


#include "EntityGwPhy.h"
#include "OpenSandModelConf.h"

#include "BlockSatCarrier.h"
#include "BlockPhysicalLayer.h"
#include "BlockInterconnect.h"


EntityGwPhy::EntityGwPhy(tal_id_t instance_id): Entity("gw_phy" + std::to_string(instance_id), instance_id)
{
}

EntityGwPhy::~EntityGwPhy()
{
}

bool EntityGwPhy::createSpecificBlocks()
{
	struct sc_specific specific;

	// instantiate all blocs
	auto block_interconnect = Rt::createBlock<BlockInterconnectUpward>("InterconnectUpward",
	                                                                   this->interconnect_address);
	if(!block_interconnect)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the InterconnectUpward block",
            this->getName().c_str());
		return false;
	}

	auto block_phy_layer = Rt::createBlock<BlockPhysicalLayer>("PhysicalLayer", this->instance_id);
	if(!block_phy_layer)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the PhysicalLayer block",
		        this->getName().c_str());
		return false;;
	}
	specific.ip_addr = this->ip_address;
	specific.tal_id = this->instance_id;
	auto block_sat_carrier = Rt::createBlock<BlockSatCarrier>("SatCarrier", specific);
	if(!block_sat_carrier)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the SatCarrier block",
            this->getName().c_str());
		return false;
	}

	Rt::connectBlocks(block_interconnect, block_phy_layer);
	Rt::connectBlocks(block_phy_layer, block_sat_carrier);

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
	BlockInterconnectUpward::generateConfiguration();
	BlockPhysicalLayer::generateConfiguration();
}

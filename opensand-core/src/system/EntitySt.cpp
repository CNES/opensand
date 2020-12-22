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


EntitySt::EntitySt(tal_id_t instance_id): Entity("st" + std::to_string(instance_id), instance_id)
{
}

EntitySt::~EntitySt()
{
}

bool EntitySt::createSpecificBlocks()
{
	struct la_specific laspecific;
	struct sc_specific scspecific;

	Block *block_lan_adaptation;
	Block *block_encap;
	Block *block_dvb;
	Block *block_phy_layer;
	Block *block_sat_carrier;

	// instantiate all blocs
	laspecific.tap_iface = this->tap_iface;
	block_lan_adaptation = Rt::createBlock<BlockLanAdaptation,
			 BlockLanAdaptation::Upward,
			 BlockLanAdaptation::Downward,
			 struct la_specific>("LanAdaptation", NULL, laspecific);
	if(!block_lan_adaptation)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the LanAdaptation block",
		        this->getName().c_str());
		return false;
	}

	block_encap = Rt::createBlock<BlockEncap,
		BlockEncap::Upward,
		BlockEncap::Downward,
		tal_id_t>("Encap", block_lan_adaptation, this->instance_id);
	if(!block_encap)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the Encap block",
            this->getName().c_str());
		return false;
	}

	block_dvb = Rt::createBlock<BlockDvbTal,
	      BlockDvbTal::Upward,
	      BlockDvbTal::Downward,
	      tal_id_t>("Dvb", block_encap, this->instance_id);
	if(!block_dvb)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the DvbTal block",
            this->getName().c_str());
		return false;
	}

	block_phy_layer = Rt::createBlock<BlockPhysicalLayer,
		    BlockPhysicalLayer::Upward,
		    BlockPhysicalLayer::Downward,
		    tal_id_t>("PhysicalLayer", block_dvb, this->instance_id);
	if(!block_phy_layer)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the PhysicalLayer block",
		        this->getName().c_str());
		return false;
	}

	scspecific.ip_addr = this->ip_address;
	scspecific.tal_id = this->instance_id;
	block_sat_carrier = Rt::createBlock<BlockSatCarrier,
		      BlockSatCarrier::Upward,
		      BlockSatCarrier::Downward,
		      struct sc_specific>("SatCarrier",
					  block_phy_layer,
					  scspecific);
	if(!block_sat_carrier)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the SatCarrier block",
            this->getName().c_str());
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
	// TODO populate attributes
	return true;
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
	auto profile = OpenSandModelConf::Get()->getProfileModel();
	// TODO
}

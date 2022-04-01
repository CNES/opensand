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
	struct la_specific spec_la;

	Block *block_lan_adaptation;
	Block *block_encap;
	Block *block_dvb;
	Block *block_interconnect;

	// instantiate all blocs
	spec_la.tap_iface = this->tap_iface;
	spec_la.packet_switch = new GatewayPacketSwitch(this->instance_id);
	block_lan_adaptation = Rt::createBlock<BlockLanAdaptation,
			 BlockLanAdaptation::Upward,
			 BlockLanAdaptation::Downward,
			 struct la_specific>("LanAdaptation", NULL, spec_la);
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

	block_dvb = Rt::createBlock<BlockDvbNcc,
	      BlockDvbNcc::Upward,
	      BlockDvbNcc::Downward,
	      tal_id_t>("Dvb", block_encap, this->instance_id);
	if(!block_dvb)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the DvbNcc block",
            this->getName().c_str());
		return false;
	}

	block_interconnect = Rt::createBlock<BlockInterconnectDownward,
		       BlockInterconnectDownward::Upward,
		       BlockInterconnectDownward::Downward,
		       const string &>
		       ("InterconnectDownward", block_dvb, this->interconnect_address);
	if(!block_interconnect)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the InterconnectDownward block",
            this->getName().c_str());
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
	BlockLanAdaptation::generateConfiguration();
	BlockEncap::generateConfiguration();
	BlockDvbNcc::generateConfiguration();
	BlockInterconnectDownward::generateConfiguration();
}

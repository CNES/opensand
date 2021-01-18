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
 * @file EntitySat.cpp
 * @brief Entity satellite process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 *
 * SE uses the following stack of mgl blocs installed over 1 NIC:
 *
 * <pre>
 *
 *                +---+
 *                |   |
 *            Encap/Desencap
 *                |   |
 *               Dvb Sat
 *                |   |
 *           Sat Carrier Eth
 *                |   |
 *               eth nic
 *
 * </pre>
 *
 */


#include "EntitySat.h"
#include "OpenSandModelConf.h"

#include "BlockDvbSatTransp.h"
#include "BlockSatCarrier.h"


EntitySat::EntitySat(): Entity("sat", 0)
{
}

EntitySat::~EntitySat()
{
}

bool EntitySat::createSpecificBlocks()
{
	struct sc_specific specific;

	Block *block_dvb;
	Block *block_sat_carrier;

	// instantiate all blocs
	block_dvb = Rt::createBlock<BlockDvbSatTransp,
	BlockDvbSatTransp::UpwardTransp,
	BlockDvbSatTransp::DownwardTransp>("Dvb", NULL);
	if(!block_dvb)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the DvbSat block",
            this->getName().c_str());
		return false;
	}

	specific.ip_addr = this->ip_address;
	block_sat_carrier = Rt::createBlock<BlockSatCarrier,
	      BlockSatCarrier::Upward,
	      BlockSatCarrier::Downward,
	      struct sc_specific>("SatCarrier",
		  block_dvb,
		  specific);
	if(!block_sat_carrier)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the SatCarrier block",
            this->getName().c_str());
		return false;
	}

	return true;
}

bool EntitySat::loadConfiguration(const std::string &profile_path)
{
	auto Conf = OpenSandModelConf::Get();
	return Conf->getSatInfrastructure(this->ip_address);
}

bool EntitySat::createSpecificConfiguration(const std::string &filepath) const
{
	return true;
}

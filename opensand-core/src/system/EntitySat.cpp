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

#include "BlockSatCarrier.h"
#include "BlockTransp.h"
#include "BlockInterconnect.h"


EntitySat::EntitySat(tal_id_t instance_id):
    Entity("sat" + std::to_string(instance_id), instance_id),
    instance_id{instance_id}
{
}


EntitySat::~EntitySat()
{
}


bool EntitySat::createSpecificBlocks()
{
	try
	{
		auto conf = OpenSandModelConf::Get();
		auto &spot_topo = conf->getSpotsTopology();

		TranspConfig transp_cfg;
		transp_cfg.entity_id = instance_id;
		transp_cfg.isl_enabled = false;
		auto block_transp = Rt::createBlock<BlockTransp>("Transp", transp_cfg);

		for (auto &&spot: spot_topo)
		{
			const SpotTopology &topo = spot.second;
			const spot_id_t spot_id = spot.first;
			auto spot_id_str = std::to_string(spot_id);

			if (topo.sat_id_gw == instance_id)
			{
				sc_specific specific;
				specific.ip_addr = ip_address;
				specific.tal_id = instance_id;
				specific.spot_id = spot_id;
				specific.destination_host = Component::gateway;
				auto block_sc_gw = Rt::createBlock<BlockSatCarrier>("SatCarrierGw" + spot_id_str, specific);

				Rt::connectBlocks(block_transp, block_sc_gw, {spot_id, Component::gateway});
			}

			if (topo.sat_id_st == instance_id)
			{
				sc_specific specific;
				specific.ip_addr = ip_address;
				specific.tal_id = instance_id;
				specific.spot_id = spot_id;
				specific.destination_host = Component::terminal;
				auto block_sc_st = Rt::createBlock<BlockSatCarrier>("SatCarrierSt" + spot_id_str, specific);

				Rt::connectBlocks(block_transp, block_sc_st, {spot_id, Component::terminal});
			}
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

bool EntitySat::loadConfiguration(const std::string &)
{
	auto Conf = OpenSandModelConf::Get();
	return Conf->getSatInfrastructure(this->ip_address);
}

bool EntitySat::createSpecificConfiguration(const std::string &) const
{
	return true;
}

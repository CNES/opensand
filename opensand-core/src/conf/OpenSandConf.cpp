/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
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
 * @file OpenSandConf.cpp
 * @brief Global interface for configuration file reading
 * @author Viveris Technologies
 */


#include "OpenSandConf.h"
#include "OpenSandConfFile.h"


OpenSandConfFile OpenSandConf::global_config;
map <unsigned int, uint16_t> OpenSandConf::carrier_map;
map <uint16_t, uint16_t> OpenSandConf::gw_table;

OpenSandConf::OpenSandConf()
{
}

OpenSandConf::~OpenSandConf()
{
	carrier_map.clear();
	gw_table.clear();
}

bool OpenSandConf::getGwWithTalId(uint16_t tal_id,
                                  uint16_t &gw_id)
{
	return global_config.getGwWithTalId(OpenSandConf::gw_table,
	                                    tal_id,
	                                    gw_id);
}

bool OpenSandConf::getGwWithCarrierId(unsigned int car_id,
                                        uint16_t &gw)
{
	return global_config.getGwWithCarrierId(OpenSandConf::carrier_map,
	                                        car_id,
	                                        gw);
}

void OpenSandConf::loadConfig(void)
{
	global_config.loadCarrierMap(OpenSandConf::carrier_map);
	global_config.loadGwTable(OpenSandConf::gw_table);
}

bool OpenSandConf::isGw(uint16_t gw_id)
{
	return global_config.isGw(OpenSandConf::gw_table, gw_id);
}

bool OpenSandConf::getSpot(string section,
                           uint16_t gw_id,
                           ConfigurationList &current_gw)
{
	return global_config.getSpot(section, gw_id, current_gw);
}

bool OpenSandConf::getScpcEncapStack(string return_link_std,
                                     vector<string> &encap_stack)
{
	return global_config.getScpcEncapStack(return_link_std, encap_stack);
}

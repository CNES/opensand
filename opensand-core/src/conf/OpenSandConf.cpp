/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
map <unsigned int, std::pair<uint8_t, uint16_t> > OpenSandConf::carrier_map;
map <uint16_t, uint8_t> OpenSandConf::spot_table;
map <uint16_t, uint16_t> OpenSandConf::gw_table;

OpenSandConf::OpenSandConf()
{
}

OpenSandConf::~OpenSandConf()
{
	carrier_map.clear();
	spot_table.clear();
	gw_table.clear();
}

bool OpenSandConf::getSpotWithTalId(uint16_t tal_id,
                                    uint8_t &spot)
{
	return global_config.getSpotWithTalId(OpenSandConf::spot_table,
	                                      tal_id,
	                                      spot);
}

bool OpenSandConf::getSpotWithCarrierId(unsigned int car_id,
                                        uint8_t &spot, 
                                        uint16_t &gw)
{
	return global_config.getSpotWithCarrierId(OpenSandConf::carrier_map,
	                                          car_id,
	                                          spot, gw);
}

void OpenSandConf::loadConfig(void)
{
	global_config.loadCarrierMap(OpenSandConf::carrier_map);
	global_config.loadSpotTable(OpenSandConf::spot_table);
	global_config.loadGwTable(OpenSandConf::gw_table);
}

bool OpenSandConf::isGw(uint16_t gw_id)
{
	return global_config.isGw(OpenSandConf::gw_table, gw_id);
}

bool OpenSandConf::getSpot(string section,
                           uint8_t spot_id,
                           uint16_t gw_id,
                           ConfigurationList &current_gw)
{
	return global_config.getSpot(section, spot_id, gw_id, current_gw);
}

bool OpenSandConf::getScpcEncapStack(string return_link_std,
                                     vector<string> &encap_stack)
{
	return global_config.getScpcEncapStack(return_link_std, encap_stack);
}

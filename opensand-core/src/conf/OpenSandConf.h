/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file OpenSandConf.h
 * @brief GLobal interface for configuration file reading
 * @author Bénédicte MOTTO / <bmotto@toulouse.viveris.com>
 * @author Joaquin Muguerza / <jmuguerza@toulouse.viveris.com>
 */

#ifndef OPENSAND_CONF_H
#define OPENSAND_CONF_H

#define CONF_TOPOLOGY "topology.conf"
#define CONF_GLOBAL_FILE "core_global.conf"
#define CONF_DEFAULT_FILE "core.conf"

#include "OpenSandCore.h"
#include "OpenSandConfFile.h"

#include <opensand_conf/conf.h>

#include <string>
#include <vector>

using namespace std;

class OpenSandConf
{
 public:

	OpenSandConf(void);
	~OpenSandConf(void);

	/**
	 * The spot association with each carrier
	 */
	static map<unsigned int, std::pair<uint8_t, uint16_t> > carrier_map;

	/**
	 * The spot association with each terminal
	 */
	static map<tal_id_t, spot_id_t> spot_table;

	/**
	 * The spot association with each terminal
	 */ 
	static map<tal_id_t, tal_id_t> gw_table;

	/**
	 * Load some configuration files content into memory
	 * @param conf_files the configuration files path
	 * @return  true on success, false otherwise
	 */
	static void loadConfig(void);

	/**
	 * Get gateway id value in terminal map
	 *
	 * @param tal_id   the terminal id
	 * @param gw_id    the gateway id
	 * @return true on success, false otherwise
	 */
	static bool getGwWithTalId(uint16_t tal_id,
	                           uint16_t &gw_id);

	/**
	 * Get spot value in terminal map
	 *
	 * @param tal_it   the terminal id
	 * @param spot     the found spot
	 * @return true on success, false otherwise
	 */
	static bool getSpotWithTalId(uint16_t tal_id,
	                             uint8_t &spot);

	/**
	 * Get spot value in carrier map
	 *
	 * @param car_it   the carrier id
	 * @param car_iter the found iterator 
	 * @return true on success, false otherwise
	 */
	static bool getSpotWithCarrierId(unsigned int car_id,
	                                 uint8_t &spot,
	                                 uint16_t &gw);

	/**
	 * Check if the id is a gateway
	 * @param gw_id     the currend id to check is a gw
	 * @return true if this id is a gw, false otherwize
	 */
	static bool isGw(uint16_t gw_id);

	/**
	 * return current spot with gw and spot ids
	 * @param section    the section name
	 * @param spot_id    the spot id
	 * @param gw_id      the gw id
	 * @param current_gw the found spot/gw
	 * @return true on success, false otherwize
	 */
	static bool getSpot(string section,
	                    uint8_t spot_id,
	                    uint16_t gw_id,
	                    ConfigurationList &current_gw);

	/**
	 * Get the SCPC encapsulation stack in function for a return link standard
	 * 
	 * @param return_link_std  the return link standard
	 * @param encap_stack      the found encapsulation stack for SCPC
	 * @return true on success, false otherwise
	 */
	static bool getScpcEncapStack(string return_link_std,
	                              vector<string> &encap_stack);

 private:

	static OpenSandConfFile global_config;

};



#endif

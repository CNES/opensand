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
 * @file OpenSandConfFile.h
 * @brief Reading parameters from a configuration file
 * @author Bénédicte MOTTO / <bmotto@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 */

#ifndef OPENSAND_CONF_FILE_H
#define OPENSAND_CONF_FILE_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <stdint.h>

#include <opensand_conf/conf.h>

#include <opensand_output/OutputLog.h>

using namespace std;


/*
 * @class OpenSandConfFile
 * @brief Reading parameters from a special configuration file
 *
 */
class OpenSandConfFile
{
 public:
	// Ctor & dtor
	OpenSandConfFile(void);
	virtual ~OpenSandConfFile(void);

	/**
	 * Create a Map which associate carrier id to spot id
	 * @param carrier_map the map between carrier id and spot id
	 */
	void loadCarrierMap(map<unsigned int, std::pair<uint8_t, uint16_t> > &carrier_map);

	/**
	 * Create a Map which associate terminal id to spot id
	 * @param terminal_map the map between terminal id and spot id
	 */
	void loadSpotTable(map<uint16_t, uint8_t> &spot_table);

	/**
	 * Create a Map which associate terminal id to gw id
	 * @param terminal_map the map between terminal id and spot id
	 */
	void loadGwTable(map<uint16_t, uint16_t> &gw_table);

	/**
	 * Get gateway id value in terminal map
	 *
	 * @param tal_id   the terminal id
	 * @param gw_id    the gateway id
	 * @return true on success, false otherwise
	 */
	bool getGwWithTalId(map<uint16_t, uint16_t> terminal_map,
			    uint16_t tal_id,
	                    uint16_t &gw_id);

	/**
	 * Get spot value in terminal map
	 *
	 * @param tal_it   the terminal id
	 * @param spot     the found spot
	 * @return true on success, false otherwise
	 */
	bool getSpotWithTalId(map<uint16_t, uint8_t> terminal_map,
	                      uint16_t tal_id,
	                      uint8_t &spot);

	/**
	 * Get spot value in carrier map
	 *
	 * @param car_it   the carrier id
	 * @param spot     the found spot
	 * @param gw       the found gw
	 * @return true on success, false otherwise
	 */
	bool getSpotWithCarrierId(map<unsigned int,
	                          std::pair<uint8_t, uint16_t> > carrier_map,
	                          unsigned int car_id,
	                          uint8_t &spot,
	                          uint16_t &gw);

	/**
	 * Check if the id is a gateway
	 * @param gw_table  the table of tal by gw
	 * @param gw_id     the currend id to check is a gw
	 * @return true if this id is a gw, false otherwize
	 */
	bool isGw(map<uint16_t, uint16_t> &gw_table,
	          uint16_t gw_id);


	/**
	 * return current spot with gw and spot ids
	 * @param section    the section name
	 * @param spot_id    the spot id
	 * @param gw_id      the gw id
	 * @param current_gw the found spot/gw
	 * @return true on success, false otherwize
	 */
	bool getSpot(string section,
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
	bool getScpcEncapStack(string return_link_std,
	                       vector<string> &encap_stack);

 private:

	/// Output Log
	OutputLog *log_conf;

	/// SCPC encapsulation stack for each return link standard
	map< string, vector<string> > scpc_encap_stacks;
};


#endif

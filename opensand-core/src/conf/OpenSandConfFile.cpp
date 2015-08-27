/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file OpenSandConfFile.cpp
 * @brief Reading parameters from a configuration file
 * @author Viveris Technologies
 */


#include "OpenSandConfFile.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>



OpenSandConfFile::OpenSandConfFile()
{
}

OpenSandConfFile::~OpenSandConfFile()
{
}

void OpenSandConfFile::loadCarrierMap(map<unsigned int, std::pair<uint8_t, uint16_t> > &carrier_map)
{
	ConfigurationList section_sat_car;
	ConfigurationList spots;
	ConfigurationList::iterator iter_spots;

	section_sat_car = Conf::section_map[SATCAR_SECTION];

	if(!Conf::getListNode(section_sat_car, SPOT_LIST, spots))
	{
		return;
	}

	for(iter_spots = spots.begin() ; iter_spots != spots.end() ; ++iter_spots)
	{
		ConfigurationList current_spot;
		ConfigurationList carrier_list;
		ConfigurationList::iterator iter_carrier;
		xmlpp::Node* spot_node = *iter_spots;
		// TODO avoid using xmlpp::Node
		current_spot.push_front(spot_node);
		uint8_t spot_id = 0;
		uint16_t gw_id = 0;

		// get current spot id
		if(!Conf::getAttributeValue(iter_spots, ID, spot_id))
		{
			return;
		}
		
		// get current gw id
		if(!Conf::getAttributeValue(iter_spots, GW, gw_id))
		{
			return;
		}
	 
	 	// get spot channel
		if(!Conf::getListItems(*iter_spots, CARRIER_LIST, carrier_list))
		{
			return;
		}

		// associate channel to spot
		for(iter_carrier = carrier_list.begin() ; iter_carrier != carrier_list.end() ; 
		    ++iter_carrier)
		{
			int carrier_id = 0;

			//get carrier ID
			if(!Conf::getAttributeValue(iter_carrier, CARRIER_ID, carrier_id))
			{
				return;
			}

			carrier_map[carrier_id] = make_pair(spot_id, gw_id);
		}
	}

}

void OpenSandConfFile::loadSpotTable(map<uint16_t, uint8_t> &spot_table)
{
	ConfigurationList spot_table_section;
	ConfigurationList spots;
	ConfigurationList::iterator iter_spots;

	spot_table_section = Conf::section_map[SPOT_TABLE_SECTION];

	if(!Conf::getListNode(spot_table_section, SPOT_LIST, spots))
	{
		return;
	}

	for(iter_spots = spots.begin() ; iter_spots != spots.end() ; ++iter_spots)
	{
		ConfigurationList current_spot;
		ConfigurationList terminal_list;
		ConfigurationList::iterator iter_terminal;
		xmlpp::Node* spot_node = *iter_spots;
		// TODO surcharger pour donner élément simple
		current_spot.push_front(spot_node);
		uint8_t spot_id = 0;

		// get current spot id
		if(!Conf::getAttributeValue(iter_spots, ID, spot_id))
		{
			return;
		}

		// get spot channel
		if(!Conf::getListItems(current_spot, TERMINAL_LIST, terminal_list))
		{
			return;
		}

		// associate channel to spot
		for(iter_terminal = terminal_list.begin() ; iter_terminal != terminal_list.end() ; 
		    ++iter_terminal)
		{
			uint16_t tal_id = 0;

			//get carrier ID
			if(!Conf::getAttributeValue(iter_terminal, ID, tal_id))
			{
				return;
			}
			spot_table[tal_id] = spot_id;
		}
	}
}

void OpenSandConfFile::loadGwTable(map<uint16_t, uint16_t> &gw_table)
{
	ConfigurationList gw_table_section;
	ConfigurationList gws;
	ConfigurationList::iterator iter_gws;

	gw_table_section = Conf::section_map[GW_TABLE_SECTION];

	if(!Conf::getListNode(gw_table_section, GW_LIST, gws))
	{
		return;
	}

	for(iter_gws = gws.begin() ; iter_gws != gws.end() ; ++iter_gws)
	{
		ConfigurationList current_gw;
		ConfigurationList terminal_list;
		ConfigurationList::iterator iter_terminal;
		xmlpp::Node* gw_node = *iter_gws;
		// TODO avoid using xmlpp::Node
		current_gw.push_front(gw_node);
		uint8_t gw_id = 0;

		// get current spot id
		if(!Conf::getAttributeValue(iter_gws, ID, gw_id))
		{
			return;
		}

		// get spot channel
		if(!Conf::getListItems(current_gw, TERMINAL_LIST, terminal_list))
		{
			return;
		}

		// associate channel to spot
		for(iter_terminal = terminal_list.begin() ; iter_terminal != terminal_list.end() ; 
		    ++iter_terminal)
		{
			uint16_t tal_id = 0;

			//get carrier ID
			if(!Conf::getAttributeValue(iter_terminal, ID, tal_id))
			{
				return;
			}
			gw_table[tal_id] = gw_id;
		}
	}
}


bool OpenSandConfFile::getSpotWithTalId(map<uint16_t, uint8_t> terminal_map, 
                                        uint16_t tal_id,
                                        uint8_t &spot)
{
    map<uint16_t, uint8_t>::iterator tal_iter;
	tal_iter = terminal_map.find(tal_id);
	if(tal_iter == terminal_map.end())
	{
		return false;
	}
	spot = (*tal_iter).second;
	return true;
}

bool OpenSandConfFile::getSpotWithCarrierId(map<unsigned int, std::pair<uint8_t, uint16_t> > carrier_map, 
                                            unsigned int car_id,
                                            uint8_t &spot, 
                                            uint16_t &gw)
{
	map<unsigned int, std::pair<uint8_t, uint16_t> >::iterator car_iter;
	car_iter = carrier_map.find(car_id);
	if(car_iter == carrier_map.end())
	{
		return false;
	}
		
	spot = carrier_map[car_id].first;
	gw = carrier_map[car_id].second;
	return true;
}

bool OpenSandConfFile::isGw(map<uint16_t, uint16_t> &gw_table, uint16_t gw_id)
{
	map<uint16_t, uint16_t>::const_iterator it;

	for( it = gw_table.begin(); it != gw_table.end(); ++it)
	{
		if(it->second == gw_id)
		{
			return true;
		}
	}
	return false;
}

bool OpenSandConfFile::getSpot(string section, 
                               uint8_t spot_id, 
                               uint16_t gw_id,
                               ConfigurationList &current_gw)
{
	ConfigurationList spot_list;
	ConfigurationList current_spot;

	if(!Conf::getListNode(Conf::section_map[section], 
	                      SPOT_LIST, spot_list))
	{
		return false;;
	}
	
	if(!Conf::getElementWithAttributeValue(spot_list, ID,
	                                       spot_id, current_spot))
	{
		return false;
	}

	if(gw_id != NO_GW)
	{
		 if(!Conf::getElementWithAttributeValue(current_spot, GW,
	                                       gw_id, current_gw))
		{
			return false;
		}
	}else{
		current_gw = current_spot;
	}

	return true;
}

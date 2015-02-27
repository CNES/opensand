/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file Configuration.cpp
 * @brief Global interface for configuration file reading
 * @author Viveris Technologies
 */


#include "Configuration.h"
#include "ConfigurationFile.h"


ConfigurationFile Conf::global_config;
map <string, ConfigurationList> Conf::section_map;
map <unsigned int, uint8_t> Conf::carrier_map;
map <uint16_t, uint8_t> Conf::terminal_map;

Conf::Conf()
{
}

Conf::~Conf()
{
	section_map.clear();
	carrier_map.clear();
	terminal_map.clear();
	global_config.unloadConfig();
}

bool Conf::loadConfig(const string conf_file)
{
	if(!global_config.loadConfig(conf_file))
	{
		return false;
	}
	else
	{
		loadMap();
		return true;
	}
}

bool Conf::loadConfig(const vector<string> conf_files)
{
	if(!global_config.loadConfig(conf_files))
	{
		return false;
	}
	else
	{
		loadMap();
		return true;
	}
}

bool Conf::getComponent(string &compo)
{
	return global_config.getComponent(compo);
}

bool Conf::getListNode(ConfigurationList sectionList,
                       const char *key,
                       xmlpp::Node::NodeList &nodeList)
{
	return global_config.getListNode(sectionList, key, nodeList);
}


bool Conf::getElementWithAttributeValue(ConfigurationList list,
                                        const char *attribute_name,
                                        const char *attribute_value,
                                        ConfigurationList &elements)
{
	return global_config.getElementWithAttributeValue(list, attribute_name,
	                                                  attribute_value,
	                                                  elements);
}

bool Conf::getNbListItems(ConfigurationList section,
                          const char *key,
                          int &nbr)
{
	return global_config.getNbListItems(section, key, nbr);
}

bool Conf::getNbListItems(ConfigurationList section,
                          const char *key,
                          unsigned int &nbr)
{
	return global_config.getNbListItems(section, key, (int &)nbr);
}



bool Conf::getListItems(ConfigurationList section,
                        const char *key,
                        ConfigurationList &list)
{
	return global_config.getListItems(section, key, list);
}

bool Conf::loadLevels(map<string, log_level_t> &levels,
                      map<string, log_level_t> &specific)
{
	return global_config.loadLevels(levels, specific);
}

void Conf::loadMap(void)
{
	global_config.loadSectionMap(Conf::section_map);
	global_config.loadCarrierMap(Conf::carrier_map);
	global_config.loadTerminalMap(Conf::terminal_map);
}


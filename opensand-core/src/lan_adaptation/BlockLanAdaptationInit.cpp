/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file BlockLanAdaptationInit.cpp
 * @brief BlockLanAdaptation initialisation and termination
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#define DBG_PACKAGE PKG_QOS_DATA
#include <opensand_conf/uti_debug.h>

#include "BlockLanAdaptation.h"
#include "ServiceClass.h"
#include "TrafficCategory.h"
#include "Ipv4Address.h"
#include "Ipv6Address.h"
#include "Plugin.h"

#include <opensand_conf/ConfigurationFile.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <algorithm>



/**
 * Read configuration parameters;
 * instantiate all service classes and traffic flow categories
 * open the record file if necessary
 * instantiate IPv4 and IPv6 SARP tables
 *
 * @return true on success, false otherwise
 */
bool BlockLanAdaptation::getConfig()
{
	string sat_type;
	int dflt = -1;

	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE, sat_type))
	{
		UTI_ERROR("%s missing from section %s.\n", GLOBAL_SECTION, SATELLITE_TYPE);
		return false;
	}
	UTI_INFO("satellite type = %s\n", sat_type.c_str());
	this->satellite_type = strToSatType(sat_type);

	if(!this->initTrafficClasses())
	{
		return false;
	}
	if(!this->initTrafficCategories())
	{
		return false;
	}
	if(!globalConfig.getValue(SARP_SECTION, DEFAULT, dflt))
	{
		UTI_ERROR("cannot get default destination terminal, "
		          "this is not fatal\n");
		// do not return, this is not fatal
	}
	this->sarp_table.setDefaultTal(dflt);
	if(!this->initSarpTables())
	{
		return false;
	}
	if(!this->initLanAdaptationPlugin())
	{
		return false;
	}
	this->stats_timer = this->downward->addTimerEvent("LanAdaptationStats",
	                                                  this->stats_period);
	return true;
}


bool BlockLanAdaptation::initTrafficClasses()
{
	int class_nbr = 0;
	int i = 0;
	int nb;
	ConfigurationList class_config_list;
	ConfigurationList::iterator iter;

	if(!globalConfig.getNbListItems(SECTION_CLASS, CLASS_LIST, nb))
	{
		UTI_ERROR("missing or empty section [%s, %s]\n",
		          SECTION_CLASS, CLASS_LIST);
		return false;
	}
	UTI_DEBUG("%d lines in section [%s]\n", nb, SECTION_CLASS);
	this->class_list.resize(nb);

	// Service classes
	if(!globalConfig.getListItems(SECTION_CLASS, CLASS_LIST, class_config_list))
	{
		UTI_ERROR("missing or empty section [%s, %s]\n",
		          SECTION_CLASS, CLASS_LIST);
		return false;
	}

	for(iter = class_config_list.begin(); iter != class_config_list.end(); iter++)
	{
		long int class_id;
		long int sched_prio;
		long int mac_queue_id;
		string class_name;

		i++;
		// get class ID
		if(!globalConfig.getAttributeValue(iter, CLASS_ID, class_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", SECTION_CLASS, CLASS_LIST,
			           CLASS_ID, i);
			return false;
		}
		// get class name
		if(!globalConfig.getAttributeValue(iter, CLASS_NAME, class_name))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", SECTION_CLASS, CLASS_LIST,
			          CLASS_NAME, i);
			return false;
		}
		// get scheduler priority
		if(!globalConfig.getAttributeValue(iter, CLASS_SCHED_PRIO, sched_prio))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", SECTION_CLASS, CLASS_LIST,
			          CLASS_SCHED_PRIO, i);
			return false;
		}
		// get mac queue ID
		if(!globalConfig.getAttributeValue(iter, CLASS_MAC_ID, mac_queue_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", SECTION_CLASS, CLASS_LIST,
			          CLASS_MAC_ID, i);
			return false;
		}

		this->class_list[class_nbr].id = class_id;
		this->class_list[class_nbr].schedPrio = sched_prio;
		this->class_list[class_nbr].macQueueId = mac_queue_id;
		this->class_list[class_nbr].name = class_name;
		class_nbr++;
	}
	this->class_list.resize(class_nbr); // some lines may have been rejected
	sort(this->class_list.begin(), this->class_list.end());

	return true;
}

bool BlockLanAdaptation::initTrafficCategories()
{
	int i = 0;
	TrafficCategory *category;
	vector<TrafficCategory *>::iterator cat_iter;
	ConfigurationList category_list;
	ConfigurationList::iterator iter;
	ServiceClass service_class;
	vector <ServiceClass>::iterator class_iter;

	// Traffic flow categories
	if(!globalConfig.getListItems(SECTION_CATEGORY, CATEGORY_LIST,
	                              category_list))
	{
		UTI_ERROR("missing or empty section [%s, %s]\n",
		          SECTION_CATEGORY, CATEGORY_LIST);
		return false;
	}

	for(iter = category_list.begin(); iter != category_list.end(); iter++)
	{
		long int category_id;
		long int category_service;
		string category_name;

		i++;
		// get category id
		if(!globalConfig.getAttributeValue(iter, CATEGORY_ID, category_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", SECTION_CATEGORY, CATEGORY_LIST,
			          CATEGORY_ID, i);
			return false;
		}
		// get category name
		if(!globalConfig.getAttributeValue(iter, CATEGORY_NAME, category_name))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", SECTION_CATEGORY, CATEGORY_LIST,
			          CATEGORY_NAME, i);
			return false;
		}
		// get service class
		if(!globalConfig.getAttributeValue(iter, CATEGORY_SERVICE,
		                                   category_service))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", SECTION_CATEGORY, CATEGORY_LIST,
			          CATEGORY_SERVICE, i);
			return false;
		}

		service_class.id = category_service;
		class_iter = find(this->class_list.begin(),
		                  this->class_list.end(), service_class);
		if(class_iter == this->class_list.end())
		{
			UTI_ERROR("Traffic category %ld rejected: class id %d "
			          "unknown\n", category_id, service_class.id);
			return false;
		}
		if(this->category_map.count(category_id))
		{
			UTI_ERROR("Traffic category %ld - [%s] rejected: identifier "
			          "already exists for [%s]\n", category_id,
			          category_name.c_str(), this->category_map[category_id]->name.c_str());
			return false;
		}

		category = new TrafficCategory();

		category->id = category_id;
		category->name = category_name;
		category->svcClass = &(*class_iter);

		(*class_iter).categoryList.push_back(category);
		this->category_map[category_id] = category;
	}
	// Get default category
	if(!globalConfig.getValue(SECTION_CATEGORY, KEY_DEF_CATEGORY,
	                          this->default_category))
	{
		this->default_category = (this->category_map.begin())->first;
		UTI_ERROR("cannot find default traffic category\n");
		return false;
	}

	// Check classes and categories; display configuration
	for(class_iter = this->class_list.begin(); class_iter != this->class_list.end();)
	{
		service_class = *class_iter;

		// A service class should at least contain one category, otherwise reject it
		if(service_class.categoryList.size() == 0)
		{
			UTI_ERROR("Service class %s (%d) rejected: no traffic category\n",
			          service_class.name.c_str(), service_class.id);
			class_iter = this->class_list.erase(class_iter);
			// class_iter points now to the element following the one erased
			return false;
		}
		else
		{
			// change for next class (we cannot put it inside the for() because
			// erase() is used in the loop and the pointer is no longer valid
			// after a call to erase()
			class_iter++;
		}

		// display the Service Class parameters and categories
		UTI_DEBUG("class %s (%d): schedPrio %d, macQueueId %d, "
		          "nb categories %zu\n", service_class.name.c_str(),
		          service_class.id, service_class.schedPrio, service_class.macQueueId,
		          service_class.categoryList.size());
		for(cat_iter = service_class.categoryList.begin();
		    cat_iter != service_class.categoryList.end(); cat_iter++)
		{
			UTI_DEBUG("   category %s (%d)\n",
			          (*cat_iter)->name.c_str(), (*cat_iter)->id);
		}
	}
	UTI_INFO("lan adapation bloc activated with %zu service classes\n",
	         this->class_list.size());


	return true;
}

bool BlockLanAdaptation::initSarpTables()
{
	int i;

	long int tal_id;
	int mask;
	IpAddress *ip_addr;

	ConfigurationList terminal_list;
	ConfigurationList::iterator iter;

	// IPv4 SARP table
	if(!globalConfig.getListItems(SARP_SECTION, IPV4_LIST, terminal_list))
	{
		UTI_ERROR("missing section [%s, %s]\n", SARP_SECTION,
		          IPV4_LIST);
	}

	i = 0;
	for(iter = terminal_list.begin(); iter != terminal_list.end(); iter++)
	{
		string ipv4_addr;

		i++;
		// get the IPv4 address
		if(!globalConfig.getAttributeValue(iter, TERMINAL_ADDR, ipv4_addr))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, IPV4_LIST,
			          TERMINAL_ADDR, i);
			return false;
		}
		// get the IPv4 mask
		if(!globalConfig.getAttributeValue(iter, TERMINAL_IP_MASK, mask))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, IPV4_LIST,
			          TERMINAL_IP_MASK, i);
			return false;
		}
		// get the terminal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, IPV4_LIST,
			          TAL_ID, i);
			return false;
		}
		ip_addr = new Ipv4Address(ipv4_addr);

		UTI_DEBUG("%s/%d -> tal id %ld \n",
		          ip_addr->str().c_str(), mask, tal_id);

		this->sarp_table.add(ip_addr, mask, tal_id);
	} // for all IPv4 entries

	// IPv6 SARP table
	terminal_list.clear();
	if(!globalConfig.getListItems(SARP_SECTION, IPV6_LIST, terminal_list))
	{
		UTI_ERROR("missing section [%s, %s]\n", SARP_SECTION,
		          IPV6_LIST);
	}

	i = 0;
	for(iter = terminal_list.begin(); iter != terminal_list.end(); iter++)
	{
		string ipv6_addr;

		i++;
		// get the IPv6 address
		if(!globalConfig.getAttributeValue(iter, TERMINAL_ADDR, ipv6_addr))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, IPV6_LIST,
			          TERMINAL_ADDR, i);
			return false;
		}
		// get the IPv6 mask
		if(!globalConfig.getAttributeValue(iter, TERMINAL_IP_MASK, mask))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, IPV6_LIST,
			          TERMINAL_IP_MASK, i);
			return false;
		}
		// get the terminal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, IPV6_LIST,
			          TAL_ID, i);
			return false;
		}

		ip_addr = new Ipv6Address(ipv6_addr);

		UTI_DEBUG("%s/%d -> tal id %ld \n",
		          ip_addr->str().c_str(), mask, tal_id);

		this->sarp_table.add(ip_addr, mask, tal_id);
	} // for all IPv6 entries

	// Ethernet SARP table
	// TODO we could only initialize IP or Ethernet tables according to stack
	terminal_list.clear();
	if(!globalConfig.getListItems(SARP_SECTION, ETH_LIST, terminal_list))
	{
		UTI_ERROR("missing section [%s, %s]\n", SARP_SECTION,
		          ETH_LIST);
	}

	i = 0;
	for(iter = terminal_list.begin(); iter != terminal_list.end(); iter++)
	{
		string addr;
		MacAddress *mac_addr;

		i++;
		// get the MAC address
		if(!globalConfig.getAttributeValue(iter, MAC_ADDR, addr))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, ETH_LIST,
			          MAC_ADDR, i);
			return false;
		}
		// get the terminal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SARP_SECTION, ETH_LIST,
			          TAL_ID, i);
			return false;
		}
		
		mac_addr = new MacAddress(addr);
		UTI_DEBUG("%s -> tal id %ld \n",
		          mac_addr->str().c_str(), tal_id);

		this->sarp_table.add(mac_addr, tal_id);
	} // for all Ethernet entries

	return true;
}

bool BlockLanAdaptation::initLanAdaptationPlugin()
{
	ConfigurationList lan_list;
	int lan_scheme_nbr;
	LanAdaptationPlugin *upper = NULL;
	LanAdaptationPlugin *plugin;

	// get the number of lan adaptation context to use
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, LAN_ADAPTATION_SCHEME_LIST,
	                                lan_scheme_nbr))
	{
		UTI_ERROR("Section %s, %s missing\n", GLOBAL_SECTION,
		          LAN_ADAPTATION_SCHEME_LIST);
		return false;
	}
	UTI_DEBUG("found %d lan adaptation contexts\n", lan_scheme_nbr);

	for(int i = 0; i < lan_scheme_nbr; i++)
	{
		string name;
		LanAdaptationPlugin::LanAdaptationContext *context;

		// get all the lan adaptation plugins to use from upper to lower
		if(!globalConfig.getValueInList(GLOBAL_SECTION, LAN_ADAPTATION_SCHEME_LIST,
		                                POSITION, toString(i), PROTO, name))
		{
			UTI_ERROR("Section %s, invalid value %d for parameter '%s'\n",
			          GLOBAL_SECTION, i, POSITION);
			return false;
		}

		if(!Plugin::getLanAdaptationPlugin(name, &plugin))
		{
			UTI_ERROR("cannot get plugin for %s lan adaptation",
			          name.c_str());
			return false;
		}

		context = plugin->getContext();
		this->contexts.push_back(context);
		if(upper == NULL &&
		   !context->setUpperPacketHandler(NULL,
		                                   this->satellite_type))
		{
			UTI_ERROR("cannot use %s for packets read on the interface",
			          context->getName().c_str());
			return false;
		}
		else if(upper && !context->setUpperPacketHandler(
										upper->getPacketHandler(),
										this->satellite_type))
		{
			UTI_ERROR("upper lan adaptation type %s is not supported by %s",
			          upper->getName().c_str(),
			          context->getName().c_str());
			return false;
		}
		upper = plugin;
		UTI_DEBUG("add lan adaptation: %s\n",
		          plugin->getName().c_str());
	}

	return true;
}


/**
 * Free all resources
 */
void BlockLanAdaptation::terminate()
{
	map<qos_t, TrafficCategory *>::iterator iter;

	// free all service classes
	this->class_list.clear();

	// free all traffic flow categories
	for(iter = this->category_map.begin();
	    iter != this->category_map.end(); ++iter)
	{
		delete(*iter).second;
	}
	this->category_map.clear();
}

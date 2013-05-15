/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file ipqos_init.cpp
 * @brief BlocIPQoS initialisation and termination
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <algorithm>

// Project includes
#define DBG_PACKAGE PKG_QOS_DATA
#include "opensand_conf/uti_debug.h"
#include "opensand_conf/ConfigurationFile.h"

#include "bloc_ip_qos.h"
#include "ServiceClass.h"
#include "TrafficCategory.h"

#define C_IPQOS_NO_MAX_LATENCY 0


/**
 * Read configuration parameters;
 * instantiate all service classes and traffic flow categories
 * open the record file if necessary
 * instantiate IPv4 and IPv6 SARP tables
 */
void BlocIPQoS::getConfig()
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[getConfig]";
	int nb;
	int class_nbr;
	int i;
	ServiceClass svcClass;
	vector < ServiceClass >::iterator classIter;
	ConfigurationList class_list;
	TrafficCategory *category;
	vector < TrafficCategory * >::iterator catIter;
	ConfigurationList category_list;
	ConfigurationList::iterator iter;

#define KEY_MISSING "%s: %s missing from section %s. \n",FUNCNAME

	UTI_DEBUG_LEVEL(1); // used only if not set in conf file

	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
							  this->_satellite_type))
	{
		UTI_ERROR(KEY_MISSING, GLOBAL_SECTION, SATELLITE_TYPE);
		exit(1);
	}
	UTI_INFO("%s: satellite type = %s\n", FUNCNAME,
			 this->_satellite_type.c_str());
	// Service classes
	if(!globalConfig.getNbListItems(SECTION_CLASS, CLASS_LIST, nb))
	{
		UTI_ERROR("%s: missing or empty section [%s, %s]\n", FUNCNAME,
		          SECTION_CLASS, CLASS_LIST);
		exit(1);
	}
	UTI_DEBUG("%d lines in section [%s]\n", nb, SECTION_CLASS);
	classList.resize(nb);

	// Service classes
	if(!globalConfig.getListItems(SECTION_CLASS, CLASS_LIST, class_list))
	{
		UTI_ERROR("%s: missing or empty section [%s, %s]\n", FUNCNAME,
		          SECTION_CLASS, CLASS_LIST);
		exit(1);
	}

	class_nbr = 0;
	i = 0;
	for(iter = class_list.begin(); iter != class_list.end(); iter++)
	{
		long int class_id;
		long int sched_prio;
		long int mac_queue_id;
		string class_name;

		i++;
		// get class ID
		if(!globalConfig.getAttributeValue(iter, CLASS_ID, class_id))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", FUNCNAME, SECTION_CLASS, CLASS_LIST,
			           CLASS_ID, i);
			continue;
		}
		// get class name
		if(!globalConfig.getAttributeValue(iter, CLASS_NAME, class_name))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", FUNCNAME, SECTION_CLASS, CLASS_LIST,
			          CLASS_NAME, i);
			continue;
		}
		// get scheduler priority
		if(!globalConfig.getAttributeValue(iter, CLASS_SCHED_PRIO, sched_prio))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", FUNCNAME, SECTION_CLASS, CLASS_LIST,
			          CLASS_SCHED_PRIO, i);
			continue;
		}
		// get mac queue ID
		if(!globalConfig.getAttributeValue(iter, CLASS_MAC_ID, mac_queue_id))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", FUNCNAME, SECTION_CLASS, CLASS_LIST,
			          CLASS_MAC_ID, i);
			continue;
		}

		classList[class_nbr].id = class_id;
		classList[class_nbr].schedPrio = sched_prio;
		classList[class_nbr].macQueueId = mac_queue_id;
		classList[class_nbr].name = class_name;
		class_nbr++;
	}
	classList.resize(class_nbr); // some lines may have been rejected
	sort(classList.begin(), classList.end());

	// Traffic flow categories
	if(!globalConfig.getListItems(SECTION_CATEGORY, CATEGORY_LIST,
	                              category_list))
	{
		UTI_ERROR("%s: missing or empty section [%s, %s]\n", FUNCNAME,
		          SECTION_CATEGORY, CATEGORY_LIST);
		exit(1);
	}

	i = 0;
	for(iter = category_list.begin(); iter != category_list.end(); iter++)
	{
		long int category_id;
		long int category_service;
		string category_name;

		i++;
		// get category id
		if(!globalConfig.getAttributeValue(iter, CATEGORY_ID, category_id))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", FUNCNAME, SECTION_CATEGORY, CATEGORY_LIST,
			          CATEGORY_ID, i);
			continue;
		}
		// get category name
		if(!globalConfig.getAttributeValue(iter, CATEGORY_NAME, category_name))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", FUNCNAME, SECTION_CATEGORY, CATEGORY_LIST,
			          CATEGORY_NAME, i);
			continue;
		}
		// get service class
		if(!globalConfig.getAttributeValue(iter, CATEGORY_SERVICE,
		                                   category_service))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "at line %d\n", FUNCNAME, SECTION_CATEGORY, CATEGORY_LIST,
			          CATEGORY_SERVICE, i);
			continue;
		}

		svcClass.id = category_service;
		classIter = find(classList.begin(), classList.end(), svcClass);
		if(classIter == classList.end())
		{
			UTI_ERROR("%s: Traffic category %ld rejected: class id %d "
			          "unknown\n", FUNCNAME, category_id, svcClass.id);
			continue;
		}
		if(categoryMap.count(category_id))
		{
			UTI_ERROR("%s: Traffic category %ld - [%s] rejected: identifier "
			          "already exists for [%s]\n", FUNCNAME, category_id,
			          category_name.c_str(), categoryMap[category_id]->name.c_str());
			continue;
		}

		category = new TrafficCategory();

		category->id = category_id;
		category->name = category_name;
		category->svcClass = &(*classIter);

		(*classIter).categoryList.push_back(category);
		categoryMap[category_id] = category;
	}
	// Get default category
	if(!globalConfig.getValue(SECTION_CATEGORY, KEY_DEF_CATEGORY,
	                          defaultCategory))
	{
		defaultCategory = (categoryMap.begin())->first;
		UTI_ERROR("%s: cannot find default traffic category, use %s instead\n",
		          FUNCNAME, categoryMap[defaultCategory]->name.c_str());
	}

	// Check classes and categories; display configuration
	for(classIter = classList.begin(); classIter != classList.end();)
	{
		svcClass = *classIter;

		// A service class should at least contain one category, otherwise reject it
		if(svcClass.categoryList.size() == 0)
		{
			UTI_ERROR("%s: Service class %s (%d) rejected: no traffic category\n",
			          FUNCNAME, svcClass.name.c_str(), svcClass.id);
			classIter = classList.erase(classIter);
			// classIter points now to the element following the one erased
			continue;
		}
		else
		{
			// change for next class (we cannot put it inside the for() because
			// erase() is used in the loop and the pointer is no longer valid
			// after a call to erase()
			classIter++;
		}

		// display the Service Class parameters and categories
		UTI_DEBUG("%s: class %s (%u): schedPrio %d, macQueueId %d, "
		          "nb categories %zu\n", FUNCNAME, svcClass.name.c_str(),
		          svcClass.id, svcClass.schedPrio, svcClass.macQueueId,
		          svcClass.categoryList.size());
		for(catIter = svcClass.categoryList.begin();
		    catIter != svcClass.categoryList.end(); catIter++)
		{
			UTI_DEBUG("%s:    category %s (%d)\n", FUNCNAME,
			          (*catIter)->name.c_str(), (*catIter)->id);
		}
	}
	UTI_INFO("%s: IP QoS activated with %zu service classes\n",
	         FUNCNAME, classList.size());

	// instantiate IPv4 and IPv6 SARP tables
	this->initSarpTables();
}

/**
 * Instantiate IPv4 and IPv6 SARP tables
 */
void BlocIPQoS::initSarpTables()
{
	const char *FUNCNAME = IPQOS_DBG_PREFIX "[initSarpTables]";
	int i;

	long int tal_id;
	int mask;
	IpAddress *ip_addr;

	ConfigurationList terminal_list;
	ConfigurationList::iterator iter;

	// IPv4 SARP table
	if(!globalConfig.getListItems(IPD_SECTION_V4, TERMINAL_LIST, terminal_list))
	{
		UTI_ERROR("%s: missing section [%s, %s]\n", FUNCNAME, IPD_SECTION_V4,
		          TERMINAL_LIST);
	}

	i = 0;
	for(iter = terminal_list.begin(); iter != terminal_list.end(); iter++)
	{
		string ipv4_addr;

		i++;
		// get the IPv4 address
		if(!globalConfig.getAttributeValue(iter, TERMINAL_ADDR, ipv4_addr))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "line %d\n", FUNCNAME, IPD_SECTION_V4, TERMINAL_LIST,
			          TERMINAL_ADDR, i);
			continue;
		}
		// get the IPv4 mask
		if(!globalConfig.getAttributeValue(iter, TERMINAL_IP_MASK, mask))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "line %d\n", FUNCNAME, IPD_SECTION_V4, TERMINAL_LIST,
			          TERMINAL_IP_MASK, i);
			continue;
		}
		// get the terminal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "line %d\n", FUNCNAME, IPD_SECTION_V4, TERMINAL_LIST,
			          TAL_ID, i);
			continue;
		}
		ip_addr = new Ipv4Address(ipv4_addr);

		UTI_DEBUG("%s: %s/%d -> tal id %ld \n", FUNCNAME,
		          ip_addr->str().c_str(), mask, tal_id);

		this->sarpTable.add(ip_addr, mask, tal_id);
	} // for all IPv4 entries

	// IPv6 SARP table
	terminal_list.clear();
	if(!globalConfig.getListItems(IPD_SECTION_V6, TERMINAL_LIST, terminal_list))
	{
		UTI_ERROR("%s: missing section [%s, %s]\n", FUNCNAME, IPD_SECTION_V6,
		          TERMINAL_LIST);
	}

	i = 0;
	for(iter = terminal_list.begin(); iter != terminal_list.end(); iter++)
	{
		string ipv6_addr;

		i++;
		// get the IPv6 address
		if(!globalConfig.getAttributeValue(iter, TERMINAL_ADDR, ipv6_addr))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "line %d\n", FUNCNAME, IPD_SECTION_V6, TERMINAL_LIST,
			          TERMINAL_ADDR, i);
			continue;
		}
		// get the IPv6 mask
		if(!globalConfig.getAttributeValue(iter, TERMINAL_IP_MASK, mask))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "line %d\n", FUNCNAME, IPD_SECTION_V6, TERMINAL_LIST,
			          TERMINAL_IP_MASK, i);
			continue;
		}
		// get the terminal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			UTI_ERROR("%s: section '%s, %s': failed to retrieve %s at "
			          "line %d\n", FUNCNAME, IPD_SECTION_V6, TERMINAL_LIST,
			          TAL_ID, i);
			continue;
		}

		ip_addr = new Ipv6Address(ipv6_addr);

		UTI_DEBUG("%s: %s/%d -> tal id %ld \n", FUNCNAME,
		          ip_addr->str().c_str(), mask, tal_id);

		this->sarpTable.add(ip_addr, mask, tal_id);
	} // for all IPv6 entries
}

/**
 * Free all resources
 */
int BlocIPQoS::terminate()
{
	map < unsigned short, TrafficCategory * >::iterator iter;

	// free all service classes
	classList.clear();

	// free all traffic flow categories
	for(iter = categoryMap.begin(); iter != categoryMap.end(); iter++)
	{
		delete(*iter).second;
	}
	categoryMap.clear();

	return 0;
}

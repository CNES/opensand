/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
#include "platine_conf/uti_debug.h"
#include "platine_conf/ConfigurationFile.h"
#include "platine_margouilla/msg_ip.h"

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
	string str;
	int idx, nb, nbClass, val, ret;
	char name[30];
	unsigned short categoryId;
	ServiceClass svcClass;
	vector < ServiceClass >::iterator classIter;
	TrafficCategory *category;
	vector < TrafficCategory * >::iterator catIter;

#define KEY_MISSING "%s: %s missing from section %s. \n",FUNCNAME

	UTI_DEBUG_LEVEL(1); // used only if not set in conf file

	// Satellite Type (for GW)
	if(this->_host_name == "GW")
	{
		ret = globalConfig.getStringValue(GLOBAL_SECTION, SATELLITE_TYPE,
		                                  this->_satellite_type);
		if(ret < 0)
		{
			UTI_ERROR(KEY_MISSING, GLOBAL_SECTION, SATELLITE_TYPE);
			exit(1);
		}
		UTI_INFO("%s: satellite type = %s\n", FUNCNAME,
		         this->_satellite_type.c_str());
	}

	// Service classes
	nb = globalConfig.getNbListItems(SECTION_CLASS);
	if(nb <= 0)
	{
		UTI_ERROR("%s: missing or empty section [%s]\n", FUNCNAME, SECTION_CLASS);
		exit(1);
	}
	UTI_DEBUG("%d lines in section [%s]\n", nb, SECTION_CLASS);
	classList.resize(nb);
	nbClass = 0;
	for(idx = 0; idx < nb; idx++)
	{
		ret = globalConfig.getListItem(SECTION_CLASS, idx + 1, str);
		if(ret == 0)
		{
			sscanf(str.c_str(), "%u %s %u %u", &classList[idx].id, name,
			       &classList[idx].schedPrio, &classList[idx].macQueueId);
			classList[idx].name = name;
			nbClass++;
		}
	}
	classList.resize(nbClass); // some lines may have been rejected
	sort(classList.begin(), classList.end());

	// Traffic flow categories
	nb = globalConfig.getNbListItems(SECTION_CATEGORY);
	if(nb <= 0)
	{
		UTI_ERROR("%s: missing or empty section [%s]\n", FUNCNAME, SECTION_CATEGORY);
		exit(1);
	}
	UTI_DEBUG("%d lines in section [%s]\n", nb, SECTION_CATEGORY);
	for(idx = 1; idx <= nb; idx++)
	{
		ret = globalConfig.getListItem(SECTION_CATEGORY, idx, str);
		if(ret == 0)
		{
			sscanf(str.c_str(), "%hu %s %u", &categoryId, name,
			       &svcClass.id);
			classIter = find(classList.begin(), classList.end(), svcClass);
			if(classIter == classList.end())
			{
				UTI_ERROR("%s: Traffic category %d rejected: class id %d "
				          "unknown\n", FUNCNAME, categoryId, svcClass.id);
				continue;
			}
			if(categoryMap.count(categoryId))
			{
				UTI_ERROR("%s: Traffic category %d - [%s] rejected: identifier "
				          "already exists for [%s]\n", FUNCNAME, categoryId,
				          name, categoryMap[categoryId]->name.c_str());
				continue;
			}

			category = new TrafficCategory();

			category->id = categoryId;
			category->name = name;
			category->svcClass = &(*classIter);

			(*classIter).categoryList.push_back(category);
			categoryMap[categoryId] = category;
		}
	}
	// Get default category
	ret = globalConfig.getIntegerValue(SECTION_CATEGORY, KEY_DEF_CATEGORY, val);
	if(!ret)
		defaultCategory = val;
	else
		defaultCategory = (*(categoryMap.begin())).first;

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
		UTI_DEBUG("%s: class %s (%d): schedPrio %d, macQueueId %d, "
		          "nb categories %d\n", FUNCNAME, svcClass.name.c_str(),
		          svcClass.id, svcClass.schedPrio, svcClass.macQueueId,
		          svcClass.categoryList.size());
		for(catIter = svcClass.categoryList.begin();
		    catIter != svcClass.categoryList.end(); catIter++)
		{
			UTI_DEBUG("\tcategory %s (%d)\n", (*catIter)->name.c_str(),
			          (*catIter)->id);
		}
	}
	UTI_INFO("%s: IP QoS activated with %d service classes\n",
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
	int nbEntries;
	int ret;
	int i;

	unsigned long spotID;
	unsigned int talID;
	int maskLength;
	string strConfig;

	int ipv4[4];
	char buffer[255];
	struct in6_addr in6;
	IpAddress *ip_addr;

	// IPv4 SARP table
	nbEntries = globalConfig.getNbListItems(IPD_SECTION_V4);
	if(nbEntries <= 0)
	{
		UTI_ERROR("%s: missing or empty section [%s]\n", FUNCNAME, IPD_SECTION_V4);
		exit(1);
	}

	UTI_DEBUG("%s %d line(s) in section '%s'\n", FUNCNAME, nbEntries,
	          IPD_SECTION_V4);

	for(i = 0; i < nbEntries; i++)
	{
		ret = globalConfig.getListItem(IPD_SECTION_V4, i + 1, strConfig);
		if(ret < 0)
		{
			UTI_ERROR("%s cannot get listItem from section '%s'\n",
			          FUNCNAME, IPD_SECTION_V4);
			continue;
		}
		ret = sscanf(strConfig.c_str(), "%d.%d.%d.%d/%d %lu %u",
		             ipv4, ipv4 + 1, ipv4 + 2, ipv4 + 3,
		             &maskLength, &spotID, &talID);
		if(ret < 7)
		{
			UTI_DEBUG("%s scanf IPv4 with netmask failed (ret = %d), "
			          "trying without it\n", FUNCNAME, ret);

			ret = sscanf(strConfig.c_str(), "%d.%d.%d.%d %lu %u", ipv4,
			             ipv4 + 1, ipv4 + 2, ipv4 + 3, &spotID, &talID);
			if(ret < 6)
			{
				UTI_ERROR("%s bad IPv4 spot description (%d) in section '%s'\n",
				          FUNCNAME, i + 1, IPD_SECTION_V4);
				continue;
			}

			UTI_INFO("%s no netmask provided for IPv4, set it to 24\n",
			         FUNCNAME);
			maskLength = 24;
		}

		UTI_DEBUG("%s %d.%d.%d.%d/%d -> spot %lu -> tal id %u \n", FUNCNAME, ipv4[0],
		          ipv4[1], ipv4[2], ipv4[3], maskLength, spotID, talID);

		ip_addr = new Ipv4Address(ipv4[0], ipv4[1], ipv4[2], ipv4[3]);

		this->sarpTable.add(ip_addr, maskLength, spotID,talID);
	} // for all IPv4 entries

	// IPv6 SARP table
	nbEntries = globalConfig.getNbListItems(IPD_SECTION_V6);
	if(nbEntries <= 0)
	{
		UTI_ERROR("%s: missing or empty section [%s]\n", FUNCNAME, IPD_SECTION_V6);
		exit(1);
	}

	UTI_DEBUG("%s %d line(s) in section '%s'\n", FUNCNAME, nbEntries,
	          IPD_SECTION_V6);

	for(i = 0; i < nbEntries; i++)
	{
		ret = globalConfig.getListItem(IPD_SECTION_V6, i + 1, strConfig);
		if(ret < 0)
		{
			UTI_ERROR("%s cannot get listItem from section '%s'\n",
			          FUNCNAME, IPD_SECTION_V6);
			continue;
		}
		ret = sscanf(strConfig.c_str(), "%255[^/]/%d %lu %u", buffer,
		             &maskLength, &spotID, &talID);
		if(ret < 4)
		{
			UTI_DEBUG("%s scanf IPv6 with netmask failed (ret = %d), "
			          "trying whithout it", FUNCNAME, ret);

			ret = sscanf(strConfig.c_str(), "%255s %lu %u", buffer, &spotID, &talID);
			if(ret < 3)
			{
				UTI_ERROR("%s bad IPv6 spot description (%d) in section '%s'",
				          FUNCNAME, i + 1, IPD_SECTION_V6);
				continue;
			}

			UTI_INFO("%s no netmask provided for IPv6, set it to 64\n",
			         FUNCNAME);
			maskLength = 64;
		}

		UTI_DEBUG("%s %s/%d -> spot %lu %u\n", FUNCNAME, buffer,
		          maskLength, spotID, talID);

		inet_pton(AF_INET6, buffer, &in6);

		char *ip6 = (char *) &in6;

		ip_addr = new Ipv6Address(ip6[0], ip6[1], ip6[2], ip6[3],
		                          ip6[4], ip6[5], ip6[6], ip6[7],
		                          ip6[8], ip6[9], ip6[10], ip6[11],
		                          ip6[12], ip6[13], ip6[14], ip6[15]);

		this->sarpTable.add(ip_addr, maskLength, spotID,talID);
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

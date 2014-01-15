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
#include "TrafficCategory.h"
#include "Ipv4Address.h"
#include "Ipv6Address.h"
#include "Plugin.h"

#include <opensand_conf/ConfigurationFile.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <algorithm>


#define TUNTAP_BUFSIZE MAX_ETHERNET_SIZE // ethernet header + mtu + options, crc not included

bool BlockLanAdaptation::onInit(void)
{
	ConfigurationList lan_list;
	int lan_scheme_nbr;
	LanAdaptationPlugin *upper = NULL;
	LanAdaptationPlugin *plugin;
	string sat_type;
	sat_type_t satellite_type;
	lan_contexts_t contexts;
	int fd = -1;

	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE, sat_type))
	{
		UTI_ERROR("%s missing from section %s.\n", GLOBAL_SECTION, SATELLITE_TYPE);
		return false;
	}
	UTI_INFO("satellite type = %s\n", sat_type.c_str());
	satellite_type = strToSatType(sat_type);

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
		contexts.push_back(context);
		if(upper == NULL &&
		   !context->setUpperPacketHandler(NULL,
		                                   satellite_type))
		{
			UTI_ERROR("cannot use %s for packets read on the interface",
			          context->getName().c_str());
			return false;
		}
		else if(upper && !context->setUpperPacketHandler(
										upper->getPacketHandler(),
										satellite_type))
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

	this->is_tap = contexts.front()->handleTap();
	// create TUN or TAP virtual interface
	if(!this->allocTunTap(fd))
	{
		return false;
	}

	((Upward *)this->upward)->setContexts(contexts);
	((Downward *)this->downward)->setContexts(contexts);
	// we can share FD as one thread will write, the second will read
	((Upward *)this->upward)->setFd(fd);
	((Downward *)this->downward)->setFd(fd);

	return true;
}


bool BlockLanAdaptation::Downward::onInit(void)
{
	this->stats_timer = this->addTimerEvent("LanAdaptationStats",
	                                        this->stats_period);
	return true;
}

bool BlockLanAdaptation::Upward::onInit(void)
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
	return true;
}

void BlockLanAdaptation::Upward::setContexts(const lan_contexts_t &contexts)
{
	this->contexts = contexts;
}

void BlockLanAdaptation::Downward::setContexts(const lan_contexts_t &contexts)
{
	this->contexts = contexts;
}

void BlockLanAdaptation::Upward::setFd(int fd)
{
	this->fd = fd;
}

void BlockLanAdaptation::Downward::setFd(int fd)
{
	// add file descriptor for TUN/TAP interface
	this->addFileEvent("tun/tap", fd, TUNTAP_BUFSIZE + 4);
}

bool BlockLanAdaptation::Upward::initSarpTables(void)
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



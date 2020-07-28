/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @author Aurélien Delrieu <aurelien.delrieu@viveris.fr>
 */



#include "BlockLanAdaptation.h"
#include "TrafficCategory.h"
#include "Ipv4Address.h"
#include "Ipv6Address.h"
#include "Plugin.h"

#include <opensand_output/Output.h>

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
	lan_contexts_t contexts;
	int fd = -1;

	// get the number of lan adaptation context to use
	if(!Conf::getNbListItems(Conf::section_map[GLOBAL_SECTION],
		                     LAN_ADAPTATION_SCHEME_LIST,
	                         lan_scheme_nbr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n", GLOBAL_SECTION,
		    LAN_ADAPTATION_SCHEME_LIST);
		return false;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "found %d lan adaptation contexts\n", lan_scheme_nbr);

	{
		string name = "Ethernet";
		LanAdaptationPlugin::LanAdaptationContext *context;

		if(!Plugin::getLanAdaptationPlugin(name, &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get plugin for %s lan adaptation",
			    name.c_str());
			return false;
		}

		context = plugin->getContext();
		contexts.push_back(context);
		if(upper == NULL &&
		   !context->setUpperPacketHandler(NULL))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot use %s for packets read on the interface",
			    context->getName().c_str());
			return false;
		}
		else if(upper && !context->setUpperPacketHandler(upper->getPacketHandler()))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "upper lan adaptation type %s is not supported "
			    "by %s", upper->getName().c_str(),
			    context->getName().c_str());
			return false;
		}
		upper = plugin;
		LOG(this->log_init, LEVEL_INFO,
		    "add lan adaptation: %s\n",
		    plugin->getName().c_str());
	}

	// create TAP virtual interface
	if(!this->allocTap(fd))
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
	// statistics timer
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               STATS_TIMER,
	                   this->stats_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, STATS_TIMER);
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "statistics_timer set to %d\n",
	    this->stats_period_ms);
	this->stats_timer = this->addTimerEvent("LanAdaptationStats",
	                                        this->stats_period_ms);
	return true;
}

bool BlockLanAdaptation::Upward::onInit(void)
{
	int dflt = -1;

	if(!Conf::getValue(Conf::section_map[SARP_SECTION],
		               DEFAULT, dflt))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get default destination terminal, "
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
	// add file descriptor for TAP interface
	this->addFileEvent("tap", fd, TUNTAP_BUFSIZE + 4);
}

bool BlockLanAdaptation::Upward::initSarpTables(void)
{
	int i;

	tal_id_t tal_id = 0;

	ConfigurationList terminal_list;
	ConfigurationList::iterator iter;

	// Ethernet SARP table
	// TODO we could only initialize IP or Ethernet tables according to stack
	terminal_list.clear();
	if(!Conf::getListItems(Conf::section_map[SARP_SECTION],
		                   ETH_LIST, terminal_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "missing section [%s, %s]\n", SARP_SECTION,
		    ETH_LIST);
	}

	i = 0;
	for(iter = terminal_list.begin(); iter != terminal_list.end(); iter++)
	{
		string addr;
		MacAddress *mac_addr;

		i++;
		// get the MAC address
		if(!Conf::getAttributeValue(iter, MAC_ADDR, addr))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SARP_SECTION, ETH_LIST,
			    MAC_ADDR, i);
			return false;
		}
		// get the terminal ID
		if(!Conf::getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SARP_SECTION, ETH_LIST,
			    TAL_ID, i);
			return false;
		}

		mac_addr = new MacAddress(addr);
		LOG(this->log_init, LEVEL_INFO,
		    "%s -> tal id %u\n",
		    mac_addr->str().c_str(), tal_id);

		this->sarp_table.add(mac_addr, tal_id);
	} // for all Ethernet entries

	return true;
}



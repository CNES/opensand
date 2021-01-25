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


#include "OpenSandModelConf.h"
#include "BlockLanAdaptation.h"
#include "TrafficCategory.h"
#include "Plugin.h"

#include <opensand_output/Output.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <algorithm>
#include <vector>

#define TUNTAP_BUFSIZE MAX_ETHERNET_SIZE // ethernet header + mtu + options, crc not included

void BlockLanAdaptation::generateConfiguration()
{
	auto Conf = OpenSandModelConf::Get();

	auto global = OpenSandModelConf::Get()->getOrCreateComponent("encapsulation", "Encapsulation");
	auto lan_adaptation = global->addList("lan_adaptation_schemes", "LAN Adaptation scheme", "lan_adaptation_scheme")->getPattern();

	auto lan_adaptation_protocols = Plugin::generatePluginsConfiguration(lan_adaptation_plugin);
	lan_adaptation_protocols.push_back("IP");

	auto types = Conf->getModelTypesDefinition();
	types->addEnumType("lan_adaptation_protocol", "LAN Adaptation Protocol", lan_adaptation_protocols);
	lan_adaptation->addParameter("protocol", "Protocol", types->getType("lan_adaptation_protocol"));
}

bool BlockLanAdaptation::onInit(void)
{
	LanAdaptationPlugin *upper = NULL;
	LanAdaptationPlugin *plugin;
	lan_contexts_t contexts;
	int fd = -1;
	vector<string> lan_scheme_names({"Ethernet"});
	vector<string>::const_iterator iter;

	auto encap = OpenSandModelConf::Get()->getProfileData()->getComponent("encapsulation");
	for(auto& item : encap->getList("lan_adaptation_schemes")->getItems())
	{
		auto lan_adaptation_scheme = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);
		std::string name;

		// get all the lan adaptation plugins to use from upper to lower
		if(!OpenSandModelConf::extractParameterData(lan_adaptation_scheme->getParameter("protocol"), name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section encapsulation, missing parameter 'lan adaptation scheme protocol'\n");
			return false;
		}
		lan_scheme_names.push_back(name);
	}

	for(iter = lan_scheme_names.begin(); iter != lan_scheme_names.end(); iter++)
	{
		LanAdaptationPlugin::LanAdaptationContext *context;

		if(!Plugin::getLanAdaptationPlugin(*iter, &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get plugin for %s lan adaptation",
			    iter->c_str());
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
	if(!OpenSandModelConf::Get()->getStatisticsPeriod(this->stats_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'timers': missing parameter 'statistics'\n");
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
  return OpenSandModelConf::Get()->getSarp(this->sarp_table);
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


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
#include "Ethernet.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <algorithm>
#include <vector>

#define TUNTAP_BUFSIZE MAX_ETHERNET_SIZE // ethernet header + mtu + options, crc not included


void BlockLanAdaptation::generateConfiguration()
{
	Ethernet::generateConfiguration();
}

bool BlockLanAdaptation::onInit(void)
{
	LanAdaptationPlugin *plugin = Ethernet::constructPlugin();
	LanAdaptationPlugin::LanAdaptationContext *context = plugin->getContext();
	if(!context->setUpperPacketHandler(nullptr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot use %s for packets read on the interface",
		    context->getName().c_str());
		return false;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "add lan adaptation: %s\n",
	    plugin->getName().c_str());

	// create TAP virtual interface
	int fd = -1;
	if(!this->allocTap(fd))
	{
		return false;
	}

	lan_contexts_t contexts;
	contexts.push_back(context);
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


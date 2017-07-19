/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 CNES
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
 * @file Constant.cpp
 * @brief Constant
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "ConstantDelay.h"

#include <opensand_output/Output.h>

#include <sstream>

#define DELAY              "delay"
#define CONF_FILENAME      "constant_delay.conf"

ConstantDelay::ConstantDelay():
	SatDelayPlugin(),
	is_init(false)
{
}

ConstantDelay::~ConstantDelay()
{  
}

bool ConstantDelay::init()
{
	bool global_constant_delay;
	time_ms_t delay;
	ConfigurationFile config;
	string conf_file_path;
	conf_file_path = this->getConfPath() + string(CONF_FILENAME);

	if(this->is_init)
		return true;

	// If global delay, load delay from global conf
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   GLOBAL_CONSTANT_DELAY, global_constant_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", GLOBAL_CONSTANT_DELAY);
		goto error;
	}
	if(global_constant_delay)
	{
		if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
				   DELAY, delay))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get '%s' value", DELAY);
			goto error;
		}
	}
	else
	{
		// Else, load conf from plugin
		if(config.loadConfig(conf_file_path.c_str()) < 0)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to load config file '%s'",
			    conf_file_path.c_str());
			return false;
		}

		config.loadSectionMap(this->config_section_map);	

		if(!Conf::getValue(this->config_section_map[SAT_DELAY_CONF],
				   DELAY, delay))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Constant delay: cannot get %s",
			    DELAY);
			goto error;
		}
	}

	LOG(this->log_init, LEVEL_DEBUG,
			"Constant delay: %d ms", delay);
	this->setSatDelay(delay);
	// TODO: should is_init use a mutex??
	this->is_init = true;
	return true;
error:
	return false;
}

bool ConstantDelay::updateSatDelay()
{
	// Empty function, not necessary for a constant delay
	return true;
}

bool ConstantDelay::getMaxDelay(time_ms_t &delay)
{
	// Get delay from conf in case it is needed before the SatDelay
	// plugin is initialized
	if(!this->is_init)
		return false;
	delay = this->getSatDelay();
	return true;
}

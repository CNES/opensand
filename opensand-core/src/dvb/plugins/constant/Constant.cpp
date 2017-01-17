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


#include "Constant.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <sstream>

#define CONSTANT_SECTION "constant"
#define DELAY        "delay"
#define CONF_CST_FILENAME    "constant_delay.conf"

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
	ConfigurationFile config;
	string conf_cst_path;
	time_ms_t delay; 

	if(this->is_init)
		return true;

	conf_cst_path = this->getConfPath() + string(CONF_CST_FILENAME);

	if(config.loadConfig(conf_cst_path.c_str()) < 0)
	{   
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to load config file '%s'", conf_cst_path.c_str());
		goto error;
	}

	config.loadSectionMap(this->config_section_map);

	if(!config.getValue(this->config_section_map[CONSTANT_SECTION], 
		                DELAY, delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Constant delay: cannot get %s",
		    DELAY);
		goto error;
	}
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

time_ms_t ConstantDelay::getMaxDelay()
{
	// Get delay from conf in case it is needed before the SatDelay
	// plugin is initialized
	if(this->init())
		return this->getSatDelay();
	return 0;
}

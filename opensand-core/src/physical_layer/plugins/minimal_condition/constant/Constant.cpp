/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 CNES
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
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */


#include "Constant.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <sstream>

#define CONSTANT_SECTION "constant"
#define THRESHOLD        "threshold"
#define CONF_CST_FILE    "/etc/opensand/plugins/constant.conf"

Constant::Constant():
	MinimalConditionPlugin()
{
}

Constant::~Constant()
{  
}

bool Constant::init(void)
{
	ConfigurationFile config;

	if(config.loadConfig(CONF_CST_FILE) < 0)
	{   
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to load config file '%s'", CONF_CST_FILE);
		goto error;
	}

	config.loadMap(this->config_section_map);

	if(!config.getValue(this->config_section_map[CONSTANT_SECTION], 
		                THRESHOLD, this->minimal_cn))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Constant minimal conditions: cannot get %s",
		    THRESHOLD);
		goto error;
	}
	return true;
error:
	return false;
}


bool Constant::updateThreshold(uint8_t UNUSED(modcod_id))
{
	// Empty function, not necessary when
	// Constant Minimal conditions
	return true;
}


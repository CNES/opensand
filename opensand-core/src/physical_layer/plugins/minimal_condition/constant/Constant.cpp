/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "Constant.h"

#include <opensand_old_conf/conf.h>
#include <opensand_output/Output.h>

#include <sstream>

#define CONSTANT_SECTION "constant"
#define THRESHOLD        "threshold"
#define CONF_CST_FILENAME    "constant.conf"

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
	string conf_cst_path;
	conf_cst_path = this->getConfPath() + string(CONF_CST_FILENAME);

	if(!config.loadConfig(conf_cst_path.c_str()))
	{   
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to load config file '%s'", conf_cst_path.c_str());
		goto error;
	}

	config.loadSectionMap(this->config_section_map);

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


bool Constant::updateThreshold(uint8_t UNUSED(modcod_id), uint8_t UNUSED(message_type))
{
	// Empty function, not necessary when
	// Constant Minimal conditions
	return true;
}


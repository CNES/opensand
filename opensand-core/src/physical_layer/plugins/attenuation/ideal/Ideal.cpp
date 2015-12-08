/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 CNES
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
 * @file Ideal.cpp
 * @brief Ideal
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 */

#include "Ideal.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <sstream>

#define IDEAL_SECTION     "ideal"
#define IDEAL_LIST        "ideal_attenuations"
#define ATTENUATION_VALUE "attenuation_value"
#define CONF_IDEAL_FILE   "/etc/opensand/plugins/ideal.conf"


Ideal::Ideal():
	AttenuationModelPlugin()
{
}

Ideal::~Ideal()
{
}


bool Ideal::init(time_ms_t refresh_period_ms, string link)
{
	ConfigurationFile config;

	if(config.loadConfig(CONF_IDEAL_FILE) < 0)
	{   
		LOG(this->log_init, LEVEL_ERROR, 
		    "failed to load config file '%s'",
		    CONF_IDEAL_FILE);
		goto error;
	}

	config.loadSectionMap(this->config_section_map);

	this->refresh_period_ms = refresh_period_ms;

	if(!config.getValueInList(this->config_section_map[IDEAL_SECTION],
		                      IDEAL_LIST,
	                          LINK, link,
	                          ATTENUATION_VALUE, this->value))
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "Ideal attenuation %slink: cannot get %s",
		    link.c_str(), ATTENUATION_VALUE);
		goto error;
	}

	return true;
error:
	return false;
}

bool Ideal::updateAttenuationModel()
{
	this->attenuation = this->value;
	LOG(this->log_init, LEVEL_INFO, 
	    "Constant attenuation: %.2f dB\n", this->getAttenuation());

	return true;
}



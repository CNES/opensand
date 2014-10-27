/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file OnOff.cpp
 * @brief OnOff
 * @author Fatima LAHMOUAD <fatima.lahmouad@etu.enseeiht.fr>
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 */


#include "OnOff.h"

#include <opensand_conf/ConfigurationFile.h>
#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <fstream>
#include <sstream>

#define ON_OFF_SECTION   "on_off"
#define ON_OFF_LIST      "on_off_attenuations"
#define PERIOD_ON        "period_on"
#define PERIOD_OFF       "period_off"
#define AMPLITUDE        "amplitude"
#define CONF_ON_OFF_FILE "/etc/opensand/plugins/on_off.conf"


OnOff::OnOff():
	AttenuationModelPlugin(),
	duration_counter(0)
{
}

OnOff::~OnOff()
{
}


bool OnOff::init(time_ms_t refresh_period_ms, string link)
{
	ConfigurationFile config;

	if(config.loadConfig(CONF_ON_OFF_FILE) < 0)
	{   
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to load config file '%s'",
		    CONF_ON_OFF_FILE);
		goto error;
	}

	this->refresh_period_ms = refresh_period_ms;

	if(!config.getValueInList(ON_OFF_SECTION, ON_OFF_LIST,
	                          LINK, link,
	                          PERIOD_ON, this->on_duration))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "On/Off attenuation %slink: cannot get %s",
		    link.c_str(), PERIOD_ON);
		goto error;
	}

	if(!config.getValueInList(ON_OFF_SECTION, ON_OFF_LIST,
	                          LINK, link,
	                          PERIOD_OFF, this->off_duration))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "On/Off attenuation %slink: cannot get %s",
		    link.c_str(), PERIOD_OFF);
		goto error;
	}

	if(!config.getValueInList(ON_OFF_SECTION, ON_OFF_LIST,
	                          LINK, link,
	                          AMPLITUDE, this->amplitude))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "On/Off attenuation %slink: cannot get %s",
		    link.c_str(), AMPLITUDE);
		goto error;
	}

	return true;
error:
	return false;
}

bool OnOff::updateAttenuationModel()
{
	this->duration_counter = (this->duration_counter + 1) %
	                         (this->on_duration + this->off_duration);

	LOG(this->log_attenuation, LEVEL_INFO,
	    "Attenuation model counter %d\n", this->duration_counter);
	if(this->duration_counter < this->off_duration)
	{
		this->setAttenuation(0);
	}
	else
	{
		this->setAttenuation(this->amplitude);
	}

	LOG(this->log_attenuation, LEVEL_INFO,
	    "On/Off Attenuation %.2f dB\n", this->getAttenuation());
	return true;
}



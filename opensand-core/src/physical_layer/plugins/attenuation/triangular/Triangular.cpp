/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file Triangular.cpp
 * @brief Triangular
 * @author Fatima LAHMOUAD <fatima.lahmouad@etu.enseeiht.fr>
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 */


#include "Triangular.h"

#include <opensand_conf/conf.h>

#include <string>
#include <iostream>
#include <math.h>
#include <sstream>

#define TRIANGULAR_SECTION   "triangular"
#define TRIANGULAR_LIST      "triangular_attenuations"
#define SLOPE                "slope"
#define PERIOD               "period"
#define CONF_TRIANGULAR_FILENAME "triangular.conf"

Triangular::Triangular():
	AttenuationModelPlugin(),
	duration_counter(0)
{
}

Triangular::~Triangular()
{
}

bool Triangular::init(time_ms_t refresh_period_ms, string link)
{
	ConfigurationFile config;
	string conf_triangular_path;
	conf_triangular_path = this->getConfPath() + string(CONF_TRIANGULAR_FILENAME);

	if(config.loadConfig(conf_triangular_path.c_str()) < 0)
	{   
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to load config file '%s'", 
		    conf_triangular_path.c_str());
		goto error;
	}

	config.loadSectionMap(this->config_section_map);

	this->refresh_period_ms = refresh_period_ms;

	if(!config.getValueInList(this->config_section_map[TRIANGULAR_SECTION], 
		                      TRIANGULAR_LIST, LINK, link, 
		                      PERIOD, this->period))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Triangular attenuation %slink: cannot get %s",
		    link.c_str(), PERIOD);
		goto error;
	}

	if(!config.getValueInList(this->config_section_map[TRIANGULAR_SECTION], 
		                      TRIANGULAR_LIST, LINK, link, 
		                      SLOPE, this->slope))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Triangular attenuation %slink: cannot get %s",
		    link.c_str(), SLOPE);
		goto error;
	}

	return true;
error:
	return false;
}


bool Triangular::updateAttenuationModel()
{
	double time;

	this->duration_counter = (this->duration_counter + 1) % this->period;
	time = this->duration_counter * this->refresh_period_ms / 1000;

	if(time < this->period / 2)
	{
		this->setAttenuation(time * this->slope);
	}
	else
	{
		double max = this->period * this->slope * this->refresh_period_ms / 1000;
		this->setAttenuation(max - time * this->slope);
	}

	LOG(this->log_attenuation, LEVEL_INFO,
	    "On/Off Attenuation %.2f dB\n", this->getAttenuation());

	return true;
}



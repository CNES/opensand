/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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

#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include "opensand_conf/uti_debug.h"
#include <opensand_conf/ConfigurationFile.h>
#include "opensand_conf/conf.h"

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


bool Ideal::init(int granularity, string link)
{
	ConfigurationFile config;

	if(config.loadConfig(CONF_IDEAL_FILE) < 0)
	{   
		UTI_ERROR("failed to load config file '%s'",
		          CONF_IDEAL_FILE);
		goto error;
	}

	this->granularity = granularity;

	if(!config.getValueInList(IDEAL_SECTION, IDEAL_LIST,
	                          LINK, link,
	                          ATTENUATION_VALUE, this->value))
	{
		UTI_ERROR("Ideal attenuation %slink: cannot get %s",
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
	UTI_DEBUG("Constant attenuation: %f \n",
	          this->getAttenuation());

	return true;
}



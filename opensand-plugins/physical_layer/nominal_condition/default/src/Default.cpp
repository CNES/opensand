/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file Default.cpp
 * @brief Default
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */



#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include "Default.h"

#include <opensand_conf/ConfigurationFile.h>
#include <opensand_conf/conf.h>

#include <sstream>

#define DEFAULT_SECTION   "default"
#define DEFAULT_LIST      "default_nominal_conditions"
#define NOMINAL_CN        "nominal_cn"
#define CONF_DEFAULT_NOM_FILE "/etc/opensand/plugins/default.conf"

Default::Default():
	NominalConditionPlugin()
{
}

bool Default::init(string link)
{
	ConfigurationFile config;

	if(config.loadConfig(CONF_DEFAULT_NOM_FILE) < 0)
	{   
		UTI_ERROR("failed to load config file '%s'",
		          CONF_DEFAULT_NOM_FILE);
		goto error;
	}

	if(!config.getValueInList(DEFAULT_SECTION, DEFAULT_LIST,
	                          LINK, link,
	                          NOMINAL_CN, this->nominal_cn))
	{
		UTI_ERROR("Default attenuation %slink: cannot get %s",
		          link.c_str(), NOMINAL_CN);
		goto error;
	}

	return true;
error:
	return false;
}

Default::~Default()
{  
}


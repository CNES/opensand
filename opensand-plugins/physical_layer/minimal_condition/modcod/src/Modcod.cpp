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
 * @file Modcod.cpp
 * @brief Modcod
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include "Modcod.h"

#include <opensand_conf/ConfigurationFile.h>
#include <opensand_conf/conf.h>

#include <errno.h>
#include <string.h>

#define MODCOD_SECTION    "modcod"
#define MODCOD_PATH       "modcod_path"
#define CONF_MODCOD_FILE  "/etc/opensand/plugins/modcod.conf"

Modcod::Modcod():
	MinimalConditionPlugin(), modcod_table()
{
}

Modcod::~Modcod()
{
}


bool Modcod::init()
{
	ConfigurationFile config;
	string filename;

	if(config.loadConfig(CONF_MODCOD_FILE) < 0)
	{
		UTI_ERROR("failed to load config file '%s'",
		          CONF_MODCOD_FILE);
		goto error;
	}

	if(!config.getValue(MODCOD_SECTION, MODCOD_PATH,
	                    filename))
	{
		UTI_ERROR("Modcod minimal conditions: cannot get %s", MODCOD_PATH);
		goto error;
	}


	if(access(filename.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		          filename.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("modcod definition file for minimal condition = '%s'\n",
	         filename.c_str());

	// load all the MODCOD definitions from file
	if(!(this->modcod_table).load(filename))
	{
		UTI_ERROR("unable to load the modcod definition table");
		goto error;
	}

	return true;
error:
	return false;
}

bool Modcod::updateThreshold(T_DVB_HDR *hdr)
{
	double threshold;  // Value to be updated with the current MODCOD
	T_DVB_BBFRAME * bbheader;

	// Init variables
	threshold = this->minimal_cn; // Default, keep previous threshold
	if(hdr->msg_type == MSG_TYPE_BBFRAME)
	{
		bbheader = (T_DVB_BBFRAME *) hdr;
		threshold = (double)(this->modcod_table.getRequiredEsN0(bbheader->used_modcod));
		UTI_DEBUG("BBFrame: Required Es/N0 for Modcod %d --> %f \n",
		          bbheader->used_modcod,
		          this->modcod_table.getRequiredEsN0(bbheader->used_modcod));
	}

	this->minimal_cn = threshold;;
	return true;
}



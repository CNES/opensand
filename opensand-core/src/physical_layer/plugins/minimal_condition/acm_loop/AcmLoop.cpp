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
 * @file AcmLoop.cpp
 * @brief AcmLoop
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include "AcmLoop.h"

#include <opensand_conf/ConfigurationFile.h>
#include <opensand_conf/conf.h>

#include <errno.h>
#include <string.h>

AcmLoop::AcmLoop():
	MinimalConditionPlugin(), modcod_table()
{
}

AcmLoop::~AcmLoop()
{
}


bool AcmLoop::init(void)
{
	string val;
	string filename;
	string modcod_key;
	sat_type_t sat_type;
	component_t compo;

	// satellite type
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	UTI_INFO("satellite type = %s\n", val.c_str());
	sat_type = strToSatType(val);

	val = "";
	if(!globalConfig.getComponent(val))
	{
		UTI_ERROR("cannot get component type\n");
		goto error;
	}
	UTI_INFO("host type = %s\n", val.c_str());
	compo = getComponentType(val);

	if(compo == terminal ||
	   (sat_type == REGENERATIVE && compo == gateway))
	{
		modcod_key = DOWN_FORWARD_MODCOD_DEF;
	}
	else
	{
		modcod_key = UP_RETURN_MODCOD_DEF;
	}
	// get appropriate MODCOD definitions for receving link
	if(!globalConfig.getValue(GLOBAL_SECTION, modcod_key.c_str(),
	                          filename))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, modcod_key.c_str());
		goto error;
	}

	if(access(filename.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		          filename.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("ACM loop definition file for minimal condition = '%s'\n",
	         filename.c_str());

	// load all the ACM_LOOP definitions from file
	if(!(this->modcod_table).load(filename))
	{
		UTI_ERROR("unable to load the acm_loop definition table");
		goto error;
	}

	return true;
error:
	return false;
}

bool AcmLoop::updateThreshold(uint8_t modcod_id)
{
	double threshold;  // Value to be updated with the current ACM_LOOP

	// Init variables
	threshold = this->minimal_cn; // Default, keep previous threshold
	threshold = (double)(this->modcod_table.getRequiredEsN0(modcod_id));
	UTI_DEBUG("BBFrame: Required Es/N0 for ACM loop %u --> %.2f dB\n",
	          modcod_id,
	          this->modcod_table.getRequiredEsN0(modcod_id));

	this->minimal_cn = threshold;
	return true;
}



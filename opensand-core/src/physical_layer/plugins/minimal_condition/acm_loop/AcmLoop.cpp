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
 * @file AcmLoop.cpp
 * @brief AcmLoop
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */



#include "AcmLoop.h"

#include <opensand_conf/Configuration.h>
#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

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
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION], 
		               SATELLITE_TYPE,
	                   val))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, SATELLITE_TYPE);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "satellite type = %s\n", val.c_str());
	sat_type = strToSatType(val);

	val = "";
	if(!Conf::getComponent(val))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get component type\n");
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "host type = %s\n", val.c_str());
	compo = getComponentType(val);

	// TODO SCPC !
	if(compo == terminal ||
	   (sat_type == REGENERATIVE && compo == gateway))
	{
		modcod_key = MODCOD_DEF_S2;
	}
	else
	{
		modcod_key = MODCOD_DEF_RCS;
	}
	// get appropriate MODCOD definitions for receving link
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION], 
		               modcod_key.c_str(),
	                   filename))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, modcod_key.c_str());
		goto error;
	}

	if(access(filename.c_str(), R_OK) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot access '%s' file (%s)\n",
		    filename.c_str(), strerror(errno));
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "ACM loop definition file for minimal condition = '%s'\n",
	    filename.c_str());

	// load all the ACM_LOOP definitions from file
	if(!(this->modcod_table).load(filename))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to load the acm_loop definition table");
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
	LOG(this->log_minimal, LEVEL_DEBUG, 
	    "Required Es/N0 for ACM loop %u --> %.2f dB\n",
	    modcod_id, this->modcod_table.getRequiredEsN0(modcod_id));

	this->minimal_cn = threshold;
	return true;
}



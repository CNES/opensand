/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 CNES
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
#include "OpenSandFrames.h"

#include <opensand_conf/Configuration.h>
#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

AcmLoop::AcmLoop():
	MinimalConditionPlugin(), 
	modcod_table_rcs(),
	modcod_table_s2()
{
}

AcmLoop::~AcmLoop()
{
}


bool AcmLoop::init(void)
{
	string filename_rcs;
	string filename_s2;
	string modcod_def_rcs;
	
	return_link_standard_t return_link_standard;
	
	// return link standard type
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               RETURN_LINK_STANDARD,
	                   modcod_def_rcs))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, RETURN_LINK_STANDARD);
		return false;
	}
	return_link_standard = strToReturnLinkStd(modcod_def_rcs);
	modcod_def_rcs = return_link_standard == DVB_RCS ?
	                 MODCOD_DEF_RCS : MODCOD_DEF_RCS2;

	// get appropriate MODCOD definitions for receving link
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION], 
	                   modcod_def_rcs.c_str(),
	                   filename_rcs))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, modcod_def_rcs.c_str());
		return false;
	}

	// get appropriate MODCOD definitions for receving link
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION], 
		               MODCOD_DEF_S2,
	                   filename_s2))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, MODCOD_DEF_S2);
		return false;
	}
	
	if(access(filename_rcs.c_str(), R_OK) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot access '%s' file (%s)\n",
		    filename_rcs.c_str(), strerror(errno));
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "ACM loop definition file for minimal condition = '%s'\n",
	    filename_rcs.c_str());
	
	if(access(filename_s2.c_str(), R_OK) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot access '%s' file (%s)\n",
		    filename_s2.c_str(), strerror(errno));
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "ACM loop definition file for minimal condition = '%s'\n",
	    filename_s2.c_str());

	// load all the ACM_LOOP definitions from file
	if(!(this->modcod_table_rcs).load(filename_rcs))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to load the acm_loop definition table");
		return false;
	}
	
	if(!(this->modcod_table_s2).load(filename_s2))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to load the acm_loop definition table");
		return false;
	}

	return true;
}

bool AcmLoop::updateThreshold(uint8_t modcod_id, uint8_t message_type)
{
	double threshold;  // Value to be updated with the current ACM_LOOP

	// Init variables
	threshold = this->minimal_cn; // Default, keep previous threshold
	switch(message_type)
	{
		case MSG_TYPE_DVB_BURST:
			threshold = (double)(this->modcod_table_rcs.getRequiredEsN0(modcod_id));
			LOG(this->log_minimal, LEVEL_DEBUG, 
			    "Required Es/N0 for ACM loop %u --> %.2f dB\n",
				modcod_id, this->modcod_table_rcs.getRequiredEsN0(modcod_id));

		default:
			threshold = (double)(this->modcod_table_s2.getRequiredEsN0(modcod_id));
			LOG(this->log_minimal, LEVEL_DEBUG, 
			    "Required Es/N0 for ACM loop %u --> %.2f dB\n",
				modcod_id, this->modcod_table_s2.getRequiredEsN0(modcod_id));
	}
	
	this->minimal_cn = threshold;
	return true;
}



/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file DamaCtrlRcsCommon.cpp
 * @brief This library defines a generic DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieutoulouse.viveris.com>
 */


#include "DamaCtrlRcsCommon.h"
#include "TerminalContextDamaRcs.h"

#include <opensand_output/Output.h>


using namespace std;

/**
 * Constructor
 */
DamaCtrlRcsCommon::DamaCtrlRcsCommon(spot_id_t spot): DamaCtrl(spot)
{
}


/**
 * Destructor
 */
DamaCtrlRcsCommon::~DamaCtrlRcsCommon()
{
}


bool DamaCtrlRcsCommon::init()
{
	// Ensure parent init has been done
	if(!this->is_parent_init)
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "Parent 'init()' method must be called first.\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool DamaCtrlRcsCommon::createTerminal(TerminalContextDama **terminal,
                                 tal_id_t tal_id,
                                 rate_kbps_t cra_kbps,
                                 rate_kbps_t max_rbdc_kbps,
                                 time_sf_t rbdc_timeout_sf,
                                 vol_kb_t max_vbdc_kb)
{
	*terminal = new TerminalContextDamaRcs(tal_id,
	                                      cra_kbps,
	                                      max_rbdc_kbps,
	                                      rbdc_timeout_sf,
	                                      max_vbdc_kb,
	                                      this->converter);
	if(!(*terminal))
	{
		LOG(this->log_logon, LEVEL_ERROR,
		    "SF#%u: cannot allocate terminal %u\n",
		    this->current_superframe_sf, tal_id);
		return false;
	}
	return true;
}


bool DamaCtrlRcsCommon::removeTerminal(TerminalContextDama **terminal)
{
	delete *terminal;
	*terminal = NULL;
	return true;
}

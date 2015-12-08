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
 * @file RequestSimulator.cpp
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 *
 */

#include "RequestSimulator.h"

#include <errno.h>


RequestSimulator::RequestSimulator(spot_id_t spot_id,
                                   tal_id_t mac_id,
                                   sat_type_t sat_type,
                                   bool phy_layer,
	                               FILE** evt_file,
                                   ConfigurationList current_gw):
	satellite_type(sat_type),
	with_phy_layer(phy_layer),
	spot_id(spot_id),
	mac_id(mac_id),
	dvb_fifos(),
	event_file(NULL),
	simu_file(NULL),
	simu_st(-1),
	simu_rt(-1),
	simu_max_rbdc(-1),
	simu_max_vbdc(-1),
	simu_cr(-1),
	simu_interval(-1),
	simu_eof(false),
	log_request_simulation(NULL),
	log_init(NULL)
{
	this->log_init = Output::registerLog(LEVEL_WARNING,
	                                     "Spot_%d.InitRequestSimulation",
		                                 this->spot_id);
	this->log_request_simulation = Output::registerLog(LEVEL_WARNING,
	                                                   "Spot_%d.RequestSimulation",
		                                               this->spot_id);

	if(!this->initRequestSimulation(current_gw))
	{
		assert(0);
	}
	*evt_file = this->event_file;
	
}

RequestSimulator::~RequestSimulator()
{
	if(this->event_file)
	{
		fflush(this->event_file);
		fclose(this->event_file);
	}
	if(this->simu_file)
	{
		fclose(this->simu_file);
	}
	
	// delete fifos
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();
}

bool RequestSimulator::initRequestSimulation(ConfigurationList current_gw)
{
	memset(this->simu_buffer, '\0', SIMU_BUFF_LEN);
	string evt_type;
                                             
	// Get and open the event file
	if(!Conf::getValue(current_gw, DVB_EVENT_FILE, evt_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot load parameter %s from section %s\n",
		    DVB_EVENT_FILE, DVB_NCC_SECTION);
		return false;
	}

	if(evt_type != "none" 
		&& this->with_phy_layer)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot use simulated request with physical layer "
		    "because we need to add cni parameters in SAC (TBD!)\n");
		return false;
	}

	if(evt_type == "stdout")
	{
		this->event_file = stdout;
	}
	else if(evt_type == "stderr")
	{
		this->event_file = stderr;
	}
	else if(evt_type != "none")
	{
		this->event_file = fopen(evt_type.c_str(), "a");
		if(this->event_file == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "%s\n", strerror(errno));
		}
	}
	if(this->event_file == NULL && evt_type != "none")
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "no record file will be used for event\n");
	}
	else if(this->event_file != NULL)
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "events recorded in %s.\n", evt_type.c_str());
	}

	return true;
}




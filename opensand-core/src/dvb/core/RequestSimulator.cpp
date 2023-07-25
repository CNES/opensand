/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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

#include <errno.h>
#include <cstring>

#include <opensand_output/Output.h>

#include "RequestSimulator.h"
#include "DvbFifo.h"
#include "OpenSandModelConf.h"
#include "Except.h"


static void out_deletor(std::ostream* ostream_ptr)
{
	if (ostream_ptr)
	{
		ostream_ptr->flush();
		if (ostream_ptr != &std::cout && ostream_ptr != &std::cerr)
		{
			auto ofstream = static_cast<std::ofstream*>(ostream_ptr);
			ofstream->close();
			delete ofstream;
		}
	}
};


RequestSimulator::RequestSimulator(spot_id_t spot_id,
                                   tal_id_t mac_id,
                                   std::ostream* &evt_file):
	spot_id(spot_id),
	mac_id(mac_id),
	dvb_fifos(),
	event_file(nullptr, out_deletor),
	simu_st(-1),
	simu_rt(-1),
	simu_max_rbdc(-1),
	simu_max_vbdc(-1),
	simu_cr(-1),
	simu_interval(-1),
	simu_eof(false),
	log_request_simulation(nullptr),
	log_init(nullptr)
{
	auto output = Output::Get();

	this->log_init = output->registerLog(LEVEL_WARNING, Format("Spot_%d.InitRequestSimulation", this->spot_id));
	this->log_request_simulation = output->registerLog(LEVEL_WARNING, Format("Spot_%d.RequestSimulation", this->spot_id));

	auto initialized = this->initRequestSimulation();
	ASSERT(initialized, "Failure to initialise requests simulation");
	evt_file = this->event_file.get();
}

RequestSimulator::~RequestSimulator()
{
	// delete fifos
	this->dvb_fifos.clear();
}

void RequestSimulator::generateConfiguration()
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	auto conf = Conf->getOrCreateComponent("network", "Network", "The DVB layer configuration");
	conf->addParameter("event_file", "Event Trace File",
	                   types->getType("string"),
	                   "Should an event history be generated? "
	                   "Format would be acceptable for a simulation trace file. "
	                   "Leave empty to not generate anything.");
}

bool RequestSimulator::initRequestSimulation()
{
	// Get and open the event file
	std::string evt_type;
	auto ncc = OpenSandModelConf::Get()->getProfileData()->getComponent("network");
	if(!OpenSandModelConf::extractParameterData(ncc->getParameter("event_file"), evt_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot load parameter event_file from section ncc\n");
		return false;
	}

	if(evt_type == "stdout")
	{
		this->event_file.reset(&std::cout);
	}
	else if(evt_type == "stderr")
	{
		this->event_file.reset(&std::cerr);
	}
	else if(evt_type != "none")
	{
		try
		{
			this->event_file.reset(new std::ofstream(evt_type, std::ios::app));
		}
		catch (const std::bad_alloc&)
		{
		}
		if(!this->event_file || !(*this->event_file))
		{
			//LOG(this->log_init, LEVEL_ERROR,
			    //"%s\n", strerror(errno));
			LOG(this->log_init, LEVEL_ERROR,
			    "no record file will be used for event\n");
			this->event_file.reset();
		}
	}

	if(this->event_file != nullptr)
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "events recorded in %s.\n", evt_type);
	}

	return true;
}


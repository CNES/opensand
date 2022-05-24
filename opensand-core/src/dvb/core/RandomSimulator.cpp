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
 * @file RandomSimulator.cpp
 * @brief Random simutation
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 *
 */

#include "RandomSimulator.h"

#include <errno.h>


RandomSimulator::RandomSimulator(spot_id_t spot_id,
                                 tal_id_t mac_id,
                                 FILE** evt_file,
                                 int simu_st,
                                 int simu_rt,
                                 int simu_max_rbdc,
                                 int simu_max_vbdc,
                                 int simu_cr,
                                 int simu_interval):
	RequestSimulator(spot_id, mac_id, evt_file)
{
	this->simu_st = simu_st;
	this->simu_rt = simu_rt;
	this->simu_max_rbdc = simu_max_rbdc;
	this->simu_max_vbdc = simu_max_vbdc;
	this->simu_cr = simu_cr;
	this->simu_interval = simu_interval;

	LOG(this->log_init, LEVEL_NOTICE,
	    "random events simulated for %ld terminals with "
	    "%ld kb/s bandwidth, %ld kb/s max RBDC, "
	    "%ld kb max VBDC, a mean request of %ld kb/s "
	    "and a request amplitude of %ld kb/s)i\n",
	    this->simu_st, this->simu_rt, this->simu_max_rbdc,
	    this->simu_max_vbdc, this->simu_cr,
	    this->simu_interval);
	srandom(times(NULL));
}

RandomSimulator::~RandomSimulator()
{
}


bool RandomSimulator::simulation(std::list<DvbFrame *>* msgs,
                                 time_sf_t UNUSED(super_frame_counter))
{
	static bool initialized = false;

	int i;
	// BROADCAST_TAL_ID is maximum tal_id for emulated terminals
	tal_id_t sim_tal_id = BROADCAST_TAL_ID + 1;

	if(!initialized)
	{
		// TODO function initRandomSimu
		for(i = 0; i < this->simu_st; i++)
		{
			tal_id_t tal_id = sim_tal_id + i;
			LogonRequest *logon_req = new LogonRequest(tal_id, this->simu_rt,
			                                          this->simu_max_rbdc,
			                                          this->simu_max_vbdc);
			msgs->push_back((DvbFrame*)logon_req);

		}
		initialized = true;
	}

	for(i = 0; i < this->simu_st; i++)
	{
		uint32_t val;
		Sac *sac = new Sac(sim_tal_id + i);

		if(this->simu_interval)
		{
			val = this->simu_cr - this->simu_interval / 2 +
			      random() % this->simu_interval;
	    }
	    else
	    {
			val = this->simu_cr;
	    }
		sac->addRequest(0, access_dama_rbdc, val);
		sac->setAcm(0xffff);
		msgs->push_back((DvbFrame*)sac);
	}

	return true;
}


bool RandomSimulator::stopSimulation(void)
{
	return true;
}

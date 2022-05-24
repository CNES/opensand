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
 * @file RandomSimulator.h
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 *
 */

#ifndef RAMDOM_SIMULATOR_H
#define RAMDOM_SIMULATOR_H

#include "RequestSimulator.h"


class RandomSimulator: public RequestSimulator
{
 public:
	RandomSimulator(spot_id_t spot_id,
	                tal_id_t mac_id,
	                FILE** evt_file,
	                int simu_st,
	                int simu_rt,
	                int simu_max_rbdc,
	                int simu_max_vbdc,
	                int simu_cr,
	                int simu_interval);
	~RandomSimulator();

	/**
	 * Simulate event based on random generation
	 * @return true on success, false otherwise
	 */
	bool simulation(std::list<DvbFrame *>* msgs,
	                time_sf_t super_frame_counter);

	/**
	 * Stop simulation
	 * @return true on success, false otherwise
	 */ 
	bool stopSimulation(void);
};

#endif

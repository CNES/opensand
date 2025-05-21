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
 * @file RequestSimulator.h
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 *
 */

#ifndef REQUEST_SIMULATOR_H
#define REQUEST_SIMULATOR_H

#include <list>
#include <memory>
#include <fstream>

#include "OpenSandCore.h"
#include "DvbFrame.h"
#include "DvbFifoTypes.h"


class OutputLog;


enum Simulate
{
	none_simu,
	file_simu,
	random_simu,
};


class RequestSimulator 
{
public:
	RequestSimulator(spot_id_t spot_id,
	                 tal_id_t mac_id,
	                 std::ostream* &evt_file);
	virtual ~RequestSimulator();

	static void generateConfiguration();
	
	/**
	 * Simulate event based on an input file
	 * @return true on success, false otherwise
	 */
	virtual bool simulation(std::list<Rt::Ptr<DvbFrame>> &msgs,
	                        time_sf_t super_frame_counter) = 0;
	
	virtual bool stopSimulation() = 0;

	// statistics update
	//void updateStatistics();

protected:
	/** Read configuration for the request simulation
	 *
	 * @return  true on success, false otherwise
	 */
	bool initRequestSimulation();

	/// spot id
	uint8_t spot_id;
	
	// gw tal id
	uint8_t mac_id;

	/* Fifos */
	/// map of FIFOs per MAX priority to manage different queues
	fifos_t dvb_fifos;

	/// parameters for request simulation
	std::unique_ptr<std::ostream, void(*)(std::ostream*)> event_file;
	long simu_st;
	long simu_rt;
	long simu_max_rbdc;
	long simu_max_vbdc;
	long simu_cr;
	long simu_interval;
	bool simu_eof;

	// Output logs and events
	std::shared_ptr<OutputLog> log_request_simulation;
	std::shared_ptr<OutputLog> log_init;
};


#endif

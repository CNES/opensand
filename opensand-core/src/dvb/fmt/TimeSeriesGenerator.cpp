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
 * @file TimeSeriesGenerator.cpp
 * @brief TimeSeriesGenerator
 * @author Julien BERNARD
 */


#include "TimeSeriesGenerator.h"

TimeSeriesGenerator::TimeSeriesGenerator(string output):
	previous_modcods(),
	output_file(output.c_str()),
	index(0),
	simu_log(NULL)
{
	assert(!output_file.fail());
	this->simu_log = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.output");

	// initialize previous modcods with some values for all terminals
	for(tal_id_t tal_id = 0; tal_id < BROADCAST_TAL_ID; ++tal_id)
	{
		// the index is the terminal ID
		this->previous_modcods.push_back(0);
	}
}

TimeSeriesGenerator::~TimeSeriesGenerator()
{
	this->output_file.close();
}


bool TimeSeriesGenerator::add(const StFmtSimuList *const sts)
{
	bool update = false;
	set<tal_id_t>::const_iterator it;

	this->index++;

	if(this->output_file.fail())
	{
		LOG(this->simu_log, LEVEL_ERROR,
		    "Fail bit is set on MODCOD output file, "
		    "stop storing data\n");
		return false;
	}


	for(it = sts->begin(); it != sts->end(); ++it)
	{
		tal_id_t tal_id = (*it);
		fmt_id_t modcod_id = sts->getCurrentModcodId(tal_id);
		// add the new entry in the list
		// The index is the terminal ID
		if(this->previous_modcods[tal_id] == modcod_id)
		{
			// if this is the same MODCOD, no need to change and store it
			continue;
		}
		update = true;
		LOG(this->simu_log, LEVEL_DEBUG,
		    "Time serie has changed for terminal %u\n", tal_id);
		this->previous_modcods[tal_id] = modcod_id;
	}


	if(update)
	{
		LOG(this->simu_log, LEVEL_INFO,
		    "Update time series\n");
		this->output_file << this->index << " ";
		for(vector<fmt_id_t>::iterator it = this->previous_modcods.begin();
		    it != this->previous_modcods.end(); ++it)
		{
			this->output_file << (int)(*it) << " ";
		}
		this->output_file << std::endl;
		this->output_file.flush();
	}
	return true;
}



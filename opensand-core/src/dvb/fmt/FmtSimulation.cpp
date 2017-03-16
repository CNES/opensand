/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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
 * @file FmtSimulation.cpp
 * @brief The FMT simulation elements
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "FmtSimulation.h"

#include <opensand_output/Output.h>

#include <sstream>
#include <cstdlib>
#include <errno.h>
#include <string.h>
#include <unistd.h>


/**
 * @brief Check if a file exists
 *
 * @return true if the file is found, false otherwise
 */
inline bool fileExists(const string &filename)
{
	if(access(filename.c_str(), R_OK) < 0)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot access '%s' file (%s)\n",
		        filename.c_str(), strerror(errno));
		return false;
	}
	return true;
}

// TODO the GW columns are mainly unused at the moment, they could be used to get
//      the MODCODs the GW is able to decode

FmtSimulation::FmtSimulation():
	modcod_simu(NULL),
	file_time(0),
	current_time(0),
	is_modcod_simu_defined(false),
	acm_period_ms(0)
{
	this->log_fmt = Output::registerLog(LEVEL_WARNING, "Dvb.Fmt.Simulation");
}


/**
 * @brief Destroy a list of Satellite Terminals (ST)
 */
FmtSimulation::~FmtSimulation()
{
	if(this->modcod_simu)
	{
		// destructor closes the file
		delete this->modcod_simu;
	}
}


bool FmtSimulation::goFirstScenarioStep()
{
	if(!this->is_modcod_simu_defined)
	{
		return true;
	}

	// read next line of the modcod simulation file
	if(!this->setList(this->next_modcod_list, this->next_step))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to get the next line in the MODCOD "
		    "simulation file\n");
		return false;
	}

	// copy in modcod_list
	for(vector<string>::iterator it = this->next_modcod_list.begin();
	    it != this->next_modcod_list.end(); it++)
	{
		this->modcod_list.push_back(*it);
	}

	return true;
}


bool FmtSimulation::goNextScenarioStep(double &duration)
{
	double time_current_step = this->next_step;

	if(!this->is_modcod_simu_defined)
	{
		return true;
	}

	// next_modcod_list is now the current
	this->modcod_list.swap(this->next_modcod_list);
	this->next_modcod_list.clear();

	// read next line of the modcod simulation file
	if(!this->setList(this->next_modcod_list, this->next_step))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to get the next line in the MODCOD "
		    "simulation file\n");
		return false;
	}

	string str = "";
	for(size_t i = 0 ; i < this->next_modcod_list.size(); ++i)
	{
		str += this->next_modcod_list[i] + " ; ";
	}

	duration = (this->next_step - time_current_step) * this->acm_period_ms;

	return true;
}


bool FmtSimulation::setModcodSimu(const string &filename,
                                  time_ms_t acm_period_ms,
                                  bool loop)
{
	// we can not redefine the simulation file
	if(this->is_modcod_simu_defined)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot redefine the MODCOD simulation file\n");
		return false;
	}

	if(!fileExists(filename.c_str()))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "the file %s doesn't exist\n", filename.c_str());
		return false;
	}

	// open he simulation file
	this->modcod_simu = new ifstream(filename.c_str());
	if(!this->modcod_simu->is_open())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to open MODCOD simulation file '%s'\n",
		    filename.c_str());
		return false;
	}

	// TODO: check values in the file here

	this->is_modcod_simu_defined = true;
	this->loop = loop;

	this->acm_period_ms = acm_period_ms;

	if(!this->load())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to load MODCOD simulation file '%s'\n",
		    filename.c_str());
		return false;
	}


	return true;

}

bool FmtSimulation::load()
{
	vector<string> list;
	std::map<double, vector<string> >::const_iterator modcod_it;
	string line;
	std::istringstream line_stream;
	unsigned int line_number = 0;
	double time;
	string modcod;

	// get the next line in the file
	while(std::getline((*this->modcod_simu), line))
	{
		line_number++;

		if(line == "" || line[0] == '#' || 
		   (line.length() >= 2 && line[0] == '/' && line[1] == '*'))
		{
			continue;
		}

		// Clear previous flags (if any)
		line_stream.clear();
		line_stream.str(line);

		line_stream >> time;
		
		if(line_stream.bad() || line_stream.fail())
		{
			goto malformed;
		}

		list.clear();
		
		while(line_stream >> modcod)
		{
			list.push_back(modcod);
			modcod.clear();
		}

		if(!list.empty())
		{
			this->modcod[time] = list;
		}
	}
	
	modcod_it = this->modcod.begin();
	this->file_time = modcod_it->first;

	this->modcod_simu->close();
	return true;
malformed:
	this->modcod_simu->close();
	return false;
}

/**** private methods ****/

bool FmtSimulation::setList(vector<string> &list, double &time)
{
	std::map<double, vector<string> >::const_iterator modcod_it;
	std::map<double, vector<string> >::reverse_iterator rmodcod_it;
	modcod_it = this->modcod.upper_bound(this->file_time);
	list.clear();

	// modcod found
	// get the next line in the file
	if(modcod_it != this->modcod.end())
	{
		// Next entry in the configuration file
		this->file_time = modcod_it->first;
		list = modcod_it->second;
		time = this->current_time + this->file_time;
	}
	else if(this->loop)
	{
		//update system current time
		this->current_time += this->file_time;
		// we reached the end of the scenario, restart at beginning
		this->file_time = this->modcod.begin()->first;
		// in case file time is null there is a problem on loop
		if(this->file_time == 0)
		{
			this->file_time = 1;
		}
		list = this->modcod.begin()->second;
		time = this->current_time + this->file_time;
	}
	else // not loop
	{
		// we reached the end of the scenario, keep the last value
		this->current_time += 1;
		rmodcod_it = this->modcod.rbegin();
		this->file_time = modcod_it->first;
		list = rmodcod_it->second;
		time = this->current_time;
	}
	

	return true;
}


bool FmtSimulation::getIsModcodSimuDefined(void) const
{
	return this->is_modcod_simu_defined;
}


vector<string> FmtSimulation::getModcodList(void) const
{
	return this->modcod_list;
}



/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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


FmtSimulation::FmtSimulation():
	modcod_simu(NULL),
	is_modcod_simu_defined(false)
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

	duration = (this->next_step - time_current_step) * 1000;

	return true;
}


bool FmtSimulation::setModcodSimu(const string &filename)
{
	// we can not redefine the simulation file
	if(this->is_modcod_simu_defined)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot redefine the MODCOD simulation file\n");
		goto error;
	}

	if(!fileExists(filename.c_str()))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "the file %s doesn't exist\n", filename.c_str());
		goto error;
	}

	// open the simulation file
	this->modcod_simu = new ifstream(filename.c_str());
	if(!this->modcod_simu->is_open())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to open MODCOD simulation file '%s'\n",
		    filename.c_str());
		goto error;
	}

	// TODO: check values in the file here

	this->is_modcod_simu_defined = true;

	return true;

error:
	return false;
}


/**** private methods ****/

bool FmtSimulation::setList(vector<string> &list, double &time)
{
	std::stringbuf buf;
	std::stringstream line;
	std::stringbuf token;
	list.clear();

	// get the next line in the file
	this->modcod_simu->get(buf);
	if(buf.str() != "")
	{
		line.str(buf.str());
		// get the first element of the line (which is time)
		if(!line.fail())
		{
			token.str("");
			line.get(token, ' ');
			time = atof(token.str().c_str());
			line.ignore();
		}

		// get each element of the line
		while(!line.fail())
		{
			token.str("");
			line.get(token, ' ');
			list.push_back(token.str());
			line.ignore();
		}
	}

	// keep the last modcod when we reach the end of file
	if(this->modcod_simu->eof())
	{
		return true;
	}

	// check if getline returned an error
	if(this->modcod_simu->fail())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Error when getting next line of the simulation "
		    "file\n");
		return false;
	}

	// reset the error flags
	this->modcod_simu->clear();

	// jump after the '\n' as get does not read it
	this->modcod_simu->ignore();

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



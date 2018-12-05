/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file File.cpp
 * @brief File
 * @author Fatima LAHMOUAD <fatima.lahmouad@etu.enseeiht.fr>
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "File.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <errno.h>

#define FILE_SECTION   "file"
#define FILE_LIST      "file_attenuations"
#define PATH          "path"
#define LOOP          "loop_mode"
#define CONF_FILE_FILENAME "file.conf"

File::File():
	AttenuationModelPlugin(),
	current_time(0),
	attenuation(),
	loop(false)
{
}

File::~File()
{
	this->attenuation.clear();
}

bool File::init(time_ms_t refresh_period_ms, string link)
{
	string filename;
	ConfigurationFile config;
	string conf_file_path;
	conf_file_path = this->getConfPath() + string(CONF_FILE_FILENAME);

	if(config.loadConfig(conf_file_path.c_str()) < 0)
	{   
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to load config file '%s'",
		    conf_file_path.c_str());
		return false;
	}

	config.loadSectionMap(this->config_section_map);


	this->refresh_period_ms = refresh_period_ms;

	if(!config.getValueInList(this->config_section_map[FILE_SECTION], 
		                      FILE_LIST, LINK, link,
	                          PATH, filename))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE attenuation %slink: cannot get %s",
		    link.c_str(), PATH);
		return false;
	}

	if(!config.getValueInList(this->config_section_map[FILE_SECTION], 
		                      FILE_LIST, LINK, link,
	                          LOOP, this->loop))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE %slink: cannot get %s",
		    link.c_str(), LOOP);
		return false;
	}

	return this->load(filename);
}


bool File::load(string filename)
{
	string line;
	std::istringstream line_stream;
	unsigned int line_number = 0;

	std::ifstream file(filename.c_str());

	if(!file)
	{
		LOG(this->log_attenuation, LEVEL_ERROR,
		    "Cannot open file %s\n", filename.c_str());
		goto error;
	}

	while(std::getline(file, line))
	{
		unsigned int time;
		double attenuation;

		line_number++;

		// skip line if empty
		if(line == "" || line[0] == '#')
		{
			continue;
		}

		// Clear previous flags (if any)
		line_stream.clear();
		line_stream.str(line);

		line_stream >> time;
		if(line_stream.bad() || line_stream.fail())
		{
			LOG(this->log_attenuation, LEVEL_ERROR,
					"Bad syntax in file '%s', line %u: "
					"there should be a timestamp (integer) "
					"instead of '%s'\n",
					filename.c_str(), line_number,
					line.c_str());
			goto malformed;
		}

		// get attenuation
		line_stream >> attenuation;
		if(line_stream.bad() || line_stream.fail())
		{
			LOG(this->log_attenuation, LEVEL_ERROR,
			    "Error while parsing attenuation line %u\n",
			    line_number);
			goto malformed;
		}

		this->attenuation[time] = attenuation;

		LOG(this->log_attenuation, LEVEL_DEBUG,
		    "Entry: time: %u, attenuation: %.2f dB\n", time, attenuation);
	}

	file.close();
	return true;

malformed:
	LOG(this->log_attenuation, LEVEL_ERROR,
	    "Malformed attenuation configuration file '%s'\n",
	    filename.c_str());
	file.close();
 error:
	return false;
}



bool File::updateAttenuationModel()
{
	std::map<unsigned int, double>::const_iterator attenuation_it;
	unsigned int old_time, new_time;
	double old_attenuation, new_attenuation;
	double next_attenuation;

	this->current_time++;;

	LOG(this->log_attenuation, LEVEL_INFO,
	    "Updating attenuation scenario: current time: %u "
	    "(step: %u)\n", this->current_time,
	    this->refresh_period_ms / 1000);

	// Look for the next entry whose key is equal or greater than 'current_time'
	attenuation_it = this->attenuation.lower_bound(this->current_time);

	// Attenuation found
	if(attenuation_it != this->attenuation.end())
	{
		// Next entry in the configuration file
		new_time = attenuation_it->first;
		new_attenuation = attenuation_it->second;

		LOG(this->log_attenuation, LEVEL_DEBUG,
		    "New entry found: time: %u, value: %.2f\n",
		    new_time, new_attenuation);

		if(attenuation_it != this->attenuation.begin())
		{
			// Get previous entry in the configuration file
			double coef;
			attenuation_it--;

			old_time = attenuation_it->first;
			old_attenuation = attenuation_it->second;

			LOG(this->log_attenuation, LEVEL_DEBUG,
			    "Old time: %u, old attenuation: %.2f\n",
			    old_time, old_attenuation);

			// Linear interpolation
			coef = (new_attenuation - old_attenuation) /
			        (new_time - old_time);


			next_attenuation = old_attenuation +
			                   coef * (this->current_time - old_time);

			LOG(this->log_attenuation, LEVEL_DEBUG,
			    "Linear coef: %f, old step: %u\n",
			    coef, old_time);
		}
		else
		{
			// First (and potentially only) entry, use it
			LOG(this->log_attenuation, LEVEL_DEBUG,
			    "It is the first entry\n");
			next_attenuation = new_attenuation;
		}
	}
	else if(!this->loop)
	{
		LOG(this->log_attenuation, LEVEL_DEBUG,
		    "Reach end of simulation, keep the last value\n");
		// we reached the end of the scenario, keep the last value
		next_attenuation = this->attenuation.rend()->second;
	}
	else // loop
	{
		LOG(this->log_attenuation, LEVEL_DEBUG,
		    "Reach end of simulation, restart with the first value\n");
		// we reached the end of the scenario, restart at beginning
		attenuation_it = this->attenuation.begin();
		new_time = attenuation_it->first;
		next_attenuation = attenuation_it->second;
		this->current_time = 0;
	}

	LOG(this->log_attenuation, LEVEL_DEBUG,
	    "new attenuation value: %.2f\n", next_attenuation);

	this->setAttenuation(next_attenuation);

	return true;
}




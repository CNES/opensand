/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 CNES
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
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */


#include "FileDelay.h"

#include <opensand_output/Output.h>

#include <errno.h>

#define PATH           "path"
#define LOOP           "loop_mode"
#define CONF_FILENAME  "file_delay.conf"

FileDelay::FileDelay():
	SatDelayPlugin(),
	is_init(false),
	current_time(0),
	delays(),
	loop(false)
{
}

FileDelay::~FileDelay()
{
	this->delays.clear();
}

bool FileDelay::init()
{
	string filename;
	time_ms_t refresh_period_ms;
	ConfigurationFile config;
	string conf_file_path;
	conf_file_path = this->getConfPath() + string(CONF_FILENAME);

	if(this->is_init)
		return true;

	if(config.loadConfig(conf_file_path.c_str()) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to load config file '%s'",
		    conf_file_path.c_str());
		return false;
	}

	config.loadSectionMap(this->config_section_map);

	if(!Conf::getValue(Conf::section_map[SAT_DELAY_SECTION],
	                   REFRESH_PERIOD_MS, refresh_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE delay: cannot get %s", REFRESH_PERIOD_MS);
		return false;
	}

	this->refresh_period_ms = refresh_period_ms;

	if(!Conf::getValue(this->config_section_map[SAT_DELAY_CONF],
	                   PATH, filename))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE delay: cannot get %s", PATH);
		return false;
	}

	if(!Conf::getValue(this->config_section_map[SAT_DELAY_CONF],
	                   LOOP, this->loop))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE delay: cannot get %s", LOOP);
		return false;
	}

	return this->load(filename);
}


bool FileDelay::load(string filename)
{
	string line;
	std::istringstream line_stream;
	unsigned int line_number = 0;

	std::ifstream file(filename.c_str());

	if(!file)
	{
		LOG(this->log_delay, LEVEL_ERROR,
		    "Cannot open file %s\n", filename.c_str());
		goto error;
	}

	while(std::getline(file, line))
	{
		unsigned int time;
		time_ms_t delay;
		
		line_number++;
		
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
			LOG(this->log_delay, LEVEL_ERROR,
					"Bad syntax in file '%s', line %u: "
					"there should be a timestamp (integer) "
					"instead of '%s'\n",
					filename.c_str(), line_number,
					line.c_str());
			goto malformed;
		}

		// get delay
		line_stream >> delay;
		if(line_stream.bad() || line_stream.fail())
		{
			LOG(this->log_delay, LEVEL_ERROR,
			    "Error while parsing delay line %u\n",
			    line_number);
			goto malformed;
		}

		this->delays[time] = delay;

		LOG(this->log_delay, LEVEL_DEBUG,
		    "Entry: time: %u, delay: %d ms\n", time, delay);
	}

	file.close();
	// TODO: should is_init use a mutex??
	this->is_init = true;
	return true;

malformed:
	LOG(this->log_delay, LEVEL_ERROR,
	    "Malformed sat delay configuration file '%s'\n",
	    filename.c_str());
	file.close();
 error:
	return false;
}

bool FileDelay::updateSatDelay()
{
	std::map<unsigned int, time_ms_t>::const_iterator delay_it;
	unsigned int old_time, new_time;
	time_ms_t old_delay, new_delay, next_delay;

	this->current_time += this->refresh_period_ms / 1000;

	LOG(this->log_delay, LEVEL_INFO,
	    "Updating sat delay: current time: %u "
	    "(step: %u)\n", this->current_time,
	    this->refresh_period_ms / 1000);

	// Look for the next entry whose key is equal or greater than 'current_time'
	delay_it = this->delays.lower_bound(this->current_time);

	// Delay found
	if(delay_it != this->delays.end())
	{
		// Next entry in the configuration file
		new_time = delay_it->first;
		new_delay = delay_it->second;

		LOG(this->log_delay, LEVEL_DEBUG,
		    "New entry found: time: %u, value: %d\n",
		    new_time, new_delay);

		if(delay_it != this->delays.begin())
		{
			// Get previous entry in the configuration file
			double coef;
			delay_it--;

			old_time = delay_it->first;
			old_delay = delay_it->second;

			LOG(this->log_delay, LEVEL_DEBUG,
			    "Old time: %u, old delay: %d\n",
			    old_time, old_delay);

			// Linear interpolation
			coef = (((double)new_delay) - old_delay) /
			        (((double)new_time) - old_time);


			next_delay = old_delay +
			                   (time_ms_t) (coef * (this->current_time - old_time));

			LOG(this->log_delay, LEVEL_DEBUG,
			    "Linear coef: %f, old step: %u\n",
			    coef, old_time);
		}
		else
		{
			// First (and potentially only) entry, use it
			LOG(this->log_delay, LEVEL_DEBUG,
			    "It is the first entry\n");
			next_delay = new_delay;
		}
	}
	else if(!this->loop)
	{
		LOG(this->log_delay, LEVEL_DEBUG,
		    "Reach end of simulation, keep the last value\n");
		// we reached the end of the scenario, keep the last value
		next_delay = this->delays.rend()->second;
	}
	else // loop
	{
		LOG(this->log_delay, LEVEL_DEBUG,
		    "Reach end of simulation, restart with the first value\n");
		// we reached the end of the scenario, restart at beginning
		delay_it = this->delays.begin();
		new_time = delay_it->first;
		next_delay = delay_it->second;
		this->current_time = 0;
	}

	LOG(this->log_delay, LEVEL_DEBUG,
	    "new delay value: %d\n", next_delay);

	this->setSatDelay(next_delay);

	return true;
}

bool FileDelay::getMaxDelay(time_ms_t &delay)
{
	std::map<unsigned int, time_ms_t>::const_iterator delay_it;
	if(!this->is_init)
	{
		return false;
	}
	for(delay_it = this->delays.begin(); delay_it != this->delays.end(); delay_it++)
	{
		if(delay_it->second > delay)
			delay = delay_it->second;
	}
	return true;
}

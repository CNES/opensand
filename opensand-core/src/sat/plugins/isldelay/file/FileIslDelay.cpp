/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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


#include "FileIslDelay.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>

#include <algorithm>
#include <sstream>
#include <fstream>


std::string FileIslDelay::config_path = "";


FileIslDelay::FileIslDelay():
		IslDelayPlugin(),
		is_init(false),
		current_time(0),
		delays(),
		loop(false)
{
}


FileIslDelay::~FileIslDelay()
{
	this->delays.clear();
}


void FileIslDelay::generateConfiguration(const std::string &parent_path,
                                      const std::string &param_id,
                                      const std::string& plugin_name)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();

	FileIslDelay::config_path = parent_path;
	auto delay = Conf->getComponentByPath(parent_path);
	if (delay == nullptr)
	{
		return;
	}
	auto delay_type = delay->getParameter(param_id);
	if (delay_type == nullptr)
	{
		return;
	}

	auto path = delay->addParameter("file_path", "File Path", types->getType("string"));
	Conf->setProfileReference(path, delay_type, plugin_name);
	auto refresh_period = delay->addParameter("refresh_period", "Refresh Period", types->getType("uint"));
	refresh_period->setUnit("ms");
	Conf->setProfileReference(refresh_period, delay_type, plugin_name);
	auto loop = delay->addParameter("loop", "Loop Mode", types->getType("bool"));
	Conf->setProfileReference(loop, delay_type, plugin_name);
}


bool FileIslDelay::init()
{
	auto delay = OpenSandModelConf::Get()->getProfileData(config_path);

	if(this->is_init)
	{
		return true;
	}

	uint32_t refresh_period_ms;
	auto period_parameter = delay->getParameter("refresh_period");
	if(!OpenSandModelConf::extractParameterData(period_parameter, refresh_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE delay: cannot get refresh period");
		return false;
	}
	this->refresh_period = time_ms_t(refresh_period_ms);

	std::string filename;
	auto path_parameter = delay->getParameter("file_path");
	if(!OpenSandModelConf::extractParameterData(path_parameter, filename))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE delay: cannot get file path");
		return false;
	}

	auto loop_parameter = delay->getParameter("loop");
	if(!OpenSandModelConf::extractParameterData(loop_parameter, loop))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "FILE delay: cannot get loop mode");
		return false;
	}

	return this->load(filename);
}


bool FileIslDelay::load(std::string filename)
{
	std::string line;
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
		time_ms_t::rep delay;
		
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
					filename, line_number, line);
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

		this->delays[time] = time_ms_t(delay);

		LOG(this->log_delay, LEVEL_DEBUG,
		    "Entry: time: %u, delay: %u ms\n",
			time, delay);
	}

	file.close();
	// TODO: should is_init use a mutex??
	this->is_init = true;
	return true;

malformed:
	LOG(this->log_delay, LEVEL_ERROR,
	    "Malformed sat delay configuration file '%s'\n",
	    filename);
	file.close();
error:
	return false;
}

bool FileIslDelay::updateIslDelay()
{
	unsigned int old_time, new_time;
	time_ms_t old_delay, new_delay, next_delay;

	this->current_time++;

	LOG(this->log_delay, LEVEL_INFO,
	    "Updating sat delay: current time: %u (step: %f ms)\n",
	    this->current_time, this->refresh_period);

	// Look for the next entry whose key is equal or greater than 'current_time'
	auto delay_it = this->delays.lower_bound(this->current_time);

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
			delay_it--;

			old_time = delay_it->first;
			old_delay = delay_it->second;

			LOG(this->log_delay, LEVEL_DEBUG,
			    "Old time: %u, old delay: %u\n",
			    old_time,
			    old_delay.count());

			// Linear interpolation
			auto coef = (new_delay - old_delay) / double(new_time - old_time);

			next_delay = old_delay + std::chrono::duration_cast<time_ms_t>(coef * (this->current_time - old_time));

			LOG(this->log_delay, LEVEL_DEBUG,
			    "Linear coef: %f, old step: %u\n",
			    coef.count(), old_time);
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
	    "new delay value: %u\n",
	    next_delay.count());

	this->setSatDelay(next_delay);

	return true;
}


static bool pair_compare(const std::pair<unsigned int, time_ms_t> &a,
                         const std::pair<unsigned int, time_ms_t> &b)
{
	return a.second < b.second;
}


bool FileIslDelay::getMaxDelay(time_ms_t &delay) const
{
	if(!this->is_init)
	{
		return false;
	}

	auto result = std::max_element(this->delays.begin(), this->delays.end(), pair_compare);
	delay = result->second;
	return true;
}

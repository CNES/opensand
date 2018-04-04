/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file File.h
 * @brief File
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef FILE_SATDELAY_PLUGIN_H
#define FILE_SATDELAY_PLUGIN_H


#include "PhysicalLayerPlugin.h"

#include <opensand_conf/conf.h>

#include <fstream>
#include <string>

using std::string;
using std::map;

/**
 * @class FileDelay
 * @brief FileDelay
 */
class FileDelay: public SatDelayPlugin
{
 private:

	bool is_init;

	/// The current time (in refresh_period_ms)
	unsigned int current_time;

	/// The satdelay values we will interpolate
	map<unsigned int, time_ms_t> delays;

	/// Reading mode
	bool loop;

	map<string, ConfigurationList> config_section_map;

	/**
	 * @brief Load the sat delay file
	 *
	 * @param filename  The sat delay file name
	 * @return true on success, false otherwise
	 */
	bool load(string filename);

 public:

	/**
	 * @brief Build a File
	 */
	FileDelay();

	/**
	 * @brief Destroy the File
	 */
	~FileDelay();

	bool init();

	bool updateSatDelay();

	bool getMaxDelay(time_ms_t &delay);
};

CREATE(FileDelay, satdelay_plugin, "FileDelay");

#endif

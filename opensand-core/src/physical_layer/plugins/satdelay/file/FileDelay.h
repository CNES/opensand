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
 * @file File.h
 * @brief File
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef FILE_SATDELAY_PLUGIN_H
#define FILE_SATDELAY_PLUGIN_H


#include "OpenSandCore.h"
#include "PhysicalLayerPlugin.h"

#include <string>
#include <map>


/**
 * @class FileDelay
 * @brief FileDelay
 */
class FileDelay: public SatDelayPlugin
{
private:
	static std::string config_path;

	bool is_init;

	/// The current time (in refresh_period_ms)
	unsigned int current_time;

	/// The satdelay values we will interpolate
	std::map<unsigned int, time_ms_t> delays;

	/// Reading mode
	bool loop;

	/**
	 * @brief Load the sat delay file
	 *
	 * @param filename  The sat delay file name
	 * @return true on success, false otherwise
	 */
	bool load(std::string filename);

public:
	/**
	 * @brief Build a File
	 */
	FileDelay();

	/**
	 * @brief Destroy the File
	 */
	~FileDelay();

	/**
	 * @brief Generate the configuration for the plugin
	 */
	static void generateConfiguration(const std::string &parent_path,
	                                  const std::string &param_id,
	                                  const std::string &plugin_name);

	bool init();

	bool updateSatDelay();

	bool getMaxDelay(time_ms_t &delay) const;
};


CREATE(FileDelay, PluginType::SatDelay, "FileDelay");


#endif

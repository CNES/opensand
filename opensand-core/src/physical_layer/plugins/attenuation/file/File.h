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
 * @file File.h
 * @brief File
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 */

#ifndef FILE_ATTENUATION_MODEL_H
#define FILE_ATTENUATION_MODEL_H


#include "PhysicalLayerPlugin.h"

#include <map>
#include <string>


/**
 * @class File
 * @brief File
 */
class File: public AttenuationModelPlugin
{
private:
	/// The current time (in refresh_period_ms)
	unsigned int current_time;

	/// The attenuation values we will interpolate
	std::map<unsigned int, double> attenuation;

	/// Reading mode
	bool loop;

	/**
	 * @brief Load the attenuation file
	 *
	 * @param filename  The attenuation file name
	 * @return true on success, false otherwise
	 */
	bool load(std::string filename);

public:
	/**
	 * @brief Build a File
	 */
	File();

	/**
	 * @brief Destroy the File
	 */
	~File();

	/**
	 * @brief Generate the configuration for the plugin
	 */
	static void generateConfiguration(const std::string &parent_path,
	                                  const std::string &param_id,
	                                  const std::string &plugin_name);

	bool init(time_ms_t refresh_period_ms, std::string link);

	bool updateAttenuationModel();
};


CREATE(File, PluginType::Attenuation, "File");


#endif

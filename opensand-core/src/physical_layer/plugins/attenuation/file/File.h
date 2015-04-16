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
 * @file File.h
 * @brief File
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 */

#ifndef FILE_ATTENUATION_MODEL_H
#define FILE_ATTENUATION_MODEL_H


#include "PhysicalLayerPlugin.h"

#include <opensand_conf/ConfigurationFile.h>

#include <fstream>
#include <string>

using std::string;
using std::map;

/**
 * @class File
 * @brief File
 */
class File: public AttenuationModelPlugin
{
 private:

	/// The current time
	unsigned int current_time;

	/// The attenuation values we will interpolate
	map<unsigned int, double> attenuation;

	/// Reading mode
	bool loop;

	map<string, ConfigurationList> config_section_map;

	/**
	 * @brief Load the attenuation file
	 *
	 * @param filename  The attenuation file name
	 * @return true on success, false otherwise
	 */
	bool load(string filename);

 public:

	/**
	 * @brief Build a File
	 */
	File();

	/**
	 * @brief Destroy the File
	 */
	~File();

	bool init(time_ms_t refresh_period_ms, string link);

	bool updateAttenuationModel();

};

CREATE(File, attenuation_plugin, "File");

#endif

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
 * @file Constant.h
 * @brief Set a constant satellite delay 
 * @author Joaquin MUGUERZA <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef CONSTANT_SATDELAY_PLUGIN_H
#define CONSTANT_SATDELAY_PLUGIN_H 


#include "DvbPlugin.h"
#include "OpenSandCore.h"
#include <opensand_conf/ConfigurationFile.h>

/**
 * @class Constant 
 */
class ConstantDelay: public SatDelayPlugin
{
	private:
		
		map<string,ConfigurationList> config_section_map;

		bool is_init;

	public:

		/**
		 * @brief Build the defaultMinimalCondition
		 */
		ConstantDelay();

		/**
		 * @brief Destroy the defaultMinimalCondition
		 */
		~ConstantDelay();

		/**
		 * @brief initialize the constant delay
		 *
		 * @return true on success, false otherwise
		 */
		bool init();

		/**
		 * @brief Updates the satellite delay
		 *
		 * @return true on success, false otherwise
		 */
		bool updateSatDelay();

		/**
		 * @brief Get largest satellite delay
		 *
		 * @return largest delay
		 */
		time_ms_t getMaxDelay();
};

CREATE(ConstantDelay, satdelay_plugin, "ConstantDelay");

#endif


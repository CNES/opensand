/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file Default.h
 * @brief Default load of nominal C/N for clear-sky condition from config files (core.conf)
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef DEFAULT_NOMINAL_CONDITION_PLUGIN_H
#define DEFAULT_NOMINAL_CONDITION_PLUGIN_H 

#include "PhysicalLayerPlugin.h"

/**
 * @class Default 
 * @brief Load the max CN from the config files (core.conf) 
 */
class Default: public NominalConditionPlugin
{

	public:

		/**
		 * @brief Build the defaultNominalCondition
		 */
		Default();

		/**
		 * @brief Destroy the defaultNominalCondition
		 */
		~Default();

		/**
		 * @brief initialize the nominal condition
		 *
		 * @param link the link
		 * @return true on success, false otherwise
		 */
		bool init(string link);
};

CREATE(Default, nominal_plugin, "Default");

#endif

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
 * @file Modcod.h
 * @brief Determine Minimal C/N depending of the current Modcod 
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef MODCOD_MINIMAL_CONDITION_PLUGIN_H
#define MODCOD_MINIMAL_CONDITION_PLUGIN_H 

#include "PhysicalLayerPlugin.h"
#include "ModcodDefinitionTable.h"


/**
 * @class Modcod 
 * @brief Load the max CN from the config files (core.conf) 
 */
class Modcod:public MinimalConditionPlugin
{
	private:
		ModcodDefinitionTable modcod_table;

	public:

		/**
		 * @brief Build the defaultMinimalCondition
		 */
		Modcod();

		/**
		 * @brief Destroy the defaultMinimalCondition
		 */
		~Modcod();

		/**
		 * @brief initialize the minimal condition
		 *
		 * @return true on success, false otherwise
		 */
		bool init();

		/**
		 * @brief Updates Thresold when a msg arrives to Channel
		 *        (when MODCOD mode: use BBFRAME modcod id) 
		 *
		 * @return true on success, false otherwise
		 */
		 bool updateThreshold(T_DVB_HDR *hdr);
};

CREATE(Modcod, minimal_plugin, "Modcod");

#endif

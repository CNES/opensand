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
 * @file AcmLoop.h
 * @brief Determine Minimal C/N depending of the current AcmLoop 
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef ACM_LOOP_MINIMAL_CONDITION_PLUGIN_H
#define ACM_LOOP_MINIMAL_CONDITION_PLUGIN_H 

#include "PhysicalLayerPlugin.h"
#include "FmtDefinitionTable.h"


/**
 * @class AcmLoop 
 * @brief Load the max CN from the config files (core.conf) 
 */
class AcmLoop:public MinimalConditionPlugin
{
	private:
		FmtDefinitionTable modcod_table_rcs;
		FmtDefinitionTable modcod_table_s2;

	public:

		/**
		 * @brief Build the defaultMinimalCondition
		 */
		AcmLoop();

		/**
		 * @brief Destroy the defaultMinimalCondition
		 */
		~AcmLoop();

		/**
		 * @brief initialize the minimal condition
		 *
		 * @return true on success, false otherwise
		 */
		bool init(void);

		/**
		 * @brief Updates Thresold when a msg arrives to Channel
		 *
		 * @param message_type  The frame type
		 * @return true on success, false otherwise
		 */
		bool updateThreshold(uint8_t modcod_id, uint8_t message_type);
};

CREATE(AcmLoop, minimal_plugin, "ACM-Loop");

#endif

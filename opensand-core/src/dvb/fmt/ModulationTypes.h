/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
 * @file ModulationTypes.h
 * @brief The modulation types
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef MODULATION_TYPES_H
#define MODULATION_TYPES_H

#include <string>
#include <map>

using std::string;
using std::map;

/**
 * @class ModulationTypes
 * @brief The modulation types
 */
class ModulationTypes
{
 public:
	~ModulationTypes();

	/**
	 * @brief Check a modulation exists
	 *
	 * @param modulation_label  The modulation label
	 *
	 * @return  True if the label is managed, false otherwise
	 */
	static bool exist(string modulation_label);

	/**
	 * @brief Get the default modulation efficiency
	 *
	 * @return  The default modulation effiency
	 */
	static unsigned int getDefaultEfficiency();

	/**
	 * @brief Get a modulation efficiency
	 *
	 * @param modulation_label  The modulation label
	 *
	 * @return  The modulation effiency
	 */
	static unsigned int getEfficiency(string modulation_label);

 private:
	static ModulationTypes instance;

	unsigned int default_modulation_efficiency;
	map<string, unsigned int> modulation_efficiencies;

	ModulationTypes();
};

#endif


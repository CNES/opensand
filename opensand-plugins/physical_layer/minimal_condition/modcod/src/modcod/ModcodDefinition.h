/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file ModcodDefinition.h
 * @brief The definition of a MODCOD
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef MODCOD_DEFINITION_H
#define MODCOD_DEFINITION_H

#include "ModulationType.h"
#include <string>


/**
 * @class ModcodDefinition
 * @brief The definition of a MODCOD
 */
class ModcodDefinition
{
 private:

	/** The ID of the MODCOD definition */
	unsigned int id;

	/** The type of modulation of the MODCOD definition */
	modulation_type_t modulation;

	/** The coding rate of the MODCOD definition */
	std::string coding_rate; /* TODO: make this an enum */

	/* The spectral efficiency of the MODCOD definition */
	float spectral_efficiency;

	/**
	 * @brief The required Es/N0 of the MODCOD definition
	 *
	 * Es/N0 is the energy per symbol per noise power spectral density.
	 */
	float required_Es_N0;

 public:

	/**** constructor/destructor ****/

	/* create a MODCOD definition */
	ModcodDefinition(unsigned int id,
	                 std::string modulation,
	                 std::string coding_rate,
	                 float spectral_efficiency,
	                 float required_Es_N0);

	/* destroy a MODCOD definition */
	~ModcodDefinition();


	/**** accessors ****/

	/* get the ID of the MODCOD definition */
	unsigned int getId();

	/* get the modulation of the MODCOD definition */
	modulation_type_t getModulation();

	/* get the coding rate of the MODCOD definition */
	std::string getCodingRate();

	/* get the spectral efficiency of the MODCOD definition */
	float getSpectralEfficiency();

	/* get the required Es/N0 ratio of the MODCOD definition */
	float getRequiredEsN0();
};

#endif

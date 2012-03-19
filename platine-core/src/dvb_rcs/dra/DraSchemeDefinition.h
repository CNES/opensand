/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file DraSchemeDefinition.h
 * @brief The definition of a DRA scheme
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DRA_SCHEME_DEFINITION_H
#define DRA_SCHEME_DEFINITION_H

#include "ModulationType.h"
#include <string>


/**
 * @class DraSchemeDefinition
 * @brief The definition of a DRA scheme
 */
class DraSchemeDefinition
{
 private:

	/** The ID of the DRA scheme definition */
	unsigned int id;

	/** The type of modulation of the DRA scheme definition */
	modulation_type_t modulation;

	/** The coding rate of the DRA scheme definition */
	std::string coding_rate; /* TODO: make this an enum */

	/* The spectral efficiency of the DRA scheme definition */
	float spectral_efficiency;

	/** The symbol rate of the DRA scheme definition */
	int symbol_rate;

	/** The bit rate of the DRA scheme definition */
	float bit_rate;

	/** The required carrier to noise ratio of the DRA scheme definition */
	float required_C_N0;

 public:

	/**** constructor/destructor ****/

	/* create a DRA scheme definition */
	DraSchemeDefinition(unsigned int id,
	                    std::string modulation,
	                    std::string coding_rate,
	                    float spectral_efficiency,
	                    unsigned int symbol_rate,
	                    float bit_rate,
	                    float required_C_N0);

	/* destroy a DRA scheme definition */
	~DraSchemeDefinition();


	/**** accessors ****/

	/* get the ID of the DRA scheme definition */
	unsigned int getId();

	/* get the modulation of the DRA scheme definition */
	modulation_type_t getModulation();

	/* get the coding rate of the DRA scheme definition */
	std::string getCodingRate();

	/* get the spectral efficiency of the DRA scheme definition */
	float getSpectralEfficiency();

	/* get the symbol rate of the DRA scheme definition */
	unsigned int getSymbolRate();

	/* get the bit rate of the DRA scheme definition */
	float getBitRate();

	/* get the required C/N0 ratio of the DRA scheme definition */
	float getRequiredCarrierToNoiseRatio();
};

#endif

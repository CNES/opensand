/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file FmtDefinition.h
 * @brief The definition of a FMT
 * @author Julien BERNARD / Viveris Technologies
 */

#ifndef FMT_DEFINITION_H
#define FMT_DEFINITION_H

#include "OpenSandCore.h"
#include "ModulationType.h"
#include <string>

using std::string;

/**
 * @class FmtDefinition
 * @brief The definition of a FMT
 */
class FmtDefinition
{
 protected:

	/** The ID of the FMT definition */
	unsigned int id;

	/** The type of modulation of the FMT definition */
	modulation_type_t modulation;

	/** The coding rate of the FMT definition */
	string coding_rate;

	/* The spectral efficiency of the FMT definition */
	float spectral_efficiency;

	/** The required carrier to noise ratio */
	float required_Es_N0;

 public:

	/**** constructor/destructor ****/

	/* create a FMT definition */
	FmtDefinition(const unsigned int id,
	              const string modulation,
	              const string coding_rate,
	              const float spectral_efficiency,
	              const float required_Es_N0);

	/* destroy a FMT definition */
	~FmtDefinition();


	/**** accessors ****/

	/* get the ID of the FMT definition */
	unsigned int getId() const;

	/* get the modulation of the FMT definition */
	modulation_type_t getModulation() const;

	/* get the coding rate of the FMT definition */
	string getCodingRate() const;

	/* get the spectral efficiency of the FMT definition */
	float getSpectralEfficiency() const;

	/* get the required Es/N0 ratio of the FMT definition */
	float getRequiredEsN0() const;

};

#endif

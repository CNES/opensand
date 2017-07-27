/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file FmtDefinition.h
 * @brief The definition of a FMT
 * @author Julien BERNARD / Viveris Technologies
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef FMT_DEFINITION_H
#define FMT_DEFINITION_H

#include "OpenSandCore.h"
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

	/** The modulation type of the FMT definition */
	string modulation_type;

	/** The modulation efficiency of the FMT definition */
	unsigned int modulation_efficiency;

	/** The inverse of the modulation efficiency of the FMT definition */
	float modulation_efficiency_inv;

	/** The coding type of the FMT definition */
	string coding_type;

	/** The coding rate of the FMT definition */
	float coding_rate;

	/** The inverse of the coding rate of the FMT definition */
	float coding_rate_inv;

	/* The spectral efficiency of the FMT definition */
	float spectral_efficiency;

	/** The required carrier to noise ratio */
	double required_Es_N0;

	/** The status about burst length presence */
	bool has_burst_length;

	/** The burst length in symbols */
	vol_sym_t burst_length_sym;

 public:
	/* create a FMT definition */
	FmtDefinition(const unsigned int id,
		const string modulation_type,
		const string coding_type,
		const float spectral_efficiency,
		const double required_Es_N0);

	FmtDefinition(const unsigned int id,
		const string modulation_type,
		const string coding_type,
		const float spectral_efficiency,
		const double required_Es_N0,
		const vol_sym_t burst_length);

	/* constructor by copy */
	FmtDefinition(const FmtDefinition &fmt_def);

	/* destroy a FMT definition */
	virtual ~FmtDefinition();

	/**** accessors ****/

	/* get the ID of the FMT definition */
	unsigned int getId() const;

	/* get the modulation type of the FMT definition */
	string getModulation() const;

	/* get the modulation efficiency of the FMT definition */
	unsigned int getModulationEfficiency() const;

	/* get the coding type of the FMT definition */
	string getCoding() const;

	/* get the coding rate of the FMT definition */
	float getCodingRate() const;

	/* get the spectral efficiency of the FMT definition */
	float getSpectralEfficiency() const;

	/* get the required Es/N0 ratio of the FMT definition */
	double getRequiredEsN0() const;

	/* get the status about burst length presence */
	bool hasBurstLength() const;

	/* get the burst length in symbols of the FMT defition */
	vol_sym_t getBurstLength() const;

	/* convert a value in symbol for the FMT definition */
	vol_kb_t symToKbits(vol_sym_t vol_sym) const;

	/* convert a value in kbits for the FMT definition */
	vol_sym_t kbitsToSym(vol_kb_t vol_kbits) const;

	/* add FEC to data length */
	unsigned int addFec(unsigned int length) const;

	/* remove FEC to data length */
	unsigned int removeFec(unsigned int length) const;

	virtual void print(void); /// For debug

};

#endif

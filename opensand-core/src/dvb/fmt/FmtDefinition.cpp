/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file FmtDefinition.cpp
 * @brief The definition of a FMT
 * @author Julien BERNARD / Viveris Technologies
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "FmtDefinition.h"
#include "ModulationTypes.h"
#include "CodingTypes.h"

#include <opensand_output/Output.h>

/**
 * @brief Create a FMT definition
 *
 * @param id                   the ID of the FMT
 * @param modulation_type      the type of modulation of the FMT
 * @param coding_type          the coding type of the FMT
 * @param spectral_efficiency  the spectral efficiency of the FMT
 * @param required_Es_N0       the required Es/N0 of the FMT
 * @param burst_length         the burst length in symbols of the FMT
 */
FmtDefinition::FmtDefinition(const unsigned int id,
                             const string modulation_type,
                             const string coding_type,
                             const float spectral_efficiency,
                             const double required_Es_N0,
                             const vol_sym_t burst_length):
	id(id),
	modulation_type(modulation_type),
	coding_type(coding_type),
	spectral_efficiency(spectral_efficiency),
	required_Es_N0(required_Es_N0),
	has_burst_length(true),
	burst_length_sym(burst_length)
{
	this->modulation_efficiency =
		ModulationTypes::getEfficiency(this->modulation_type);

	this->coding_rate =
		CodingTypes::getRate(this->coding_type);
}

/**
 * @brief Create a FMT definition
 *
 * @param id                   the ID of the FMT
 * @param modulation_type      the type of modulation of the FMT
 * @param coding_type          the coding type of the FMT
 * @param spectral_efficiency  the spectral efficiency of the FMT
 * @param required_Es_N0       the required Es/N0 of the FMT
 */
FmtDefinition::FmtDefinition(const unsigned int id,
                             const string modulation_type,
                             const string coding_type,
                             const float spectral_efficiency,
                             const double required_Es_N0):
	id(id),
	modulation_type(modulation_type),
	coding_type(coding_type),
	spectral_efficiency(spectral_efficiency),
	required_Es_N0(required_Es_N0),
	has_burst_length(false),
	burst_length_sym(0)
{
	this->modulation_efficiency =
		ModulationTypes::getEfficiency(this->modulation_type);

	this->coding_rate =
		CodingTypes::getRate(this->coding_type);
}

FmtDefinition::FmtDefinition(const FmtDefinition &fmt_def)
{
	this->id = fmt_def.id;
	this->modulation_type = fmt_def.modulation_type;
	this->modulation_efficiency = fmt_def.modulation_efficiency;
	this->coding_type = fmt_def.coding_type;
	this->coding_rate = fmt_def.coding_rate;
	this->spectral_efficiency = fmt_def.spectral_efficiency;
	this->required_Es_N0 = fmt_def.required_Es_N0;
	this->burst_length_sym = fmt_def.burst_length_sym;
}

/**
 * @brief Destroy a FMT definition
 */
FmtDefinition::~FmtDefinition()
{
	// nothing particular to do
}


/**
 * @brief Get the ID of the FMT definition
 *
 * @return  the ID of the FMT
 */
unsigned int FmtDefinition::getId() const
{
	return this->id;
}

/**
 * @brief Get the modulation type of the FMT definition
 *
 * @return  the type of modulation of the FMT
 */
string FmtDefinition::getModulation() const
{
	return this->modulation_type;
}

/**
 * @brief Get the modulation efficiency of the FMT definition
 *
 * @return  the efficiency of modulation of the FMT
 */
unsigned int FmtDefinition::getModulationEfficiency() const
{
	return this->modulation_efficiency;
}

/**
 * @brief Get the coding type of the FMT definition
 *
 * @return  the type of coding of the FMT
 */
string FmtDefinition::getCoding() const
{
	return this->coding_type;
}

/**
 * @brief Get the coding rate of the FMT definition
 *
 * @return  the rate of coding of the FMT
 */
float FmtDefinition::getCodingRate() const
{
	return this->coding_rate;
}

/**
 * @brief Get the spectral efficiency of the FMT definition
 *
 * @return  the spectral efficiency of the FMT
 */
float FmtDefinition::getSpectralEfficiency() const
{
	return this->spectral_efficiency;
}

/**
 * @brief Get the required Es/N0 ratio of the FMT definition
 *
 * @return  the required Es/N0 ratio of the FMT
 */
double FmtDefinition::getRequiredEsN0() const
{
	return this->required_Es_N0;
}

/**
 * @brief Get the status about burst length presence
 *
 * @return  the burst length presence status of the FMT
 */
bool FmtDefinition::hasBurstLength() const
{
	return this->has_burst_length;
}
/**
 * @brief Get the burst length of the FMT definition
 *
 * @return  the burst length in symbols of the FMT
 */
vol_sym_t FmtDefinition::getBurstLength() const
{
	return this->burst_length_sym;
}

/**
 * @brief Convert a value in symbol for the FMT definition
 *
 * @param val_sym  the value in symbols (per ...)
 * @return    the value converted in kbits (per ...)
 */
vol_sym_t FmtDefinition::kbitsToSym(vol_kb_t vol_kb) const
{
	return ceil(vol_kb * 1000
		* this->modulation_efficiency_inv
		* this->coding_rate_inv);
}

/**
 * @brief Convert a value in kbits for the FMT definition
 *
 * @param val_kbits  the value in kbits (per ...)
 * @return    the value converted in symbol (per ...)
 */
vol_kb_t FmtDefinition::symToKbits(vol_sym_t vol_sym) const
{
	return ceil(vol_sym / 1000
		* this->modulation_efficiency
		* this->coding_rate);
}

void FmtDefinition::print(void)
{
	if(this->has_burst_length)
	{
		DFLTLOG(LEVEL_ERROR, "id = %u, modulation = %s,"
		        " coding_rate = %s, spectral_efficiency = %f,"
		        " required_Es_N0 = %f, burst_length = %u sym\n", this->id,
		        this->modulation_type.c_str(), this->coding_type.c_str(),
		        this->spectral_efficiency, this->required_Es_N0,
		        this->burst_length_sym);
	}
	else
	{
		DFLTLOG(LEVEL_ERROR, "id = %u, modulation = %s,"
		        " coding_rate = %s, spectral_efficiency = %f,"
		        " required_Es_N0 = %f\n", this->id,
		        this->modulation_type.c_str(), this->coding_type.c_str(),
		        this->spectral_efficiency, this->required_Es_N0);
	}
}


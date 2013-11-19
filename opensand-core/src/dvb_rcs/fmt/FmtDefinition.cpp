/*
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
 * @file FmtDefinition.cpp
 * @brief The definition of a FMT
 * @author Julien BERNARD / Viveris Technologies
 */

#include "FmtDefinition.h"


/**
 * @brief Create a FMT definition
 *
 * @param id                   the ID of the FMT
 * @param modulation           the type of modulation of the FMT
 * @param coding_rate          the coding rate of the FMT
 * @param spectral_efficiency  the spectral efficiency of the FMT
 * @param required_Es_N0       the required Es/N0 of the FMT
 */
FmtDefinition::FmtDefinition(const unsigned int id,
                             const string modulation,
                             const string coding_rate,
                             const float spectral_efficiency,
                             const float required_Es_N0):
	id(id),
	coding_rate(coding_rate),
	spectral_efficiency(spectral_efficiency),
	required_Es_N0(required_Es_N0)
{
	if(modulation == "BPSK")
		this->modulation = MODULATION_BPSK;
	else if(modulation == "Pi/2BPSK")
		this->modulation = MODULATION_PI_2_BPSK;
	else if(modulation == "QPSK")
		this->modulation = MODULATION_QPSK;
	else if(modulation == "8PSK")
		this->modulation = MODULATION_8PSK;
	else if(modulation == "16APSK")
		this->modulation = MODULATION_16APSK;
	else if(modulation == "32APSK")
		this->modulation = MODULATION_32APSK;
	else
		this->modulation = MODULATION_UNKNOWN;
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
 * @brief Get the modulation of the FMT definition
 *
 * @return  the type of modulation of the FMT
 */
modulation_type_t FmtDefinition::getModulation() const
{
	return this->modulation;
}


/**
 * @brief Get the coding rate of the FMT definition
 *
 * @return  the coding rate of the FMT
 */
string FmtDefinition::getCodingRate() const
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
float FmtDefinition::getRequiredEsN0() const
{
	return this->required_Es_N0;
}


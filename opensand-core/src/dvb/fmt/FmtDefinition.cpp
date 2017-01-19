/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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

#include <opensand_output/Output.h>


/**
 * @brief Create a FMT definition
 *
 * @param id                   the ID of the FMT
 * @param modulation           the type of modulation of the FMT
 * @param coding_rate          the coding rate of the FMT
 * @param spectral_efficiency  the spectral efficiency of the FMT
 * @param required_Es_N0       the required Es/N0 of the FMT
 * @param burst_length         the burst length of the FMT
 */
FmtDefinition::FmtDefinition(const unsigned int id,
                             const string modulation,
                             const string coding_rate,
                             const float spectral_efficiency,
                             const double required_Es_N0,
                             const unsigned int burst_length):
	id(id),
	coding_rate(coding_rate),
	spectral_efficiency(spectral_efficiency),
	required_Es_N0(required_Es_N0),
	burst_length(burst_length)
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
	else if(modulation == "16QAM")
		this->modulation = MODULATION_16QAM;
	else if(modulation == "32APSK")
		this->modulation = MODULATION_32APSK;
	else
		this->modulation = MODULATION_UNKNOWN;
}

FmtDefinition::FmtDefinition(const FmtDefinition &fmt_def)
{
	this->id = fmt_def.getId();
	this->coding_rate = fmt_def.getCodingRate();
	this->spectral_efficiency = fmt_def.getSpectralEfficiency();
	this->required_Es_N0 = fmt_def.getRequiredEsN0();
	this->modulation = fmt_def.getModulation();
	this->burst_length = fmt_def.getBurstLength();
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
double FmtDefinition::getRequiredEsN0() const
{
	return this->required_Es_N0;
}


/**
 * @brief Get the burst length of the FMT definition
 *
 * @return  the burst length of the FMT
 */
unsigned int FmtDefinition::getBurstLength() const
{
	return this->burst_length;
}

void FmtDefinition::print(void)
{
	DFLTLOG(LEVEL_ERROR, "id = %u,"
	        " coding_rate = %s, spectral_efficiency = %f,"
	        " required_Es_N0 = %f, burst_length = %u\n", this->id,
	        this->coding_rate.c_str(), this->spectral_efficiency,
	        this->required_Es_N0, this->burst_length);
}


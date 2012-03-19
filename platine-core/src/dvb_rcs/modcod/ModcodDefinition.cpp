/**
 * @file ModcodDefinition.cpp
 * @brief The definition of a MODCOD
 * @author Didier Barvaux / Viveris Technologies
 */

#include "ModcodDefinition.h"


/**
 * @brief Create a MODCOD definition
 *
 * @param id                   the ID of the MODCOD
 * @param modulation           the type of modulation of the MODCOD
 * @param coding_rate          the coding rate of the MODCOD
 * @param spectral_efficiency  the spectral efficiency of the MODCOD
 * @param required_Es_N0       the required Es/N0 of the MODCOD
 */
ModcodDefinition::ModcodDefinition(unsigned int id,
                                   std::string modulation,
                                   std::string coding_rate,
                                   float spectral_efficiency,
                                   float required_Es_N0)
{
	this->id = id;

	if(modulation == "BPSK")
		this->modulation = MODULATION_BPSK;
	else if(modulation == "QPSK")
		this->modulation = MODULATION_QPSK;
	else if(modulation == "8PSK")
		this->modulation = MODULATION_8PSK;
	else
		this->modulation = MODULATION_UNKNOWN;

	this->coding_rate = coding_rate;
	this->spectral_efficiency = spectral_efficiency;
	this->required_Es_N0 = required_Es_N0;
}


/**
 * @brief Destroy a MODCOD definition
 */
ModcodDefinition::~ModcodDefinition()
{
	// nothing particular to do
}


/**
 * @brief Get the ID of the MODCOD definition
 *
 * @return  the ID of the MODCOD
 */
unsigned int ModcodDefinition::getId()
{
	return this->id;
}


/**
 * @brief Get the modulation of the MODCOD definition
 *
 * @return  the type of modulation of the MODCOD
 */
modulation_type_t ModcodDefinition::getModulation()
{
	return this->modulation;
}


/**
 * @brief Get the coding rate of the MODCOD definition
 *
 * @return  the coding rate of the MODCOD
 */
std::string ModcodDefinition::getCodingRate()
{
	return this->coding_rate;
}


/**
 * @brief Get the spectral efficiency of the MODCOD definition
 *
 * @return  the spectral efficiency of the MODCOD
 */
float ModcodDefinition::getSpectralEfficiency()
{
	return this->spectral_efficiency;
}


/**
 * @brief Get the required Es/N0 ratio of the MODCOD definition
 *
 * Es/N0 is the energy per symbol per noise power spectral density.
 *
 * @return  the required Es/N0 ratio of the MODCOD
 */
float ModcodDefinition::getRequiredEsN0()
{
	return this->required_Es_N0;
}


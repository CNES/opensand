/**
 * @file DraSchemeDefinition.cpp
 * @brief The definition of a DRA scheme
 * @author Didier Barvaux / Viveris Technologies
 */

#include "DraSchemeDefinition.h"


/**
 * @brief Create a DRA scheme definition
 *
 * @param id                   the ID of the DRA scheme
 * @param modulation           the type of modulation of the DRA scheme
 * @param coding_rate          the coding rate of the DRA scheme
 * @param spectral_efficiency  the spectral efficiency of the DRA scheme
 * @param symbol_rate          the symbol rate of the DRA scheme
 * @param bit_rate             the bit rate of the DRA scheme
 * @param required_C_N0        the required carrier to noise ratio
 *                             of the DRA scheme
 */
DraSchemeDefinition::DraSchemeDefinition(unsigned int id,
                                         std::string modulation,
                                         std::string coding_rate,
                                         float spectral_efficiency,
                                         unsigned int symbol_rate,
                                         float bit_rate,
                                         float required_C_N0)
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
	this->symbol_rate = symbol_rate;
	this->bit_rate = bit_rate;
	this->required_C_N0 = required_C_N0;
}


/**
 * @brief Destroy a DRA scheme definition
 */
DraSchemeDefinition::~DraSchemeDefinition()
{
	// nothing particular to do
}


/**
 * @brief Get the ID of the DRA scheme definition
 *
 * @return  the ID of the DRA scheme
 */
unsigned int DraSchemeDefinition::getId()
{
	return this->id;
}


/**
 * @brief Get the modulation of the DRA scheme definition
 *
 * @return  the type of modulation of the DRA scheme
 */
modulation_type_t DraSchemeDefinition::getModulation()
{
	return this->modulation;
}


/**
 * @brief Get the coding rate of the DRA scheme definition
 *
 * @return  the coding rate of the DRA scheme
 */
std::string DraSchemeDefinition::getCodingRate()
{
	return this->coding_rate;
}


/**
 * @brief Get the spectral efficiency of the DRA scheme definition
 *
 * @return  the spectral efficiency of the DRA scheme
 */
float DraSchemeDefinition::getSpectralEfficiency()
{
	return this->spectral_efficiency;
}


/**
 * @brief Get the symbol rate of the DRA scheme definition
 *
 * @return  the symbol rate of the DRA scheme
 */
unsigned int DraSchemeDefinition::getSymbolRate()
{
	return this->symbol_rate;
}


/**
 * @brief Get the bit rate of the DRA scheme definition
 *
 * @return  the bit rate of the DRA scheme
 */
float DraSchemeDefinition::getBitRate()
{
	return this->bit_rate;
}


/**
 * @brief Get the required C/N0 ratio of the DRA scheme definition
 *
 * @return  the required C/N0 ratio of the DRA scheme
 */
float DraSchemeDefinition::getRequiredCarrierToNoiseRatio()
{
	return this->required_C_N0;
}


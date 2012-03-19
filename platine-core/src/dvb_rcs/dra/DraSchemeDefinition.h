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


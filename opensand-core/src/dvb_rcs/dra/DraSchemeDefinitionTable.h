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
 * @file DraSchemeDefinitionTable.h
 * @brief The table of definitions of DRA schemes
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DRA_SCHEME_DEFINITION_TABLE_H
#define DRA_SCHEME_DEFINITION_TABLE_H

#include "DraSchemeDefinition.h"

#include <map>



typedef std::map<unsigned int, DraSchemeDefinition *>::iterator dra_def_table_pos_t;



/**
 * @class DraSchemeDefinitionTable
 * @brief The table of definitions of DRA schemes
 */
class DraSchemeDefinitionTable
{
 private:

	/** The internal map that stores all the DRA scheme definitions */
	std::map<unsigned int, DraSchemeDefinition *> definitions;

 public:

	/**** constructor/destructor ****/

	/* create a table of DRA scheme definitions */
	DraSchemeDefinitionTable();

	/* destroy a table of DRA scheme definitions */
	~DraSchemeDefinitionTable();


	/**** operations ****/

	/* load DRA scheme definition table from file */
	bool load(std::string filename);

	/* add a new DRA scheme definition in the table */
	bool add(unsigned int id,
	         std::string modulation,
	         std::string coding_rate,
	         float spectral_efficiency,
	         unsigned int symbol_rate,
	         float bit_rate,
	         float required_C_N0);

	/* does a DRA scheme definition with the given ID exist ? */
	bool do_exist(unsigned int id);

	/* clear the table of DRA scheme definitions */
	void clear();


	/**** iterators ****/

	/* init an iteration on all the DRA scheme definitions */
	dra_def_table_pos_t begin();

	/* get next DRA scheme definition */
	DraSchemeDefinition * next(dra_def_table_pos_t &pos);


	/**** accessors ****/

	/* get the modulation of the DRA scheme definition
	   whose ID is given as input */
	modulation_type_t getModulation(unsigned int id);

	/* get the coding rate of the DRA scheme definition
	   whose ID is given as input */
	std::string getCodingRate(unsigned int id);

	/* get the spectral efficiency of the DRA scheme definition
	   whose ID is given as input */
	float getSpectralEfficiency(unsigned int id);

	/* get the symbol rate of the DRA scheme definition
	   whose ID is given as input */
	unsigned int getSymbolRate(unsigned int id);

	/* get the bit rate of the DRA scheme definition
	   whose ID is given as input */
	float getBitRate(unsigned int id);

	/* get the required C/N0 ratio of the DRA scheme definition
	   whose ID is given as input */
	float getRequiredCarrierToNoiseRatio(unsigned int id);
};

#endif

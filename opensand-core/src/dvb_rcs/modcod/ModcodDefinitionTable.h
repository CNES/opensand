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
 * @file ModcodDefinitionTable.h
 * @brief The table of definitions of MODCODs
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef MODCOD_DEFINITION_TABLE_H
#define MODCOD_DEFINITION_TABLE_H

#include "ModcodDefinition.h"

#include <map>



typedef std::map<unsigned int, ModcodDefinition *>::iterator modcod_def_table_pos_t;



/**
 * @class ModcodDefinitionTable
 * @brief The table of definitions of MODCODs
 */
class ModcodDefinitionTable
{
 private:

	/** The internal map that stores all the MODCOD definitions */
	std::map<unsigned int, ModcodDefinition *> definitions;

	/** The number of definitions */
	unsigned int size;

 public:

	/**** constructor/destructor ****/

	/* create a table of MODCOD definitions */
	ModcodDefinitionTable();

	/* destroy a table of MODCOD definitions */
	~ModcodDefinitionTable();


	/**** operations ****/

	/* load MODCOD definition table from file */
	bool load(std::string filename);

	/* add a new MODCOD definition in the table */
	bool add(unsigned int id,
	         std::string modulation,
	         std::string coding_rate,
	         float spectral_efficiency,
	         float required_Es_N0);

	/* does a MODCOD definition with the given ID exist ? */
	bool do_exist(unsigned int id);

	/* get the number of MODCOD definitions in the table */
	unsigned int getSize();

	/* clear the table of MODCOD definitions */
	void clear();


	/**** iterators ****/

	/* init an iteration on all the MODCOD definitions */
	modcod_def_table_pos_t begin();

	/* get next MODCOD definition */
	ModcodDefinition * next(modcod_def_table_pos_t &pos);


	/**** accessors ****/

	/* get the modulation of the MODCOD definition
	   whose ID is given as input */
	modulation_type_t getModulation(unsigned int id);

	/* get the coding rate of the MODCOD definition
	   whose ID is given as input */
	std::string getCodingRate(unsigned int id);

	/* get the spectral efficiency of the MODCOD definition
	   whose ID is given as input */
	float getSpectralEfficiency(unsigned int id);

	/* get the required Es/N0 ratio of the MODCOD definition
	   whose ID is given as input */
	float getRequiredEsN0(unsigned int id);
};

#endif

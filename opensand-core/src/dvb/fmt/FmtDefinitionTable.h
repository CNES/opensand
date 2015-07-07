/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file FmtDefinitionTable.h
 * @brief The table of definitions of FMTs
 * @author Julien BERNARD / Viveris Technologies
 */

#ifndef FMT_DEFINITION_TABLE_H
#define FMT_DEFINITION_TABLE_H


#include "FmtDefinition.h"

#include <opensand_output/OutputLog.h>

#include <map>
#include <string>

using std::map;
using std::string;

typedef map<unsigned int, FmtDefinition *>::const_iterator fmt_def_table_pos_t;


/**
 * @class FmtDefinitionTable
 * @brief The table of definitions of FMTs
 */
class FmtDefinitionTable
{
 private:

	/** The internal map that stores all the FMT definitions */
	map<unsigned int, FmtDefinition *> definitions;

	/**
	 * @brief Get a FMT definition in the table
	 *
	 * @param id  The definition ID
	 * @return  the definition
	 */
	FmtDefinition *getFmtDef(unsigned int id) const;

 protected:

	// Output Log
	OutputLog *log_fmt;

 public:

	/**** constructor/destructor ****/

	/* create a table of FMT definitions */
	FmtDefinitionTable();

	/* destroy a table of FMT definitions */
	~FmtDefinitionTable();


	/**** operations ****/

	/**
	 * @brief Load FMT definition table from file
	 *
	 * @param filename  the name of the file to load FMT definitions from
	 * @return          true if definitions are successfully loaded, false otherwise
	 */
	bool load(const string filename);

	/**
	 * @brief Add a new FMT definition in the table
	 *
	 * @param id                   the ID of the FMT
	 * @param modulation           the type of modulation of the FMT
	 * @param coding_rate          the coding rate of the FMT
	 * @param spectral_efficiency  the spectral efficiency of the FMT
	 * @param required_Es_N0        the required carrier to noise ratio
	 *                             of the FMT
	 * @return                     true if the addition is successful, false otherwise
	 */
	bool add(const unsigned int id,
	         const string modulation,
	         const string coding_rate,
	         const float spectral_efficiency,
	         const float required_Es_N0);

	/**
	 * @brief Does a FMT definition with the given ID exist ?
	 *
	 * @param id  the ID we want to check for
	 * @return    true if a FMT exist, false is it does not exist
	 */
	bool doFmtIdExist(unsigned int id) const;

	/**
	 * @brief Clear the table of FMT definitions
	 */
	void clear();


	/**** accessors ****/

	/**
	 * @brief Get the modulation of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the type of modulation of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	modulation_type_t getModulation(unsigned int id) const;

	/**
	 * @brief Get the coding rate of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the coding rate of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	string getCodingRate(unsigned int id) const;

	/**
	 * @brief Get the spectral efficiency of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the spectral efficiency of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	float getSpectralEfficiency(unsigned int id) const;

	/**
	 * @brief Get the required Es/N0 ratio of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the required Es/N0 ratio of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	float getRequiredEsN0(unsigned int id) const;

	/**
	 * @brief Get the best required MODCOD according to the Es/N0 ratio
	 *        given as input
	 *
	 * @param id  the required Es/N0 ratio
	 * @return    the best required MODCOD ID, most robust if no MODCOD is found
	 */
	uint8_t getRequiredModcod(double cni) const;

	/**
	 * @brief  Get the highest definition ID
	 *
	 * @return the highest definition ID
	 */
	unsigned int getMaxId() const;

	/**
	 * @brief Convert a value in symbol for the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id       the ID of the FMT definition we want information for
	 * @param val_sym  the value in symbols (per ...)
	 * @return    the value converted in kbits (per ...)
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	unsigned int symToKbits(unsigned int id,
	                        unsigned int val_sym) const;


	void print(void); /// For debug
};

#endif

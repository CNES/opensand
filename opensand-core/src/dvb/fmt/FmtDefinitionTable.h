/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef FMT_DEFINITION_TABLE_H
#define FMT_DEFINITION_TABLE_H


#include "FmtDefinition.h"

#include <opensand_output/OutputLog.h>

#include <map>


/**
 * @class FmtDefinitionTable
 * @brief The table of definitions of FMTs
 */
class FmtDefinitionTable
{
private:
	/** The internal map that stores all the FMT definitions */
	std::map<fmt_id_t, std::unique_ptr<FmtDefinition>> definitions;

protected:
	// Output Log
	std::shared_ptr<OutputLog> log_fmt;

public:
	/**** constructor/destructor ****/

	/* create a table of FMT definitions */
	FmtDefinitionTable();

	/* destroy a table of FMT definitions */
	~FmtDefinitionTable();

	/**** operations ****/

	/**
	 * @brief Add a new FMT definition in the table
	 *
	 * @param fmt_def  the new FMT
	 * @return         true if the addition is successful, false otherwise
	 */
	bool add(std::unique_ptr<FmtDefinition> fmt_def);

	/**
	 * @brief Does a FMT definition with the given ID exist ?
	 *
	 * @param id  the ID we want to check for
	 * @return    true if a FMT exist, false is it does not exist
	 */
	bool doFmtIdExist(fmt_id_t id) const;

	/**
	 * @brief Clear the table of FMT definitions
	 */
	void clear();

	/**** accessors ****/

	/**
	 * @brief Get the definitions
	 *
	 * @return    definitions
	 */
	//std::map<fmt_id_t, std::unique_ptr<FmtDefinition>> getDefinitions() const;

	/**
	 * @brief Get a FMT definition in the table
	 *
	 * @param id  The definition ID
	 * @return  the definition
	 */
	FmtDefinition &getDefinition(fmt_id_t id) const;

	/**
	 * @brief Get the modulation efficiency of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the modulation efficiency of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	unsigned int getModulationEfficiency(fmt_id_t id) const;

	/**
	 * @brief Get the coding rate of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the coding rate of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	float getCodingRate(fmt_id_t id) const;

	/**
	 * @brief Get the spectral efficiency of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the spectral efficiency of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	float getSpectralEfficiency(fmt_id_t id) const;

	/**
	 * @brief Get the required Es/N0 ratio of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the required Es/N0 ratio of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	double getRequiredEsN0(fmt_id_t id) const;

	/**
	 * @brief Get the burst length presence status of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the burst length presence status of the FMT
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	bool hasBurstLength(fmt_id_t id) const;

	/**
	 * @brief Get the burst length of the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id  the ID of the FMT definition we want information for
	 * @return    the burst length of the FMT in symbols
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	vol_sym_t getBurstLength(fmt_id_t id) const;
	
	/**
	 * @brief Get the best required MODCOD according to the Es/N0 ratio
	 *        given as input
	 *
	 * @param id  the required Es/N0 ratio
	 * @return    the best required MODCOD ID, most robust if no MODCOD is found
	 */
	fmt_id_t getRequiredModcod(double cni) const;

	/**
	 * @brief  Get the lowest definition ID
	 *
	 * @return the lowest definition ID
	 */
	fmt_id_t getMinId() const;

	/**
	 * @brief  Get the highest definition ID
	 *
	 * @return the highest definition ID
	 */
	fmt_id_t getMaxId() const;

	/**
	 * @brief Convert a value in symbol for the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id       the ID of the FMT definition we want information for
	 * @param vol_sym  the value in symbols (per ...)
	 * @return    the value converted in kbits (per ...)
	 *
	 * @warning Be sure that the ID is valid before calling the function
	 */
	vol_kb_t symToKbits(fmt_id_t id,
	                    vol_sym_t vol_sym) const;

	/**
	 * @brief Convert a value in kbits for the FMT definition
	 *        whose ID is given as input
	 *
	 * @param id       the ID of the FMT definition we want information for
	 * @param vol_kb   the value in kbits (per ...)
	 * @return    the value converted in symbol (per ...)
	 *
	 * @warning Be sure that the ID is valid before calling the function
	 */
	vol_sym_t kbitsToSym(fmt_id_t id,
	                     vol_kb_t vol_kb) const;

	void print() const; /// For debug
};

#endif

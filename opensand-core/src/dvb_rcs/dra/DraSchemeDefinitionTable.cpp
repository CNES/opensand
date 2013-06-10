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
 * @file DraSchemeDefinitionTable.cpp
 * @brief The table of definitions of DRA schemes
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "DraSchemeDefinitionTable.h"

#include <fstream>
#include <sstream>

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include "opensand_conf/uti_debug.h"


/**
 * @brief Create a table of DRA scheme definitions
 */
DraSchemeDefinitionTable::DraSchemeDefinitionTable():
	definitions()
{
}


/**
 * @brief Destroy a table of DRA scheme definitions
 */
DraSchemeDefinitionTable::~DraSchemeDefinitionTable()
{
	this->clear();
}


/**
 * @brief Load DRA scheme definition table from file
 *
 * @param filename  the name of the file to load DRA scheme definitions from
 * @return          true if definitions are successfully loaded, false otherwise
 *
 * @todo TODO: merge code with \ref ModcodDefinitionTable::load
 */
bool DraSchemeDefinitionTable::load(std::string filename)
{
	std::ifstream file;
	unsigned int lines_count;
	int nb_dra_schemes;
	bool is_nb_dra_schemes_found = false;
	unsigned int nb_dra_schemes_read = 0;

	// first, clear all the current DRA scheme definitions
	this->clear();

	// open the DRA scheme definition file
	file.open(filename.c_str());
	if(!file.is_open())
	{
		UTI_ERROR("failed to open the DRA scheme definition file '%s'\n",
		          filename.c_str());
		goto error;
	}

	// read every line of the file
	lines_count = 0;
	while(!file.eof())
	{
		std::string line;
		std::stringstream line_stream;
		std::string token;

		// get the full line
		std::getline(file, line);

		// skip line if empty
		if(line == "")
		{
			continue;
		}

		lines_count++;

		// get the first keyword of the line
		line_stream.str(line);
		line_stream >> token;
		if(token == "/*")
		{
			// the line starts with a comment, skip it
		}
		else if(token == "nb_dra_schemes")
		{
			// the line starts with the 'nb_dra_schemes' keyword
			if(is_nb_dra_schemes_found)
			{
				// this is not the first line that starts with
				// the 'nb_dra_schemes' keyword
				UTI_ERROR("bad syntax at line %u: "
				          "multiple lines starting "
				          "with the 'nb_dra_schemes' keyword\n",
				          lines_count);
				goto malformed;
			}
			else
			{
				// this is the first line that starts with
				// the 'nb_dra_schemes' keyword
				std::string equal;

				// get the equal symbol and the number of DRA schemes
				line_stream >> equal >> nb_dra_schemes;

				// some checks on read values
				if(equal != "=")
				{
					UTI_ERROR("bad syntax at line %u: "
					          "the 'nb_dra_schemes' "
					          "keyword should be followed by an "
					          "equal symbol\n", lines_count);
					goto malformed;
				}
				if(nb_dra_schemes <= 0 || nb_dra_schemes > 100)
				{
					UTI_ERROR("bad syntax at line %u: "
					          "the number of DRA schemes "
					          "should be a non-zero positive "
					          "value under 100\n", lines_count);
					goto malformed;
				}

				// line format is valid
				UTI_INFO("%d DRA schemes present in definition "
				         "file\n", nb_dra_schemes);
				is_nb_dra_schemes_found = true;
				nb_dra_schemes_read = 0;
			}
		}
		else
		{
			// the first keyword should be a positive integer
			std::stringstream token_stream;
			unsigned int scheme_number;
			std::string modulation;
			std::string coding_rate;
			float spectral_efficiency;
			unsigned int symbol_rate;
			float bit_rate;
			float required_c_n0;
			int ret;

			// convert the string token to integer
			token_stream.str(token);
			token_stream >> scheme_number;
			if(scheme_number <= 0 || scheme_number > 100)
			{
				UTI_ERROR("bad syntax at line %u: DRA scheme "
				          "definition should start with a "
				          "non-zero positive integer up to 100\n",
				          lines_count);
				goto malformed;
			}

			// check if the number of DRA schemes was found
			if(!is_nb_dra_schemes_found)
			{
				// the 'nb_dra_schemes' keyword should be
				// specified before any DRA schemes line
				UTI_ERROR("bad syntax at line %u: DRA scheme "
				          "definition before the 'nb_dra_schemes' "
				          "keyword\n", lines_count);
				goto malformed;
			}

			// one more DRA scheme found
			nb_dra_schemes_read++;
			if(nb_dra_schemes_read > ((unsigned int) nb_dra_schemes))
			{
				UTI_ERROR("bad syntax at line %u: %u or more "
				          "DRA schemes definitions found, but "
				          "only %d specified with the "
				          "'nb_dra_schemes' keyword\n",
				          lines_count, nb_dra_schemes_read,
				          nb_dra_schemes);
				goto malformed;
			}

			// get all the parameters of the DRA scheme
			line_stream >> modulation
			            >> coding_rate
			            >> spectral_efficiency
			            >> symbol_rate
			            >> bit_rate
			            >> required_c_n0;

			// DRA scheme definition is OK, record it in the table
			ret = this->add(scheme_number, modulation, coding_rate,
			                spectral_efficiency, symbol_rate,
			                bit_rate, required_c_n0);
			if(ret != true)
			{
				UTI_ERROR("failed to add new DRA scheme definition: "
				          "%d, %s, %s, %f, %d, %f, %f\n",
					  scheme_number, modulation.c_str(),
				          coding_rate.c_str(),
				          spectral_efficiency, symbol_rate,
				          bit_rate, required_c_n0);
				goto malformed;
			}

			UTI_INFO("DRA scheme definition: %d, %s, %s, %f, %d, %f, %f\n",
				  scheme_number, modulation.c_str(),
				  coding_rate.c_str(), spectral_efficiency,
				  symbol_rate, bit_rate, required_c_n0);
		}
	}

	// check the number of DRA schemes read in definition file
	if(nb_dra_schemes_read != ((unsigned int) nb_dra_schemes))
	{
		UTI_ERROR("too few DRA schemes definitions: "
		          "%u found while %d specified with the "
		          "'nb_dra_schemes' keyword\n",
		          nb_dra_schemes_read, nb_dra_schemes);
		goto malformed;
	}
	UTI_INFO("%d DRA schemes found in definition file\n",
	         nb_dra_schemes);

	// close the definition file
	file.close();

	return true;

malformed:
	UTI_ERROR("malformed DRA scheme definition file\n");
	file.close();
error:
	return false;
}


/**
 * @brief Add a new DRA scheme definition in the table
 *
 * @param id                   the ID of the DRA scheme
 * @param modulation           the type of modulation of the DRA scheme
 * @param coding_rate          the coding rate of the DRA scheme
 * @param spectral_efficiency  the spectral efficiency of the DRA scheme
 * @param symbol_rate          the symbol rate of the DRA scheme
 * @param bit_rate             the bit rate of the DRA scheme
 * @param required_C_N0        the required carrier to noise ratio
 *                             of the DRA scheme
 * @return                     true if the addition is successful, false otherwise
 */
bool DraSchemeDefinitionTable::add(unsigned int id,
                                   std::string modulation,
                                   std::string coding_rate,
                                   float spectral_efficiency,
                                   unsigned int symbol_rate,
                                   float bit_rate,
                                   float required_C_N0)
{
	std::map<unsigned int, DraSchemeDefinition *>::iterator it;
	DraSchemeDefinition *new_def;

	// check that the table does not already own a DRA scheme definition
	// with the same identifier
	if(this->do_exist(id))
	{
		return false;
	}

	// create the new DRA scheme definition
	new_def = new DraSchemeDefinition(id, modulation, coding_rate,
	                                  spectral_efficiency, symbol_rate,
	                                  bit_rate, required_C_N0);
	if(new_def == NULL)
	{
		return false;
	}

	this->definitions[id] = new_def;

	return true;
}


/**
 * @brief Does a DRA scheme definition with the given ID exist ?
 *
 * @param id  the ID we want to check for
 * @return    true if a DRA scheme exist, false is it does not exist
 */
bool DraSchemeDefinitionTable::do_exist(unsigned int id)
{
	return (this->definitions.find(id) != this->definitions.end());
}


/**
 * @brief Clear the table of DRA scheme definitions
 */
void DraSchemeDefinitionTable::clear()
{
	std::map<unsigned int, DraSchemeDefinition *>::iterator it;

	// delete all stored DRA scheme definitions
	for(it = this->definitions.begin(); it != this->definitions.end(); it++)
	{
		delete it->second;
	}

	// now clear the map itself
	this->definitions.clear();
}


/**
 * @brief Init an iteration on all the DRA scheme definitions
 */
dra_def_table_pos_t DraSchemeDefinitionTable::begin()
{
	return this->definitions.begin();
}


/**
 * @brief Get next DRA scheme definition
 *
 * @param pos  the position to start from, will be updated for next call
 * @return     the DRA scheme definition if found, NULL otherwise
 */
DraSchemeDefinition * DraSchemeDefinitionTable::next(dra_def_table_pos_t &pos)
{
	DraSchemeDefinition *current;

	if(pos != this->definitions.end())
	{
		current = pos->second;
		pos++;
	}
	else
	{
		current = NULL;
	}

	return current;
}


/**
 * @brief Get the modulation of the DRA scheme definition
 *        whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the type of modulation of the DRA scheme
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
modulation_type_t DraSchemeDefinitionTable::getModulation(unsigned int id)
{
	return this->definitions[id]->getModulation();
}


/**
 * @brief Get the coding rate of the DRA scheme definition
 *        whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the coding rate of the DRA scheme
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
std::string DraSchemeDefinitionTable::getCodingRate(unsigned int id)
{
	return this->definitions[id]->getCodingRate();
}


/**
 * @brief Get the spectral efficiency of the DRA scheme definition
 *        whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the spectral efficiency of the DRA scheme
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
float DraSchemeDefinitionTable::getSpectralEfficiency(unsigned int id)
{
	return this->definitions[id]->getSpectralEfficiency();
}


/**
 * @brief Get the symbol rate of the DRA scheme definition
 *        whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the symbol rate of the DRA scheme
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
unsigned int DraSchemeDefinitionTable::getSymbolRate(unsigned int id)
{
	return this->definitions[id]->getSymbolRate();
}


/**
 * @brief Get the bit rate of the DRA scheme definition
 *        whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the bit rate of the DRA scheme
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
float DraSchemeDefinitionTable::getBitRate(unsigned int id)
{
	return this->definitions[id]->getBitRate();
}


/**
 * @brief Get the required C/N0 ratio of the DRA scheme definition
 *        whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the required C/N0 ratio of the DRA scheme
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
float DraSchemeDefinitionTable::getRequiredCarrierToNoiseRatio(unsigned int id)
{
	return this->definitions[id]->getRequiredCarrierToNoiseRatio();
}

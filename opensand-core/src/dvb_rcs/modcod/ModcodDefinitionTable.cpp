/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file ModcodDefinitionTable.cpp
 * @brief The table of definitions of MODCODs
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "ModcodDefinitionTable.h"

#include <fstream>
#include <sstream>

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include "opensand_conf/uti_debug.h"


/**
 * @brief Create a table of MODCOD definitions
 */
ModcodDefinitionTable::ModcodDefinitionTable():
	definitions()
{
	this->size = 0;
}


/**
 * @brief Destroy a table of MODCOD definitions
 */
ModcodDefinitionTable::~ModcodDefinitionTable()
{
	this->clear();
}


/**
 * @brief Load MODCOD definition table from file
 *
 * @param filename  the name of the file to load MODCOD definitions from
 * @return          true if definitions are successfully loaded, false otherwise
 *
 * @todo TODO: merge code with \ref DraSchemeDefinitionTable::load
 */
bool ModcodDefinitionTable::load(std::string filename)
{
	std::ifstream file;
	unsigned int lines_count;
	int nb_modcod;
	bool is_nb_modcod_found = false;
	unsigned int nb_modcod_read = 0;

	// first, clear all the current MODCOD definitions
	this->clear();

	// open the MODCOD definition file
	file.open(filename.c_str());
	if(!file.is_open())
	{
		UTI_ERROR("failed to open the MODCOD definition file '%s'\n",
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
		else if(token == "nb_modcod")
		{
			// the line starts with the 'nb_modcod' keyword
			if(is_nb_modcod_found)
			{
				// this is not the first line that starts with
				// the 'nb_modcod' keyword
				UTI_ERROR("bad syntax at line %u: "
				          "multiple lines starting "
				          "with the 'nb_modcod' keyword\n",
				          lines_count);
				goto malformed;
			}
			else
			{
				// this is the first line that starts with
				// the 'nb_modcod' keyword
				std::string equal;

				// get the equal symbol and the number of MODCODs
				line_stream >> equal >> nb_modcod;

				// some checks on read values
				if(equal != "=")
				{
					UTI_ERROR("bad syntax at line %u: "
					          "the 'nb_modcod' "
					          "keyword should be followed by an "
						  "equal symbol\n", lines_count);
					goto malformed;
				}
				if(nb_modcod <= 0 || nb_modcod > 100)
				{
					UTI_ERROR("bad syntax at line %u: "
					          "the number of MODCODs "
					          "should be a non-zero positive "
					          "value under 100\n", lines_count);
					goto malformed;
				}

				// line format is valid
				UTI_INFO("%d MODCODs present in definition "
				         "file\n", nb_modcod);
				is_nb_modcod_found = true;
				nb_modcod_read = 0;
			}
		}
		else
		{
			// the first keyword should be a positive integer
			std::stringstream token_stream;
			unsigned int def_number;
			std::string modulation;
			std::string coding_rate;
			float spectral_efficiency;
			float required_es_n0;
			int ret;

			// convert the string token to integer
			token_stream.str(token);
			token_stream >> def_number;
			if(def_number <= 0 || def_number > 100)
			{
				UTI_ERROR("bad syntax at line %u: MODCOD "
				          "definition should start with a "
				          "non-zero positive integer up to 100\n",
				          lines_count);
				goto malformed;
			}

			// check if the number of MODCODs was found
			if(!is_nb_modcod_found)
			{
				// the 'nb_modcod' keyword should be
				// specified before any MODCODs line
				UTI_ERROR("bad syntax at line %u: MODCOD "
				          "definition before the 'nb_modcod' "
				          "keyword\n", lines_count);
				goto malformed;
			}

			// one more MODCOD found
			nb_modcod_read++;
			if(nb_modcod_read > ((unsigned int) nb_modcod))
			{
				UTI_ERROR("bad syntax at line %u: %u or more "
				          "MODCODs definitions found, but "
				          "only %d specified with the "
				          "'nb_modcod' keyword\n",
				          lines_count, nb_modcod_read,
				          nb_modcod);
				goto malformed;
			}

			// get all the parameters of the MODCOD
			line_stream >> modulation
			            >> coding_rate
			            >> spectral_efficiency
			            >> required_es_n0;

			// MODCOD definition is OK, record it in the table
			ret = this->add(def_number, modulation, coding_rate,
			                spectral_efficiency, required_es_n0);
			if(ret != true)
			{
				UTI_ERROR("failed to add new MODCOD definition: "
				          "%d, %s, %s, %f, %f\n",
				          def_number, modulation.c_str(),
				          coding_rate.c_str(),
				          spectral_efficiency, required_es_n0);
				goto malformed;
			}

			UTI_INFO("MODCOD definition: %d, %s, %s, %f, %f\n",
			         def_number, modulation.c_str(),
			         coding_rate.c_str(), spectral_efficiency,
			         required_es_n0);
		}
	}

	// check the number of MODCODs read in definition file
	if(nb_modcod_read != ((unsigned int) nb_modcod))
	{
		UTI_ERROR("too few MODCODs definitions: "
		          "%u found while %d specified with the "
		          "'nb_modcod' keyword\n",
		          nb_modcod_read, nb_modcod);
		goto malformed;
	}
	UTI_INFO("%d MODCODs found in definition file\n", nb_modcod);

	// record the number of MODCOD definitions loaded from file
	this->size = nb_modcod;

	// close the definition file
	file.close();

	return true;

malformed:
	UTI_ERROR("malformed MODCOD definition file\n");
	file.close();
error:
	return false;
}


/**
 * @brief Add a new MODCOD definition in the table
 *
 * @param id                   the ID of the MODCOD
 * @param modulation           the type of modulation of the MODCOD
 * @param coding_rate          the coding rate of the MODCOD
 * @param spectral_efficiency  the spectral efficiency of the MODCOD
 * @param required_Es_N0       the required Es/N0 of the MODCOD
 * @return                     true if the addition is successful, false otherwise
 */
bool ModcodDefinitionTable::add(unsigned int id,
                                std::string modulation,
                                std::string coding_rate,
                                float spectral_efficiency,
                                float required_Es_N0)
{
	std::map<unsigned int, ModcodDefinition *>::iterator it;
	ModcodDefinition *new_def;

	// check that the table does not already own a MODCOD definition
	// with the same identifier
	if(this->do_exist(id))
	{
		return false;
	}

	// create the new MODCOD definition
	new_def = new ModcodDefinition(id, modulation, coding_rate,
	                               spectral_efficiency, required_Es_N0);
	if(new_def == NULL)
	{
		return false;
	}

	this->definitions[id] = new_def;

	return true;
}


/**
 * @brief Does a MODCOD definition with the given ID exist ?
 *
 * @param id  the ID we want to check for
 * @return    true if a MODCOD exist, false is it does not exist
 */
bool ModcodDefinitionTable::do_exist(unsigned int id)
{
	return (this->definitions.find(id) != this->definitions.end());
}


/**
 * @brief Get the number of MODCOD definitions in the table
 *
 * @return  the number of MODCODs definitions in the table
 */
unsigned int ModcodDefinitionTable::getSize()
{
	return this->size;
}


/**
 * @brief Clear the table of MODCOD definitions
 */
void ModcodDefinitionTable::clear()
{
	std::map<unsigned int, ModcodDefinition *>::iterator it;

	// delete all stored MODCOD definitions
	for(it = this->definitions.begin(); it != this->definitions.end(); it++)
	{
		delete it->second;
	}

	// now clear the map itself
	this->definitions.clear();
}


/**
 * @brief Init an iteration on all the MODCOD definitions
 */
modcod_def_table_pos_t ModcodDefinitionTable::begin()
{
	return this->definitions.begin();
}


/**
 * @brief Get next MODCOD definition
 *
 * @param pos  the position to start from, will be updated for next call
 * @return     the MODCOD definition if found, NULL otherwise
 */
ModcodDefinition * ModcodDefinitionTable::next(modcod_def_table_pos_t &pos)
{
	ModcodDefinition *current;

	if(pos != this->definitions.end())
	{
		current = pos->second;
		pos++;
	}
	else
	{
		current = NULL;
	}

	return NULL;
}



/**
 * @brief Get the modulation of the MODCOD definition
 *        whose ID is given as input
 *
 * @param id  the ID of the MODCOD definition we want information for
 * @return    the type of modulation of the MODCOD
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
modulation_type_t ModcodDefinitionTable::getModulation(unsigned int id)
{
	return this->definitions[id]->getModulation();
}


/**
 * @brief Get the coding rate of the MODCOD definition
 *        whose ID is given as input
 *
 * @param id  the ID of the MODCOD definition we want information for
 * @return    the coding rate of the MODCOD
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
std::string ModcodDefinitionTable::getCodingRate(unsigned int id)
{
	return this->definitions[id]->getCodingRate();
}


/**
 * @brief Get the spectral efficiency of the MODCOD definition
 *        whose ID is given as input
 *
 * @param id  the ID of the MODCOD definition we want information for
 * @return    the spectral efficiency of the MODCOD
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
float ModcodDefinitionTable::getSpectralEfficiency(unsigned int id)
{
	return this->definitions[id]->getSpectralEfficiency();
}


/**
 * @brief Get the required Es/N0 ratio of the MODCOD definition
 *        whose ID is given as input
 *
 * @param id  the ID of the MODCOD definition we want information for
 * @return    the required Es/N0 ratio of the MODCOD
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
float ModcodDefinitionTable::getRequiredEsN0(unsigned int id)
{
	return this->definitions[id]->getRequiredEsN0();
}

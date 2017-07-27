/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file FmtDefinitionTable.cpp
 * @brief The table of definitions of FMTs
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "FmtDefinitionTable.h"
#include "ModulationTypes.h"
#include "CodingTypes.h"

#include <opensand_output/Output.h>

#include <fstream>
#include <sstream>

using std::stringstream;

/// The maximum entries number in FMT definitions table
#define MAX_FMT 32


// Returns false if the string contains any non-whitespace characters
inline bool isSpace(string str)
{
	string::iterator it = str.begin();
	while(it != str.end())
	{
		if(!std::isspace(*it))
		{
			return false;
		}
		++it;
	}
	return true;
}

/**
 * @brief Create a table of FMT definitions
 */
FmtDefinitionTable::FmtDefinitionTable():
	definitions()
{
	// Output Log
	this->log_fmt = Output::registerLog(LEVEL_WARNING,
	                                    "Dvb.Fmt.DefinitionTable");
}


/**
 * @brief Destroy a table of FMT definitions
 */
FmtDefinitionTable::~FmtDefinitionTable()
{
	this->clear();
}


bool FmtDefinitionTable::load(const string filename, vol_sym_t req_burst_length)
{
	std::ifstream file;
	unsigned int lines_count = 0;
	int nb_fmt;
	bool is_nb_fmt_found = false;
	unsigned int nb_fmt_read = 0;
	unsigned int nb_req_fmt_read = 0;
	string line;
	std::istringstream line_stream;

	// first, clear all the current FMT definitions
	this->clear();

	// open the FMT definition file
	file.open(filename.c_str());
	if(!file.is_open())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to open the FMT definition file '%s'\n",
		    filename.c_str());
		goto error;
	}

	// read every line of the file
	while(std::getline(file, line))
	{
		string token;
		lines_count++;

		if(line == "")
		{
			continue;
		}

		// clear previous flags, if any
		line_stream.clear();
		line_stream.str(line);

		// get first keyword
		line_stream >> token;
		if((token.length() > 0 && token[0] == '#') ||
		   (token.length() > 0 && token[0] == '/' && token[1] == '*'))
		{
			continue;
		}
		else if(token == "nb_fmt")
		{
			// the line starts with the 'nb_fmt' keyword
			if(is_nb_fmt_found)
			{
				// this is not the first line that starts with
				// the 'nb_fmt' keyword
				LOG(this->log_fmt, LEVEL_ERROR,
				    "bad syntax at line %u (%s): "
				    "multiple lines starting "
				    "with the 'nb_fmt' keyword\n",
				    lines_count, line.substr(0,16).c_str());
				goto malformed;
			}
			else
			{
				// this is the first line that starts with
				// the 'nb_fmt' keyword
				string equal;

				// get the equal symbol and the number of FMTs
				line_stream >> equal >> nb_fmt;

				// some checks on read values
				if(equal != "=")
				{
					LOG(this->log_fmt, LEVEL_ERROR,
					    "bad syntax at line %u (%s): the 'nb_fmt'"
					    " keyword should be followed by an equal "
					    "symbol\n", lines_count,
					    line.substr(0,16).c_str());
					goto malformed;
				}
				if(nb_fmt <= 0 || nb_fmt > MAX_FMT)
				{
					LOG(this->log_fmt, LEVEL_ERROR,
					    "bad syntax at line %u (%s): "
					    "the number of FMTs should be a non-zero "
					    "positive value under %d\n", lines_count,
					    line.substr(0,16).c_str(), MAX_FMT);
					goto malformed;
				}

				// line format is valid
				LOG(this->log_fmt, LEVEL_NOTICE,
				    "%d FMTs present in definition "
				    "file\n", nb_fmt);
				is_nb_fmt_found = true;
				nb_fmt_read = 0;
				nb_req_fmt_read = 0;
			}
		}
		else
		{
			// the first keyword should be a positive integer
			stringstream token_stream;
			unsigned int scheme_number;
			string modulation_type;
			string coding_type;
			float spectral_efficiency;
			double required_es_n0;
			vol_sym_t burst_length;
			bool burst_length_found = false;
			int ret;
			FmtDefinition *fmt_def = NULL;

			// convert the string token to integer
			token_stream.str(token);
			token_stream >> scheme_number;
			if(scheme_number <= 0 || scheme_number > MAX_FMT)
			{
				LOG(this->log_fmt, LEVEL_ERROR,
				    "bad syntax at line %u (%s): FMT "
				    "definition should start with a non-zero "
				    "positive integer up to %u\n", lines_count,
				    line.substr(0,16).c_str(), MAX_FMT);
				goto malformed;
			}

			// check if the number of FMTs was found
			if(!is_nb_fmt_found)
			{
				// the 'nb_fmt' keyword should be
				// specified before any FMTs line
				LOG(this->log_fmt, LEVEL_ERROR,
				    "bad syntax at line %u (%s): FMT "
				    "definition before the 'nb_fmt' keyword\n",
				    lines_count, line.substr(0,16).c_str());
				goto malformed;
			}

			// one more FMT found
			nb_fmt_read++;
			if(nb_fmt_read > ((unsigned int) nb_fmt))
			{
				LOG(this->log_fmt, LEVEL_ERROR,
				    "bad syntax at line %u (%s): %u or more "
				    "FMTs definitions found, but only %d specified"
				    "with the 'nb_fmt' keyword\n", lines_count,
				    line.substr(0,16).c_str(), nb_fmt_read, nb_fmt);
				goto malformed;
			}

			// get all the parameters of the FMT
			line_stream >> modulation_type
			            >> coding_type
			            >> spectral_efficiency
			            >> required_es_n0;
			if(!line_stream.eof())
			{
				line_stream >> burst_length;
				burst_length_found = true;
			}
			if(burst_length_found)
			{
				if(req_burst_length == 0 || req_burst_length != burst_length)
				{
					LOG(this->log_fmt, LEVEL_NOTICE,
					    "unmatching FMT definition: %u, %s, %s, %f, %f, %u sym "
					    "(required burst length: %u sym)\n",
					    scheme_number, modulation_type.c_str(),
					    coding_type.c_str(), spectral_efficiency,
					    required_es_n0, burst_length, req_burst_length);
					continue;
				}

				fmt_def = new FmtDefinition(
					scheme_number,
					modulation_type,
					coding_type,
					spectral_efficiency,
					required_es_n0,
					burst_length);
			}
			else
			{
				if(req_burst_length != 0)
				{
					LOG(this->log_fmt, LEVEL_WARNING,
					    "unmatching FMT definition: %u, %s, %s, %f, %f, no burst length "
					    "(required burst length: %u sym)\n",
					    scheme_number, modulation_type.c_str(),
					    coding_type.c_str(), spectral_efficiency,
					    required_es_n0, req_burst_length);
					continue;
				}

				fmt_def = new FmtDefinition(
					scheme_number,
					modulation_type,
					coding_type,
					spectral_efficiency,
					required_es_n0);
			}
			nb_req_fmt_read++;

			// FMT definition is OK, record it in the table
			ret = this->add(fmt_def);
			if(ret != true)
			{
				if(burst_length_found)
				{
					LOG(this->log_fmt, LEVEL_ERROR,
					    "failed to add new FMT definition: "
					    "%u, %s, %s, %f, %f, %u\n",
					    scheme_number, modulation_type.c_str(),
					    coding_type.c_str(), spectral_efficiency,
					    required_es_n0, burst_length);
				}
				else
				{
					LOG(this->log_fmt, LEVEL_ERROR,
					    "failed to add new FMT definition: "
					    "%u, %s, %s, %f, %f\n",
					    scheme_number, modulation_type.c_str(),
					    coding_type.c_str(), spectral_efficiency,
					    required_es_n0);
				}
				goto malformed;
			}

			if(burst_length_found)
			{
				LOG(this->log_fmt, LEVEL_NOTICE,
				    "FMT definition: %u, %s, %s, %f, %f, %u\n",
				    scheme_number, modulation_type.c_str(),
				    coding_type.c_str(), spectral_efficiency,
				    required_es_n0, burst_length);
			}
			else
			{
				LOG(this->log_fmt, LEVEL_NOTICE,
				    "FMT definition: %u, %s, %s, %f, %f\n",
				    scheme_number, modulation_type.c_str(),
				    coding_type.c_str(), spectral_efficiency,
				    required_es_n0);
			}
		}
	}

	// check the number of FMTs read in definition file
	if(nb_fmt_read != ((unsigned int) nb_fmt))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "too few FMTs definitions: %u found while %d specified"
		    " with the 'nb_fmt' keyword\n",
		    nb_fmt_read, nb_fmt);
		goto malformed;
	}
	if(nb_fmt_read == nb_req_fmt_read)
	{
		LOG(this->log_fmt, LEVEL_NOTICE,
		    "%d FMTs found in definition file\n", nb_fmt);
	}
	else
	{
		LOG(this->log_fmt, LEVEL_NOTICE,
		    "%u required FMTs found in definition file (%u total FMTs found)\n",
		    nb_req_fmt_read, nb_fmt_read);
	}

	// close the definition file
	file.close();

	return true;

malformed:
	LOG(this->log_fmt, LEVEL_ERROR,
	    "malformed FMT definition file\n");
	file.close();
error:
	return false;
}


bool FmtDefinitionTable::add(FmtDefinition *fmt_def)
{
	// check that the table does not already own a FMT definition
	// with the same identifier
	if(fmt_def == NULL || this->doFmtIdExist(fmt_def->getId()))
	{
		return false;
	}

	this->definitions[fmt_def->getId()] = fmt_def;
	return true;
}

bool FmtDefinitionTable::doFmtIdExist(fmt_id_t id) const
{
	return (this->definitions.find(id) != this->definitions.end());
}


void FmtDefinitionTable::clear()
{
	map<fmt_id_t, FmtDefinition *>::iterator it;

	// delete all stored FMT definitions
	for(it = this->definitions.begin(); it != this->definitions.end(); ++it)
	{
		delete it->second;
	}

	// now clear the map itself
	this->definitions.clear();
}


map<fmt_id_t, FmtDefinition* > FmtDefinitionTable::getDefinitions(void) const
{
	return this->definitions;
}


unsigned int FmtDefinitionTable::getModulationEfficiency(fmt_id_t id) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(!def)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find modulation efficiency from FMT definition ID %u\n", id);
		return ModulationTypes::getDefaultEfficiency();
	}
	return def->getModulationEfficiency();
}


float FmtDefinitionTable::getCodingRate(fmt_id_t id) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(!def)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find coding rate from FMT definition ID %u\n", id);
		return CodingTypes::getDefaultRate();
	}
	return def->getCodingRate();
}


float FmtDefinitionTable::getSpectralEfficiency(fmt_id_t id) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(!def)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find spectral efficiency from FMT definition ID %u\n", id);
		return 0.0;
	}
	return def->getSpectralEfficiency();
}


double FmtDefinitionTable::getRequiredEsN0(fmt_id_t id) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(!def)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find required Es/N0 from FMT definition ID %u\n", id);
		return 0.0;
	}
	return def->getRequiredEsN0();
}

bool FmtDefinitionTable::hasBurstLength(fmt_id_t id) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(!def)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find burst length presence status from FMT definition ID %u\n", id);
		return false;
	}
	return def->hasBurstLength();
}

vol_sym_t FmtDefinitionTable::getBurstLength(fmt_id_t id) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(!def)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find burst length from FMT definition ID %u\n", id);
		return 0;
	}
	return def->getBurstLength();
}

fmt_id_t FmtDefinitionTable::getRequiredModcod(double cni) const
{
	fmt_id_t modcod_id = 0;
	double current_cni;
	double previous_cni = 0.0;
	if(this->definitions.begin() != this->definitions.end())
	{
		previous_cni = this->definitions.begin()->second->getRequiredEsN0();
	}
	fmt_def_table_pos_t it;

	for(it = this->definitions.begin(); it != this->definitions.end(); it++)
	{
		current_cni = (*it).second->getRequiredEsN0();
		if(current_cni > cni)
		{
			// not supported
			continue;
		}
		// here we have a supported Es/N0 value check if it is better than the
		// previous one
		if(current_cni >= previous_cni)
		{
			previous_cni = current_cni;
			modcod_id = (*it).first;
		}
	}
	if(modcod_id <= 0)
	{
		// use at least most robust MODCOD
		modcod_id = this->getMinId();	
	}
	return modcod_id;
}


FmtDefinition *FmtDefinitionTable::getDefinition(fmt_id_t id) const
{
	fmt_def_table_pos_t it;
	FmtDefinition *def = NULL;

	it = this->definitions.find(id);
	if(it != this->definitions.end())
	{
		def = it->second;
	}

	return def;
}

fmt_id_t FmtDefinitionTable::getMinId() const
{
	fmt_def_table_pos_t it = this->definitions.begin();
	fmt_id_t id;
	if(this->definitions.size() <= 0)
	{
		return 0;	
	}
	id = (*it).first;
	++it;
	for(; it != this->definitions.end(); ++it)
	{
		if((*it).first < id)
		{
			id = (*it).first;
		}
	}
	return id;
}

fmt_id_t FmtDefinitionTable::getMaxId() const
{
	fmt_def_table_pos_t it = this->definitions.begin();
	fmt_id_t id;
	if(this->definitions.size() <= 0)
	{
		return 0;	
	}
	id = (*it).first;
	++it;
	for(; it != this->definitions.end(); ++it)
	{
		if((*it).first > id)
		{
			id = (*it).first;
		}
	}
	return id;
}

vol_kb_t FmtDefinitionTable::symToKbits(fmt_id_t id,
                                        vol_sym_t vol_sym) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(def == NULL)
	{
		return 0;
	}
	return def->symToKbits(vol_sym);
}


vol_sym_t FmtDefinitionTable::kbitsToSym(fmt_id_t id,
                                         vol_kb_t vol_kb) const
{
	FmtDefinition *def = this->getDefinition(id);
	if(def == NULL)
	{
		return 0;
	}
	return def->kbitsToSym(vol_kb);
}


void FmtDefinitionTable::print(void) const
{
	fmt_def_table_pos_t it;

	for(it = this->definitions.begin();
	    it != this->definitions.end(); it++)
	{
		it->second->print();
	}

	if(this->definitions.begin() == this->definitions.end())
	{
		DFLTLOG(LEVEL_ERROR, "Vide\n");
	}
}


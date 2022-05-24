/*
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
#include <string>


/// The maximum entries number in FMT definitions table
#define MAX_FMT 32


// Returns false if the string contains any non-whitespace characters
inline bool isSpace(std::string str)
{
  std::string::iterator it = str.begin();
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
	this->log_fmt = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Fmt.DefinitionTable");
}


/**
 * @brief Destroy a table of FMT definitions
 */
FmtDefinitionTable::~FmtDefinitionTable()
{
	this->clear();
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
  std::map<fmt_id_t, FmtDefinition *>::iterator it;

	// delete all stored FMT definitions
	for(it = this->definitions.begin(); it != this->definitions.end(); ++it)
	{
		delete it->second;
	}

	// now clear the map itself
	this->definitions.clear();
}


std::map<fmt_id_t, FmtDefinition* > FmtDefinitionTable::getDefinitions(void) const
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


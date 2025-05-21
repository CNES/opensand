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
/*
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
*/

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


bool FmtDefinitionTable::add(std::unique_ptr<FmtDefinition> fmt_def)
{
	auto id = fmt_def->getId();

	// check that the table does not already own a FMT definition
	// with the same identifier
	if (!fmt_def || this->doFmtIdExist(id))
	{
		return false;
	}

	this->definitions.emplace(id, std::move(fmt_def));
	return true;
}

bool FmtDefinitionTable::doFmtIdExist(fmt_id_t id) const
{
	return (this->definitions.find(id) != this->definitions.end());
}


void FmtDefinitionTable::clear()
{
	// now clear the map itself
	this->definitions.clear();
}


/*
std::map<fmt_id_t, std::unique_ptr<FmtDefinition>> FmtDefinitionTable::getDefinitions() const
{
	return this->definitions;
}
*/


unsigned int FmtDefinitionTable::getModulationEfficiency(fmt_id_t id) const
{
	try
	{
		return this->getDefinition(id).getModulationEfficiency();
	}
	catch (const std::range_error&)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find modulation efficiency from FMT definition ID %u\n", id);
		return ModulationTypes::getDefaultEfficiency();
	}
}


float FmtDefinitionTable::getCodingRate(fmt_id_t id) const
{
	try
	{
		return this->getDefinition(id).getCodingRate();
	}
	catch (const std::range_error&)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find coding rate from FMT definition ID %u\n", id);
		return CodingTypes::getDefaultRate();
	}
}


float FmtDefinitionTable::getSpectralEfficiency(fmt_id_t id) const
{
	try
	{
		return this->getDefinition(id).getSpectralEfficiency();
	}
	catch (const std::range_error&)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find spectral efficiency from FMT definition ID %u\n", id);
		return 0.0;
	}
}


double FmtDefinitionTable::getRequiredEsN0(fmt_id_t id) const
{
	try
	{
		return this->getDefinition(id).getRequiredEsN0();
	}
	catch (const std::range_error&)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find required Es/N0 from FMT definition ID %u\n", id);
		return 0.0;
	}
}

bool FmtDefinitionTable::hasBurstLength(fmt_id_t id) const
{
	try
	{
		return this->getDefinition(id).hasBurstLength();
	}
	catch (const std::range_error&)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find burst length presence status from FMT definition ID %u\n", id);
		return false;
	}
}

vol_sym_t FmtDefinitionTable::getBurstLength(fmt_id_t id) const
{
	try
	{
		return this->getDefinition(id).getBurstLength();
	}
	catch (const std::range_error&)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot find burst length from FMT definition ID %u\n", id);
		return 0;
	}
}

fmt_id_t FmtDefinitionTable::getRequiredModcod(double cni) const
{
	fmt_id_t modcod_id = 0;
	double previous_cni = 0.0;
	if(this->definitions.begin() != this->definitions.end())
	{
		previous_cni = this->definitions.begin()->second->getRequiredEsN0();
	}

	for(auto &&[current_modcod_id, current_definition]: this->definitions)
	{
		double current_cni = current_definition->getRequiredEsN0();
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
			modcod_id = current_modcod_id;
		}
	}
	if(modcod_id <= 0)
	{
		// use at least most robust MODCOD
		modcod_id = this->getMinId();	
	}
	return modcod_id;
}


FmtDefinition &FmtDefinitionTable::getDefinition(fmt_id_t id) const
{
	auto it = this->definitions.find(id);
	if(it != this->definitions.end())
	{
		return *(it->second);
	}

	throw std::range_error("Id not found in definition table");
}

fmt_id_t FmtDefinitionTable::getMinId() const
{
	fmt_id_t id = this->definitions.size() ? this->definitions.begin()->first : 0;

	for (auto &&it: this->definitions)
	{
		auto current_id = it.first;
		if (current_id < id)
		{
			id = current_id;
		}
	}
	return id;
}

fmt_id_t FmtDefinitionTable::getMaxId() const
{
	fmt_id_t id = this->definitions.size() ? this->definitions.begin()->first : 0;

	for (auto &&it: this->definitions)
	{
		auto current_id = it.first;
		if (current_id > id)
		{
			id = current_id;
		}
	}
	return id;
}

vol_kb_t FmtDefinitionTable::symToKbits(fmt_id_t id, vol_sym_t vol_sym) const
{
	try
	{
		return this->getDefinition(id).symToKbits(vol_sym);
	}
	catch (const std::range_error&)
	{
		return 0;
	}
}


vol_sym_t FmtDefinitionTable::kbitsToSym(fmt_id_t id, vol_kb_t vol_kb) const
{
	try
	{
		return this->getDefinition(id).kbitsToSym(vol_kb);
	}
	catch (const std::range_error&)
	{
		return 0;
	}
}


void FmtDefinitionTable::print() const
{
	for (auto &&it: this->definitions)
	{
		it.second->print();
	}

	if(this->definitions.begin() == this->definitions.end())
	{
		DFLTLOG(LEVEL_ERROR, "Vide\n");
	}
}


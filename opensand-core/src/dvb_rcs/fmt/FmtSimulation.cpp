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
 * @file FmtSimulation.cpp
 * @brief The FMT simulation elements
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>

#include "FmtSimulation.h"

#include <sstream>
#include <cstdlib>


/**
 * @brief Create a list of Satellite Terminals (ST)
 */
FmtSimulation::FmtSimulation():
	sts(),
	modcod_definitions(),
	modcod_simu_file(),
	dra_scheme_definitions(),
	dra_scheme_simu_file(),
	is_modcod_simu_file_defined(false),
	is_dra_scheme_simu_file_defined(false)
{
}


/**
 * @brief Destroy a list of Satellite Terminals (ST)
 */
FmtSimulation::~FmtSimulation()
{
	this->clear();

	if(this->modcod_simu_file.is_open())
	{
		this->modcod_simu_file.close();
	}
	if(this->dra_scheme_simu_file.is_open())
	{
		this->dra_scheme_simu_file.close();
	}
}

/**
 * @brief Add a new Satellite Terminal (ST) in the list
 *
 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
 *                         in the code)
 * @param simu_column_num  the column # associated to the ST for DRA/MODCOD
 *                         simulation files
 * @return                 true if the addition is successful, false otherwise
 */
bool FmtSimulation::addTerminal(tal_id_t id,
                                unsigned long simu_column_num)
{
	StFmtSimu *new_st;

	// check that the list does not already own a ST
	// with the same identifier
	if(this->doTerminalExist(id))
	{
		UTI_ERROR("one ST with ID %u already exist in list\n", id);
		return false;
	}

	// create the new ST
	if(this->is_modcod_simu_file_defined &&
	   this->modcod_list.size() < simu_column_num)
	{
		UTI_ERROR("cannot access modcod column %lu for ST%u\n",
		          simu_column_num, id);
		return false;
	}
	if(this->is_dra_scheme_simu_file_defined &&
	   this->dra_list.size() < simu_column_num)
	{
		UTI_ERROR("cannot access dra column %lu for ST%u\n",
		          simu_column_num, id);
		return false;
	}
	new_st = new StFmtSimu(id, simu_column_num,
		this->is_modcod_simu_file_defined ?
			atoi(this->modcod_list[simu_column_num].c_str()) : 0,
		this->is_dra_scheme_simu_file_defined ?
			atoi(this->dra_list[simu_column_num].c_str()) : 0);
	if(new_st == NULL)
	{
		UTI_ERROR("failed to create a new ST\n");
		return false;
	}

	this->sts[id] = new_st;

	return true;
}


/**
 * @brief Delete a Satellite Terminal (ST) from the list
 *
 * @param id  the ID of the ST (called TAL ID or MAC ID elsewhere in the code)
 * @return    true if the deletion is successful, false otherwise
 */
bool FmtSimulation::delTerminal(tal_id_t id)
{
	map<tal_id_t, StFmtSimu *>::iterator it;

	// find the entry to delete
	it = this->sts.find(id);
	if(it == this->sts.end())
	{
		UTI_ERROR("ST with ID %u not found in list of STs\n", id);
		return false;
	}

	// delete the ST
	delete it->second;
	this->sts.erase(it);

	return true;
}


/**
 * @brief Does a ST with the given ID exist ?
 *
 * @param id  the ID we want to check for
 * @return    true if a ST, false is it does not exist
 */
bool FmtSimulation::doTerminalExist(tal_id_t id) const
{
	return (this->sts.find(id) != this->sts.end());
}


/**
 * @brief Clear the list of STs
 */
void FmtSimulation::clear()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;

	// delete all stored STs
	for(it = this->sts.begin(); it != this->sts.end(); ++it)
	{
		delete it->second;
	}

	// now clear the map itself
	this->sts.clear();
}


/**
 * @brief Go to next step in adaptive physical layer scenario
 *
 * Update current MODCOD and DRA scheme IDs of all STs in the list.
 */
bool FmtSimulation::goNextScenarioStep()
{
	// update MODCOD IDs of all STs ?
	if(this->is_modcod_simu_file_defined)
	{
		if(!this->goNextScenarioStepModcod())
		{
			UTI_ERROR("failed to go to the next step "
			          "for MODCOD scenario\n");
			goto error;
		}
	}

	// update DRA scheme IDs of all STs ?
	if(this->is_dra_scheme_simu_file_defined)
	{
		if(!this->goNextScenarioStepDraScheme())
		{
			UTI_ERROR("failed to go to the next step "
			          "for DRA scheme scenario\n");
			goto error;
		}
	}

	UTI_DEBUG("next MODCOD/DRA scenario step successfully reached\n");

	return true;

error:
	return false;
}


/**
 * @brief Was the current MODCOD IDs of all the STs advertised
 *        over the emulated network ?
 *
 * @return  true if the current MODCOD IDs of all the STs are already
 *          advertised, false if they were not yet
 */
bool FmtSimulation::areCurrentModcodsAdvertised()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;
	bool all_advertised = true;

	// check if the MODCOD ID of each ST is advertised or not
	for(it = this->sts.begin(); it != this->sts.end(); ++it)
	{
		all_advertised &= it->second->isCurrentModcodAdvertised();
	}

	return all_advertised;
}


bool FmtSimulation::setModcodDefFile(string filename)
{
	// load all the MODCOD definitions from file
	if(!this->modcod_definitions.load(filename))
	{
		UTI_ERROR("failed to load all the MODCOD definitions "
		          "from file '%s'\n", filename.c_str());
		return false;
	}
	return true;
}


/**
 * @brief Set simulation file for MODCOD
 *
 * @param filename  the name of the file in which MODCOD scenario is described
 * @return          true if the file exist and is valid, false otherwise
 */
bool FmtSimulation::setModcodSimuFile(string filename)
{
	// we can not redefine the simulation file
	if(this->is_modcod_simu_file_defined)
	{
		UTI_ERROR("cannot redefine the MODCOD simulation file\n");
		goto error;
	}

	// open the simulation file
	this->modcod_simu_file.open(filename.c_str());
	if(!this->modcod_simu_file.is_open())
	{
		UTI_ERROR("failed to open MODCOD simulation file '%s'\n",
		          filename.c_str());
		goto error;
	}

	// TODO: check values in the file here

	this->is_modcod_simu_file_defined = true;

	return true;

error:
	return false;
}

bool FmtSimulation::setDraSchemeDefFile(string filename)
{
	// load all the MODCOD definitions from file
	if(!this->dra_scheme_definitions.load(filename))
	{
		UTI_ERROR("failed to load all the DRA definitions "
		          "from file '%s'\n", filename.c_str());
		return false;
	}
	return true;
}


/**
 * @brief Set simulation file for DRA scheme
 *
 * @param filename  the name of the file in which DRA scheme scenario is described
 * @return          true if the file exist and is valid, false otherwise
 */
bool FmtSimulation::setDraSchemeSimuFile(string filename)
{
	// we can not redefine the simulation file
	if(this->is_dra_scheme_simu_file_defined)
	{
		UTI_ERROR("cannot redefine the DRA scheme simulation file\n");
		goto error;
	}

	// open the simulation file
	this->dra_scheme_simu_file.open(filename.c_str());
	if(!this->dra_scheme_simu_file.is_open())
	{
		UTI_ERROR("failed to open DRA scheme simulation file '%s'\n",
		          filename.c_str());
		goto error;
	}

	// TODO: check values in the file here

	this->is_dra_scheme_simu_file_defined = true;

	return true;

error:
	return false;
}

/**
 * @brief Get the terminal ID for wich the used ModCod is the lower
 *
 * @return terminal ID (should be positive, return -1 if an error occurs)
 */
tal_id_t FmtSimulation::getTalIdCorrespondingToLowerModcod() const
{
	map<tal_id_t, StFmtSimu*>::const_iterator st_iterator;
	unsigned int modcod_id;
	unsigned int lower_modcod_id = 0;
	tal_id_t tal_id;
	tal_id_t lower_tal_id = -1;
	bool do_advertise_modcod;

	UTI_DEBUG_L3("In getTalIdCorrespondingToLowerModcod()\n");

	for(st_iterator = this->sts.begin(); st_iterator != this->sts.end();
	    ++st_iterator)
	{
		// Retrieve the lower modcod
		tal_id = st_iterator->first;

		UTI_DEBUG_L3("reading modcod of tal_id: %u\n", tal_id);


		// retrieve the current MODCOD for the ST and whether
		// it changed or not
		if(!this->doTerminalExist(tal_id))
		{
			UTI_ERROR("encapsulation packet is for ST with ID %u "
		          "that is not registered\n", tal_id);
			goto error;
		}
		do_advertise_modcod =
			!this->isCurrentModcodAdvertised(tal_id);
		if(!do_advertise_modcod)
		{
			modcod_id = this->getCurrentModcodId(tal_id);
		}
		else
		{
			modcod_id = this->getPreviousModcodId(tal_id);
		}
		UTI_DEBUG_L3("MODCOD for ST ID %u = %u (changed = %s)\n",
	             tal_id, modcod_id,
	             do_advertise_modcod ? "yes" : "no");

#if 0 /* TODO: manage options */
		if(do_advertise_modcod)
		{
			this->createOptionModcod(comp, nb_row, *modcod_id, id);
		}
#endif

		if((st_iterator==this->sts.begin()) || (modcod_id < lower_modcod_id))
		{
			lower_modcod_id = modcod_id;
			lower_tal_id = tal_id;
		}
	}

	UTI_DEBUG_L3("TAL_ID corresponding to lower modcod: %u\n", lower_tal_id);

	return lower_tal_id;

error:
	return -1;
}

/**
 * @brief Get the column # associated to the ST whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the column # associated to the ST
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
unsigned int FmtSimulation::getSimuColumnNum(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getSimuColumnNum();
	}
	return 0;
}


/**
 * @brief Get the current MODCOD ID of the ST whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the current MODCOD ID of the ST
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
unsigned int FmtSimulation::getCurrentModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getCurrentModcodId();
	}
	return 0;
}


/**
 * @brief Get the previous MODCOD ID of the ST whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the previous MODCOD ID of the ST
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
unsigned int FmtSimulation::getPreviousModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getPreviousModcodId();
	}
	return 0;

}


/**
 * @brief Was the current MODCOD ID of the ST whose ID is given as input
 *        advertised over the emulated network ?
 *
 * @return  true if the MODCOD ID was already advertised,
 *          false if it was not advertised yet
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
bool FmtSimulation::isCurrentModcodAdvertised(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->isCurrentModcodAdvertised();
	}
	return 0;

}


/**
 * @brief Get the current DRA scheme ID of the ST whose ID is given as input
 *
 * @param id  the ID of the DRA scheme definition we want information for
 * @return    the current DRA scheme ID of the ST
 *
 * @warning Be sure sure that the ID is valid before calling the function
 */
unsigned int FmtSimulation::getCurrentDraSchemeId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getCurrentDraSchemeId();
	}
	return 0;
}



/**** private methods ****/


/**
 * @brief Update the current MODCOD IDs of all STs from MODCOD simulation file
 *
 * @return true on success, false on failure
 */
bool FmtSimulation::goNextScenarioStepModcod()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;

	if(!this->is_modcod_simu_file_defined)
	{
		UTI_ERROR("failed to update MODCOD IDs: "
		          "MODCOD simulation file not defined yet\n");
		goto error;
	}

	// read next line of the modcod simulation file
	if(!this->setList(this->modcod_simu_file, this->modcod_list))
	{
		UTI_ERROR("failed to get the next line in the MODCOD scheme simulation file\n");
		goto error;
	}

	// update all STs in list
	for(it = this->sts.begin(); it != this->sts.end(); ++it)
	{
		StFmtSimu *st;
		tal_id_t st_id;
		unsigned long column;

		st = it->second;
		st_id = st->getId();
		column = st->getSimuColumnNum();

		UTI_DEBUG_L3("ST with ID %u uses MODCOD ID at column %lu\n",
		             st_id, column);

		if(this->modcod_list.size() < column)
		{
			UTI_ERROR("cannot access modcod column %lu for ST%u\n",
			          column, st_id);
			goto error;
		}
		// replace the current MODCOD ID by the new one
		st->updateModcodId(atoi(this->modcod_list[column].c_str()));

		UTI_DEBUG_L3("new MODCOD ID of ST with ID %u = %u\n", st_id,
		             atoi(this->modcod_list[column].c_str()));
	}

	return true;

error:
	return false;
}

/**
 * @brief Update the current DRA scheme IDs of all STs from DRA simulation file
 *
 * @return true on success, false on failure
 */
bool FmtSimulation::goNextScenarioStepDraScheme()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;

	if(!this->is_dra_scheme_simu_file_defined)
	{
		UTI_ERROR("failed to update DRA scheme IDs: "
		          "DRA scheme simulation file not defined yet\n");
		goto error;
	}

	// read next line of the dra simulation file
	if(!this->setList(this->dra_scheme_simu_file, this->dra_list))
	{
		UTI_ERROR("failed to get the next line in the DRA scheme simulation file\n");
		goto error;
	}

	// update all STs in list
	for(it = this->sts.begin(); it != this->sts.end(); ++it)
	{
		StFmtSimu *st;
		tal_id_t st_id;
		unsigned long column;

		st = it->second;
		st_id = st->getId();
		column = st->getSimuColumnNum();

		UTI_DEBUG("ST with ID %u uses DRA scheme ID at column %lu\n",
		          st_id, column);

		if(this->dra_list.size() < column)
		{
			UTI_ERROR("cannot access dra column %lu for ST%u\n",
			          column, st_id);
			goto error;
		}
		// replace the current MODCOD ID by the new one
		st->updateDraSchemeId(atoi(this->dra_list[column].c_str()));

		UTI_DEBUG("new DRA scheme ID of ST with ID %u = %u\n", st_id,
		          atoi(this->dra_list[column].c_str()));
	}

	return true;

error:
	return false;
}

const FmtDefinitionTable *FmtSimulation::getModcodDefinitions() const
{
	return &(this->modcod_definitions);
}

const FmtDefinitionTable *FmtSimulation::getDraSchemeDefinitions() const
{
	return &(this->dra_scheme_definitions);
}

/**
 * @brief Read a line of a simulation file and fill the MODCOD/DRA list
 *
 * @param   simu_file the simulation file (modcod_simu_file or dra_scheme_simu_file)
 * @param   list      The MODCOD/DRA list
 * @return            true on success, false on failure
 *
 * @todo better parsing
 */
bool FmtSimulation::setList(ifstream &simu_file, vector<string> &list)
{
	std::stringbuf buf;
	std::stringstream line;
	std::stringbuf token;

	// get the next line in the file
	simu_file.get(buf);
	if(buf.str() != "")
	{
		line.str(buf.str());
		// get each element of the line
		while(!line.fail())
		{
			token.str("");
			line.get(token, ' ');
			list.push_back(token.str());
			line.ignore();
		}
	}

	// restart from beginning of file when we reach the end of file
	if(simu_file.eof())
	{
		// reset the error flags
		simu_file.clear();
		UTI_DEBUG("end of simulation file reached, restart at beginning...\n");
		simu_file.seekg(0, std::ios::beg);
		if(simu_file.fail())
		{
			UTI_ERROR("Error when going to the begining of the simulation file\n");
			goto error;
		}
		else
		{
			buf.str("");
			// read the first line and get elements
			simu_file.get(buf);
			if(buf.str() != "")
			{
				line.str(buf.str());
				// get each element of the line
				while(!line.fail())
				{
					token.str("");
					line.get(token, ' ');
					list.push_back(token.str());
					line.ignore();
				}
			}
		}
	}

	// check if getline returned an error
	if(simu_file.fail())
	{
		UTI_ERROR("Error when getting next line of the simulation file\n");
		goto error;
	}

	// reset the error flags
	simu_file.clear();

	// jump after the '\n' as get does not read it
	simu_file.ignore();

	return true;

error:
	return false;
}

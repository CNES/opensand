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
#include <errno.h>
#include <string.h>

/**
 * @brief Check if a file exists
 *
 * @return true if the file is found, false otherwise
 */
inline bool fileExists(const string &filename)
{
	if(access(filename.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		           filename.c_str(), strerror(errno));
		return false;
	}
	return true;
}


FmtSimulation::FmtSimulation():
	sts(),
	fwd_modcod_def(),
	fwd_modcod_simu(),
	ret_modcod_def(),
	ret_modcod_simu(),
	is_fwd_modcod_simu_defined(false),
	is_ret_modcod_simu_defined(false)
{
}


/**
 * @brief Destroy a list of Satellite Terminals (ST)
 */
FmtSimulation::~FmtSimulation()
{
	this->clear();

	if(this->fwd_modcod_simu.is_open())
	{
		this->fwd_modcod_simu.close();
	}
	if(this->ret_modcod_simu.is_open())
	{
		this->ret_modcod_simu.close();
	}
}

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
	if(this->is_fwd_modcod_simu_defined &&
	   this->fwd_modcod_list.size() < simu_column_num)
	{
		UTI_ERROR("cannot access down/forward modcod column %lu for ST%u\n",
		          simu_column_num, id);
		return false;
	}
	if(this->is_ret_modcod_simu_defined &&
	   this->ret_modcod_list.size() < simu_column_num)
	{
		UTI_ERROR("cannot access up/return modcod  column %lu for ST%u\n",
		          simu_column_num, id);
		return false;
	}
	new_st = new StFmtSimu(id, simu_column_num,
		this->is_fwd_modcod_simu_defined ?
			atoi(this->fwd_modcod_list[simu_column_num].c_str()) : 0,
		this->is_ret_modcod_simu_defined ?
			atoi(this->ret_modcod_list[simu_column_num].c_str()) : 0);
	if(new_st == NULL)
	{
		UTI_ERROR("failed to create a new ST\n");
		return false;
	}

	this->sts[id] = new_st;

	return true;
}


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


bool FmtSimulation::doTerminalExist(tal_id_t id) const
{
	return (this->sts.find(id) != this->sts.end());
}


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


bool FmtSimulation::goNextScenarioStep()
{
	// update down/forward MODCOD IDs of all STs ?
	if(this->is_fwd_modcod_simu_defined)
	{
		if(!this->goNextScenarioStepFwdModcod())
		{
			UTI_ERROR("failed to go to the next step "
			          "for down/foward link MODCOD scenario\n");
			goto error;
		}
	}

	// update up/return MODCOD scheme IDs of all STs ?
	if(this->is_ret_modcod_simu_defined)
	{
		if(!this->goNextScenarioStepRetModcod())
		{
			UTI_ERROR("failed to go to the next step "
			          "for up/return link MODCOD scenario\n");
			goto error;
		}
	}

	UTI_DEBUG("next MODCODscenario step successfully reached\n");

	return true;

error:
	return false;
}


bool FmtSimulation::areCurrentFwdModcodsAdvertised()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;
	bool all_advertised = true;

	// check if the MODCOD ID of each ST is advertised or not
	for(it = this->sts.begin(); it != this->sts.end(); ++it)
	{
		all_advertised &= it->second->isCurrentFwdModcodAdvertised();
	}

	return all_advertised;
}


bool FmtSimulation::setForwardModcodDef(const string &filename)
{
	if(!fileExists(filename.c_str()))
	{
		return false;;
	}

	// load all the MODCOD definitions from file
	if(!this->fwd_modcod_def.load(filename))
	{
		UTI_ERROR("failed to load the down/forward link MODCOD "
		          "definitions from file '%s'\n", filename.c_str());
		return false;
	}
	return true;
}


bool FmtSimulation::setForwardModcodSimu(const string &filename)
{
	// we can not redefine the simulation file
	if(this->is_fwd_modcod_simu_defined)
	{
		UTI_ERROR("cannot redefine the down/forward link MODCOD "
		          "simulation file\n");
		goto error;
	}

	if(!fileExists(filename.c_str()))
	{
		goto error;
	}

	// open the simulation file
	this->fwd_modcod_simu.open(filename.c_str());
	if(!this->fwd_modcod_simu.is_open())
	{
		UTI_ERROR("failed to open down/forward link MODCOD "
		          "simulation file '%s'\n", filename.c_str());
		goto error;
	}

	// TODO: check values in the file here

	this->is_fwd_modcod_simu_defined = true;

	return true;

error:
	return false;
}

bool FmtSimulation::setReturnModcodDef(const string &filename)
{
	if(!fileExists(filename.c_str()))
	{
		return false;
	}

	// load all the MODCOD definitions from file
	if(!this->ret_modcod_def.load(filename))
	{
		UTI_ERROR("failed to load the up/return link MODCOD "
		          "definitiosn from file '%s'\n", filename.c_str());
		return false;
	}
	return true;
}


bool FmtSimulation::setReturnModcodSimu(const string &filename)
{
	// we can not redefine the simulation file
	if(this->is_ret_modcod_simu_defined)
	{
		UTI_ERROR("cannot redefine the up/return link MODCOD "
		          "simulation file\n");
		goto error;
	}

	if(!fileExists(filename.c_str()))
	{
		goto error;
	}

	// open the simulation file
	this->ret_modcod_simu.open(filename.c_str());
	if(!this->ret_modcod_simu.is_open())
	{
		UTI_ERROR("failed to open up/return link MODCOD "
		          "simulation file '%s'\n", filename.c_str());
		goto error;
	}

	// TODO: check values in the file here

	this->is_ret_modcod_simu_defined = true;

	return true;

error:
	return false;
}


tal_id_t FmtSimulation::getTalIdWithLowerFwdModcod() const
{
	map<tal_id_t, StFmtSimu*>::const_iterator st_iterator;
	unsigned int modcod_id;
	unsigned int lower_modcod_id = 0;
	tal_id_t tal_id;
	tal_id_t lower_tal_id = -1;
	bool do_advertise_modcod;

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
			!this->isCurrentFwdModcodAdvertised(tal_id);
		if(!do_advertise_modcod)
		{
			modcod_id = this->getCurrentFwdModcodId(tal_id);
		}
		else
		{
			modcod_id = this->getPreviousFwdModcodId(tal_id);
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


unsigned int FmtSimulation::getCurrentFwdModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getCurrentFwdModcodId();
	}
	return 0;
}


unsigned int FmtSimulation::getPreviousFwdModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getPreviousFwdModcodId();
	}
	return 0;

}


bool FmtSimulation::isCurrentFwdModcodAdvertised(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->isCurrentFwdModcodAdvertised();
	}
	return 0;

}


unsigned int FmtSimulation::getCurrentRetModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getCurrentRetModcodId();
	}
	return 0;
}


const FmtDefinitionTable *FmtSimulation::getFwdModcodDefinitions() const
{
	return &(this->fwd_modcod_def);
}


const FmtDefinitionTable *FmtSimulation::getRetModcodDefinitions() const
{
	return &(this->ret_modcod_def);
}

/**** private methods ****/


bool FmtSimulation::goNextScenarioStepFwdModcod()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;

	if(!this->is_fwd_modcod_simu_defined)
	{
		UTI_ERROR("failed to update MODCOD IDs: "
		          "MODCOD simulation file not defined yet\n");
		goto error;
	}

	// read next line of the modcod simulation file
	if(!this->setList(this->fwd_modcod_simu, this->fwd_modcod_list))
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

		if(this->fwd_modcod_list.size() < column)
		{
			UTI_ERROR("cannot access modcod column %lu for ST%u\n",
			          column, st_id);
			goto error;
		}
		// replace the current MODCOD ID by the new one
		st->updateFwdModcodId(atoi(this->fwd_modcod_list[column].c_str()));

		UTI_DEBUG_L3("new MODCOD ID of ST with ID %u = %u\n", st_id,
		             atoi(this->fwd_modcod_list[column].c_str()));
	}

	return true;

error:
	return false;
}


bool FmtSimulation::goNextScenarioStepRetModcod()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;

	if(!this->is_ret_modcod_simu_defined)
	{
		UTI_ERROR("failed to update up/return MODCOD IDs: "
		          "up/return MODCOD simulation file not defined yet\n");
		goto error;
	}

	// read next line of the modcod simulation file
	if(!this->setList(this->ret_modcod_simu, this->ret_modcod_list))
	{
		UTI_ERROR("failed to get the next line in the up/return MODCOD "
		          "simulation file\n");
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

		UTI_DEBUG("ST with ID %u uses up/return MODCOD ID at column %lu\n",
		          st_id, column);

		if(this->ret_modcod_list.size() < column)
		{
			UTI_ERROR("cannot access up/return MODCOD column %lu for ST%u\n",
			          column, st_id);
			goto error;
		}
		// replace the current MODCOD ID by the new one
		st->updateRetModcodId(atoi(this->ret_modcod_list[column].c_str()));

		UTI_DEBUG("new up/return MODCOD ID of ST with ID %u = %u\n", st_id,
		          atoi(this->ret_modcod_list[column].c_str()));
	}

	return true;

error:
	return false;
}


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

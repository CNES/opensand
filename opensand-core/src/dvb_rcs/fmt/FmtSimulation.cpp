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


#include "FmtSimulation.h"

#include <opensand_output/Output.h>

#include <sstream>
#include <cstdlib>
#include <errno.h>
#include <string.h>


// TODO we say that if modcod id1 < modcod id2, then
// modcod id1 is more robust thant modcod id2 but this is not really
// the case as the modcod are ordered per modulation type and not
// per Es/N0

/**
 * @brief Check if a file exists
 *
 * @return true if the file is found, false otherwise
 */
inline bool fileExists(const string &filename)
{
	if(access(filename.c_str(), R_OK) < 0)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot access '%s' file (%s)\n",
		        filename.c_str(), strerror(errno));
		return false;
	}
	return true;
}


FmtSimulation::FmtSimulation():
	sts(),
	modcod_def(),
	modcod_simu(NULL),
	is_modcod_simu_defined(false),
	need_advertise()
{
	this->log_fmt = Output::registerLog(LEVEL_WARNING, "Dvb.Fmt.Simulation");
}


/**
 * @brief Destroy a list of Satellite Terminals (ST)
 */
FmtSimulation::~FmtSimulation()
{
	this->clear();

	if(this->modcod_simu)
	{
		// destructor closes the file
		delete this->modcod_simu;
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
		LOG(this->log_fmt, LEVEL_ERROR,
		    "one ST with ID %u already exist in list\n", id);
		return false;
	}

	if(this->is_modcod_simu_defined &&
	   this->modcod_list.size() <= simu_column_num)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot access modcod  column %lu for ST%u\n",
		    simu_column_num, id);
		return false;
	}
	// if scenario are not defined, set less robust modcod at init
	// in order to authorize any MODCOD
	new_st = new StFmtSimu(id, simu_column_num,
		this->is_modcod_simu_defined ?
			atoi(this->modcod_list[simu_column_num].c_str()) :
			this->getMaxModcod());
	if(new_st == NULL)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to create a new ST\n");
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
		LOG(this->log_fmt, LEVEL_ERROR,
		    "ST with ID %u not found in list of STs\n", id);
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


bool FmtSimulation::goNextScenarioStep(bool need_advert)
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;

	if(!this->is_modcod_simu_defined)
	{
		return true;
	}

	// read next line of the modcod simulation file
	if(!this->setList(this->modcod_simu, this->modcod_list))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to get the next line in the MODCOD "
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

		LOG(this->log_fmt, LEVEL_DEBUG,
		    "ST with ID %u uses MODCOD ID at column %lu\n",
		    st_id, column);

		if(this->modcod_list.size() <= column)
		{
			LOG(this->log_fmt, LEVEL_ERROR,
			    "cannot access MODCOD column %lu for ST%u\n",
			    column, st_id);
			goto error;
		}
		// replace the current MODCOD ID by the new one
		st->updateModcodId(atoi(this->modcod_list[column].c_str()));
		if(need_advert)
		{
			list<tal_id_t>::iterator tal_it;
			tal_it = std::find(this->need_advertise.begin(),
			                   this->need_advertise.end(), st_id);
			// add the terminal ID in le list of not advertised terminal if necessary
			if(!st->isCurrentModcodAdvertised() &&
			   tal_it == this->need_advertise.end())
			{
				this->need_advertise.push_back(st_id);
			}
		}

		LOG(this->log_fmt, LEVEL_DEBUG,
		    "new MODCOD ID of ST with ID %u = %u\n", st_id,
		    atoi(this->modcod_list[column].c_str()));
	}

	return true;

error:
	return false;
}


bool FmtSimulation::areCurrentModcodsAdvertised()
{
	map<tal_id_t, StFmtSimu *>::const_iterator it;
	bool all_advertised = true;

	// check if the MODCOD ID of each ST is advertised or not
	for(it = this->sts.begin(); it != this->sts.end(); ++it)
	{
		all_advertised &= it->second->isCurrentModcodAdvertised();
	}
	if(all_advertised)
	{
		// clear in case this was not
		this->need_advertise.clear();
	}
	return all_advertised;
}



bool FmtSimulation::setModcodDef(const string &filename)
{
	if(!fileExists(filename.c_str()))
	{
		return false;
	}

	// load all the MODCOD definitions from file
	if(!this->modcod_def.load(filename))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to load the MODCOD definitions from file "
		    "'%s'\n", filename.c_str());
		return false;
	}
	return true;
}


bool FmtSimulation::setModcodSimu(const string &filename)
{
	// we can not redefine the simulation file
	if(this->is_modcod_simu_defined)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "cannot redefine the MODCOD simulation file\n");
		goto error;
	}

	if(!fileExists(filename.c_str()))
	{
		goto error;
	}

	// open the simulation file
	this->modcod_simu = new ifstream(filename.c_str());
	if(!this->modcod_simu->is_open())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to open MODCOD simulation file '%s'\n",
		    filename.c_str());
		goto error;
	}

	// TODO: check values in the file here

	this->is_modcod_simu_defined = true;

	return true;

error:
	return false;
}


tal_id_t FmtSimulation::getTalIdWithLowerModcod() const
{
	map<tal_id_t, StFmtSimu*>::const_iterator st_iterator;
	uint8_t modcod_id;
	uint8_t lower_modcod_id = 0;
	tal_id_t tal_id;
	tal_id_t lower_tal_id = 255;
	bool advertised_modcod;

	for(st_iterator = this->sts.begin(); st_iterator != this->sts.end();
	    ++st_iterator)
	{
		// Retrieve the lower modcod
		tal_id = st_iterator->first;

		// retrieve the current MODCOD for the ST and whether
		// it changed or not
		advertised_modcod = !this->isCurrentModcodAdvertised(tal_id);
		if(!advertised_modcod)
		{
			modcod_id = this->getCurrentModcodId(tal_id);
		}
		else
		{
			modcod_id = this->getPreviousModcodId(tal_id);
		}
		LOG(this->log_fmt, LEVEL_DEBUG,
		    "MODCOD for ST ID %u = %u (changed = %s)\n",
		    tal_id, modcod_id,
		    advertised_modcod ? "yes" : "no");

		if((st_iterator == this->sts.begin()) || (modcod_id < lower_modcod_id))
		{
			lower_modcod_id = modcod_id;
			lower_tal_id = tal_id;
		}
	}

	LOG(this->log_fmt, LEVEL_DEBUG,
	    "TAL_ID corresponding to lower modcod: %u\n", lower_tal_id);

	return lower_tal_id;
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


uint8_t FmtSimulation::getCurrentModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getCurrentModcodId();
	}
	return 0;
}


uint8_t FmtSimulation::getPreviousModcodId(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->getPreviousModcodId();
	}
	return 0;

}


bool FmtSimulation::isCurrentModcodAdvertised(tal_id_t id) const
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
		return (*st_iter).second->isCurrentModcodAdvertised();
	}
	return false;
}


bool FmtSimulation::getNextModcodToAdvertise(tal_id_t &tal_id, uint8_t &modcod_id)
{
	if(this->need_advertise.size() == 0)
	{
		return false;
	}
	tal_id = this->need_advertise.front();
	this->need_advertise.pop_front();
	modcod_id = this->getCurrentModcodId(tal_id);
	this->setModcodAdvertised(tal_id);
	return true;
}


uint8_t FmtSimulation::getMaxModcod() const
{
	return this->modcod_def.getMaxId();
}


const FmtDefinitionTable *FmtSimulation::getModcodDefinitions() const
{
	return &(this->modcod_def);
}

void FmtSimulation::setRequiredModcod(tal_id_t id, double cni) const
{
	uint8_t modcod_id;
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;

	modcod_id = this->modcod_def.getRequiredModcod(cni);
	LOG(this->log_fmt, LEVEL_INFO,
	    "Terminal %u required %.2f dB, will receive allocation "
	    "with MODCOD %u\n", id, cni, modcod_id);
	st_iter = this->sts.find(id);
	if(st_iter != this->sts.end())
	{
	/* For forward :
	   do not advertise because we have the physical layer enabled in this case
		list<tal_id_t>::const_iterator tal_it;

		tal_it = std::find(this->need_advertise.begin(), this->need_advertise.end(), id);
		// add the terminal ID in le list of not advertised terminal if necessary
		if(!(*st_iter).second->isCurrentModcodAdvertised() &&
		   tal_it == this->need_advertise.end())
		{
			this->need_advertise.push_back(id);
		}*/

		return (*st_iter).second->updateModcodId(modcod_id);
	}
}


/**** private methods ****/

bool FmtSimulation::setList(ifstream *simu_file, vector<string> &list)
{
	std::stringbuf buf;
	std::stringstream line;
	std::stringbuf token;
	list.clear();

	// get the next line in the file
	simu_file->get(buf);
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
	if(simu_file->eof())
	{
		// reset the error flags
		simu_file->clear();
		LOG(this->log_fmt, LEVEL_INFO,
		    "end of simulation file reached, restart at beginning...\n");
		simu_file->seekg(0, std::ios::beg);
		if(simu_file->fail())
		{
			LOG(this->log_fmt, LEVEL_ERROR,
			    "Error when going to the begining of the "
			    "simulation file\n");
			goto error;
		}
		else
		{
			buf.str("");
			// read the first line and get elements
			simu_file->get(buf);
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
	if(simu_file->fail())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Error when getting next line of the simulation "
		    "file\n");
		goto error;
	}

	// reset the error flags
	simu_file->clear();

	// jump after the '\n' as get does not read it
	simu_file->ignore();

	return true;

error:
	return false;
}


void FmtSimulation::setModcodAdvertised(tal_id_t tal_id)
{
	map<tal_id_t, StFmtSimu *>::const_iterator st_iter;
	st_iter = this->sts.find(tal_id);
	if(st_iter != this->sts.end())
	{
		(*st_iter).second->setModcodAdvertised();
	}
}



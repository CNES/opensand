/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file StFmtSimu.cpp
 * @brief The internal representation of a Satellite Terminal (ST)
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Adrien Thibaud <athibaud@toulouse.viveris.com>
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 */

#include "StFmtSimu.h"


StFmtSimu::StFmtSimu(string name,
                     tal_id_t id,
                     uint8_t init_modcod_id,
                     const FmtDefinitionTable *const modcod_def):
	id(id),
	modcod_def(modcod_def),
	cni_has_changed(true),
	// the column is the id at beginning
	column(id),
	current_modcod_id(init_modcod_id)
{
	// TODO we should do more specific logs, like here, wherever it's possible
	if(id < BROADCAST_TAL_ID)
	{
		this->log_fmt = Output::registerLog(LEVEL_WARNING,
		                                    "Dvb.Fmt.%sStFmtSimu%u", name.c_str(), id);
	}
	else
	{
		this->log_fmt = Output::registerLog(LEVEL_WARNING,
		                                    "Dvb.Fmt.%sSimuatedStFmtSimu", name.c_str());
	}
}


StFmtSimu::~StFmtSimu()
{
	// nothing particular to do
}


tal_id_t StFmtSimu::getId() const
{
	return this->id;
}


unsigned long StFmtSimu::getSimuColumnNum() const
{
	return (unsigned long) this->column;
}

void StFmtSimu::setSimuColumnNum(unsigned long col)
{
	// use to set default column when there is no column corresponding to id
	this->column = col;
}


uint8_t StFmtSimu::getCurrentModcodId() const
{
	return this->current_modcod_id;
}

void StFmtSimu::updateModcodId(uint8_t new_id,
                               double acm_loop_margin_db /*=0.0*/)
{
	// we check here if MODCOD is decreasgin else, we will never have
	// the highest MODCOD when using FMT simulation file
	// TODO but on the first decrease the margin won't be applied
	if(acm_loop_margin_db != 0.0 and new_id < this->current_modcod_id)
	{
		double cni = this->modcod_def->getRequiredEsN0(this->current_modcod_id);
		this->updateCni(cni, acm_loop_margin_db);
		return;
	}

	if(new_id != this->current_modcod_id)
	{
		this->cni_has_changed = true;
	}
	this->current_modcod_id = new_id;
}

void StFmtSimu::updateCni(double cni,
                          double acm_loop_margin_db)
{
	// TODO we should improve this and only apply if CNI
	//      is deareasing for example (not really satisfying)
	if(acm_loop_margin_db != 0.0)
	{
		LOG(this->log_fmt, LEVEL_INFO,
		    "Terminal %u: apply ACM loop margin (%.2f dB) on new CNI (%.2f dB)\n",
		    id, acm_loop_margin_db, cni);
		cni -= acm_loop_margin_db;
	}
	fmt_id_t modcod_id = this->modcod_def->getRequiredModcod(cni);
	LOG(this->log_fmt, LEVEL_INFO,
	    "Terminal %u: CNI = %.2f dB, corresponding to MODCOD ID %u\n",
	    id, cni, modcod_id);
	this->updateModcodId(modcod_id);
}

double StFmtSimu::getRequiredCni()
{
	double cni = this->modcod_def->getRequiredEsN0(this->current_modcod_id);
	if(cni == 0.0)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Cannot get required CNI for MODCOD %u\n", this->current_modcod_id);
	}
	this->cni_has_changed = false;

	return cni;

}

bool StFmtSimu::getCniHasChanged()
{
	return this->cni_has_changed;
}


//////////////////////////////////////////////////////////////////////////////
////////////////////////         StFmtSimuList          ////////////////////////
////////////////////////////////////////////////////////////////////////////////


StFmtSimuList::StFmtSimuList(string name):
	name(name),
	sts(NULL),
	acm_loop_margin_db(0.0),
	sts_mutex("sts_mutex")
{
	// Output Log
	this->log_fmt = Output::registerLog(LEVEL_WARNING,
	                                    "Dvb.Fmt.%sStFmtSimuList", name.c_str());

	this->sts = new ListStFmt();
}

StFmtSimuList::~StFmtSimuList()
{
	RtLock lock(this->sts_mutex);
	for(ListStFmt::iterator it = this->sts->begin();
	    it != this->sts->end(); it++)
	{
		delete it->second;
	}
	this->sts->clear();
	delete this->sts;
}

void StFmtSimuList::setAcmLoopMargin(double acm_loop_margin_db)
{
	this->acm_loop_margin_db = acm_loop_margin_db;
}

bool StFmtSimuList::addTerminal(tal_id_t st_id, fmt_id_t init_modcod,
                                const FmtDefinitionTable *const modcod_def)
{
	StFmtSimu *new_st;

	if(this->isStPresent(st_id))
	{
		LOG(this->log_fmt, LEVEL_WARNING,
		    "ST%u already exist in FMT simu list, erase it\n", st_id);
		this->delTerminal(st_id);
	}

	// take the lock after checking if ST aleady exists
	RtLock lock(this->sts_mutex);
	LOG(this->log_fmt, LEVEL_DEBUG,
	    "add ST%u in FMT simu list\n", st_id);

	// Create the st
	new_st = new StFmtSimu(this->name, st_id, init_modcod, modcod_def);
	if(!new_st)
	{
		LOG(this->log_fmt, LEVEL_ERROR, "Failed to create a new ST\n");
		return false;
	}

	// insert it
	this->sts->insert(std::make_pair(st_id, new_st));
	this->insert(st_id);

	return true;
}

bool StFmtSimuList::delTerminal(tal_id_t st_id)
{
	RtLock lock(this->sts_mutex);
	ListStFmt::iterator it;

	// find the entry to delete
	it = this->sts->find(st_id);
	if(it == this->sts->end())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "ST with ID %u not found in list of STs\n", st_id);
		return false;
	}

	// delete the ST
	delete it->second;
	this->sts->erase(it);
	this->erase(st_id);

	return true;
}

void StFmtSimuList::updateModcod(const FmtSimulation &fmt_simu)
{
	RtLock lock(this->sts_mutex);
	ListStFmt::iterator it;

	for(it = this->sts->begin(); it != this->sts->end(); ++it)
	{
		StFmtSimu *st;
		tal_id_t st_id;
		tal_id_t column;

		st = it->second;
		st_id = st->getId();
		column = st->getSimuColumnNum();

		LOG(this->log_fmt, LEVEL_DEBUG,
		    "ST with ID %u uses MODCOD ID at column %u\n",
		    st_id, column);

		if(fmt_simu.getModcodList().size() <= column)
		{
			LOG(this->log_fmt, LEVEL_DEBUG,
			    "cannot access MODCOD column %u for ST%u\n"
			    "default MODCOD is used\n",
			    column, st_id);
			column = fmt_simu.getModcodList().size() - 1;
			st->setSimuColumnNum(column);
		}
		// replace the current MODCOD ID by the new one
		st->updateModcodId(atoi(fmt_simu.getModcodList()[column].c_str()),
		                   this->acm_loop_margin_db);

		LOG(this->log_fmt, LEVEL_DEBUG, "new MODCOD ID of ST with ID %u = %u\n",
		    st_id, atoi(fmt_simu.getModcodList()[column].c_str()));
	}
}


void StFmtSimuList::setRequiredCni(tal_id_t st_id, double cni)
{
	RtLock lock(this->sts_mutex);
	ListStFmt::iterator st_iter;


	st_iter = this->sts->find(st_id);
	if(st_iter == this->sts->end())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "ST%u not found, cannot set required CNI\n", st_id);
		return;
	}
	LOG(this->log_fmt, LEVEL_INFO,
	    "set required CNI %.2f for ST%u\n", cni, st_id);

	(*st_iter).second->updateCni(cni, this->acm_loop_margin_db);
}

double StFmtSimuList::getRequiredCni(tal_id_t st_id) const
{
	RtLock lock(this->sts_mutex);
	ListStFmt::iterator st_iter;

	st_iter = this->sts->find(st_id);
	if(st_iter == this->sts->end())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "ST%u not found, cannot get required CNI\n", st_id);
		return 0.0;
	}

	return (*st_iter).second->getRequiredCni();
}


fmt_id_t StFmtSimuList::getCurrentModcodId(tal_id_t st_id) const
{
	RtLock lock(this->sts_mutex);
	ListStFmt::const_iterator st_iter;

	st_iter = this->sts->find(st_id);
	if(st_iter == this->sts->end())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "ST%u not found, cannot get current MODCOD\n", st_id);
		return 0;
	}

	return (*st_iter).second->getCurrentModcodId();
}

bool StFmtSimuList::getCniHasChanged(tal_id_t st_id)
{
	RtLock lock(this->sts_mutex);
	ListStFmt::iterator st_iter;

	st_iter = this->sts->find(st_id);
	if(st_iter == this->sts->end())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "ST%u not found, cannot get CNI status\n", st_id);
		return false;
	}

	return (*st_iter).second->getCniHasChanged();
}

bool StFmtSimuList::isStPresent(tal_id_t st_id) const
{
	RtLock lock(this->sts_mutex);
	set<tal_id_t>::const_iterator it;
	it = std::find(this->begin(),
	               this->end(), st_id);

	return(it != this->end());
}

tal_id_t StFmtSimuList::getTalIdWithLowerModcod() const
{
	RtLock lock(this->sts_mutex);
	ListStFmt::const_iterator st_iterator;
	uint8_t modcod_id;
	uint8_t lower_modcod_id = 0;
	tal_id_t tal_id;
	tal_id_t lower_tal_id = 255;

	for(st_iterator = this->sts->begin();
	    st_iterator != this->sts->end();
	    ++st_iterator)
	{
		// Retrieve the lower modcod
		tal_id = (*st_iterator).first;
		modcod_id = (*st_iterator).second->getCurrentModcodId();

		// TODO:retrieve with lower Es/N0 not modcod_id
		if((st_iterator == this->sts->begin()) ||
		    (modcod_id < lower_modcod_id))
		{
			lower_modcod_id = modcod_id;
			lower_tal_id = tal_id;
		}
	}

	LOG(this->log_fmt, LEVEL_DEBUG,
	    "TAL_ID corresponding to lower modcod: %u\n", lower_tal_id);

	return lower_tal_id;
}





/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 */

#include "StFmtSimu.h"


StFmtSimu::StFmtSimu(tal_id_t id,
                     uint8_t modcod_id):
	id(id),
	// the column is the id at beginning
	column(id),
	current_modcod_id(modcod_id),
	modcod_mutex("StFmtSimu_mutex")
{
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
	RtLock lock(this->modcod_mutex);
	return (unsigned long) this->column;
}

void StFmtSimu::setSimuColumnNum(unsigned long col)
{
	RtLock lock(this->modcod_mutex);
	// use to set default column when there is no column corresponding to id
	this->column = col;
}


uint8_t StFmtSimu::getCurrentModcodId()
{
	RtLock lock(this->modcod_mutex);
	return this->current_modcod_id;
}

void StFmtSimu::updateModcodId(uint8_t new_id)
{
	RtLock lock(this->modcod_mutex);
	this->current_modcod_id = new_id;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////         StFmtSimuList          ////////////////////////
////////////////////////////////////////////////////////////////////////////////


StFmtSimuList::StFmtSimuList():
	sts(NULL),
	sts_mutex("sts_mutex")
{
	// Output Log
	this->log_fmt = Output::registerLog(LEVEL_WARNING,
	                                    "Dvb.Fmt.StFmtSimuList");

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
}


const ListStFmt *StFmtSimuList::getListSts(void) const
{
	RtLock lock(this->sts_mutex);
	return this->sts;
}

bool StFmtSimuList::addTerminal(tal_id_t st_id, fmt_id_t modcod)
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
	new_st = new StFmtSimu(st_id, modcod);
	if(!new_st)
	{
		LOG(this->log_fmt, LEVEL_ERROR, "Failed to create a new ST\n");
		return false;
	}

	// insert it
	this->sts->insert(std::make_pair(st_id, new_st));

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
			LOG(this->log_fmt, LEVEL_WARNING,
			    "cannot access MODCOD column %u for ST%u\n" "defaut MODCOD is used\n",
			    column, st_id);
			column = fmt_simu.getModcodList().size() - 1;
			st->setSimuColumnNum(column);
		}
		// replace the current MODCOD ID by the new one
		st->updateModcodId(atoi(fmt_simu.getModcodList()[column].c_str()));

		LOG(this->log_fmt, LEVEL_DEBUG, "new MODCOD ID of ST with ID %u = %u\n",
		    st_id, atoi(fmt_simu.getModcodList()[column].c_str()));
	}
}

void StFmtSimuList::setRequiredModcod(tal_id_t st_id, fmt_id_t modcod_id)
{
	RtLock lock(this->sts_mutex);
	ListStFmt::iterator st_iter;


	st_iter = this->sts->find(st_id);
	if(st_iter == this->sts->end())
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "ST%u not found, cannot set required MODCOD\n", st_id);
		return;
	}
	LOG(this->log_fmt, LEVEL_INFO,
	    "set required MODCOD %u for ST%u\n", modcod_id, st_id);

	(*st_iter).second->updateModcodId(modcod_id);
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

bool StFmtSimuList::isStPresent(tal_id_t st_id) const
{
	RtLock lock(this->sts_mutex);
	ListStFmt::const_iterator st_iter;

	st_iter = this->sts->find(st_id);
	if(st_iter == this->sts->end())
	{
		// the st is not present
		return false;
	}

	return true;
}


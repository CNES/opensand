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


StFmtSimu::StFmtSimu(long id,
                     uint8_t modcod_id):
id(id),
current_modcod_id(modcod_id),
modcod_mutex("StFmtSimu_mutex")
{
}


StFmtSimu::~StFmtSimu()
{
	// nothing particular to do
}


long StFmtSimu::getId() const
{
	return this->id;
}


unsigned long StFmtSimu::getSimuColumnNum() const
{
	// the column is the id
	return (unsigned long) this->id;
}


uint8_t StFmtSimu::getCurrentModcodId() const
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
sts(),
sts_ids(),
gws_id(),
sts_mutex("sts_mutex")
{
}

StFmtSimuList::~StFmtSimuList()
{
	RtLock lock(this->sts_mutex);
	ListStsPerSpotPerGw::iterator it;

	for(it = this->sts.begin();
	    it != this->sts.end(); it++)
	{
		this->clearListStsPerSpot(it->second);
	}

	this->sts.clear();
	this->sts_ids.clear();
	this->gws_id.clear();
}

ListStsPerSpotPerGw StFmtSimuList::getSts(void) const
{
	RtLock lock(this->sts_mutex);
	return this->sts;
}

ListStsPerSpot* StFmtSimuList::getListStsPerSpot(tal_id_t gw_id) const
{
	// No lock on mutex because this function is private
	// but the mutex should be locked when this fonction is called
	ListStsPerSpotPerGw::const_iterator it;

	it = this->sts.find(gw_id);

	if(it == this->sts.end())
	{
		DFLTLOG(LEVEL_DEBUG, "Gw %u not found in the list\n", gw_id);
		return NULL;
	}

	return it->second;
}

bool StFmtSimuList::setListStsPerSpot(tal_id_t gw_id, ListStsPerSpot *list_sts_per_spot)
{
	RtLock lock(this->sts_mutex);
	ListStsPerSpotPerGw::iterator it;

	it = this->sts.find(gw_id);

	if(it != this->sts.end())
	{
		this->clearListStsPerSpot(it->second);
	}

	it->second = list_sts_per_spot;

	return true;
}

ListSts* StFmtSimuList::getListSts(tal_id_t gw_id, spot_id_t spot_id) const
{
	RtLock lock(this->sts_mutex);
	return this->getListStsPriv(gw_id, spot_id);
}

ListSts* StFmtSimuList::getListStsPriv(tal_id_t gw_id, spot_id_t spot_id) const
{
	// No lock on mutex because this function is private
	// but the mutex should be locked when this fonction is called
	ListStsPerSpot::iterator it;
	ListStsPerSpot* list_sts_per_spot;

	list_sts_per_spot = this->getListStsPerSpot(gw_id);
	if(!list_sts_per_spot)
	{
		return NULL;
	}

	it = list_sts_per_spot->find(spot_id);
	if(it != list_sts_per_spot->end())
	{
		DFLTLOG(LEVEL_ERROR, "Spot %u not found in the list\n", spot_id);
		return NULL;
	}

	return it->second;
}

bool StFmtSimuList::setListSts(tal_id_t gw_id, spot_id_t spot_id, ListSts *list_sts)
{
	RtLock lock(this->sts_mutex);
	return this->setListStsPriv(gw_id, spot_id, list_sts);
}

bool StFmtSimuList::setListStsPriv(tal_id_t gw_id, spot_id_t spot_id, ListSts *list_sts)
{
	// No lock on mutex because this function is private
	// but the mutex should be locked when this fonction is called
	ListStsPerSpotPerGw::iterator it;
	ListStsPerSpot::iterator it2;
	ListStsPerSpot* new_list;
	std::pair<ListStsPerSpotPerGw::iterator,bool> ret;

	it = this->sts.find(gw_id);
	if(it == this->sts.end())
	{
		new_list = new ListStsPerSpot();
		if(!new_list)
		{
			DFLTLOG(LEVEL_ERROR, "Failed to create ListStsPerSpot for Gw %u",
			        gw_id);
			return false;
		}
		ret = this->sts.insert(std::make_pair(gw_id, new_list));
		if(!ret.second)
		{
			DFLTLOG(LEVEL_ERROR, "Insert failed\n");
			return false;
		}
		it = ret.first;
	}

	it2 = it->second->find(spot_id);
	if(it2 != it->second->end())
	{
		this->clearListSts(it2->second);
	}
	it2->second = list_sts;

	return true;
}

bool StFmtSimuList::addTerminal(tal_id_t st_id, uint8_t modcod, tal_id_t gw_id,
                                spot_id_t spot_id)
{
	RtLock lock(this->sts_mutex);
	ListSts* list_sts;
	StFmtSimu *new_st;

	DFLTLOG(LEVEL_DEBUG, "addTerminal, ST %u, GW %u, Spot %u\n", st_id, gw_id, spot_id);

	if(st_id != gw_id)
	{
		// We are adding a st
		std::pair<spot_id_t, tal_id_t> pair = std::make_pair(spot_id, gw_id);
		this->sts_ids.insert(std::make_pair<tal_id_t, std::pair<spot_id_t, tal_id_t> >(st_id, pair));
	}
	this->gws_id.insert(gw_id);

	// Find the good list
	list_sts = this->getListStsPriv(gw_id, spot_id);
	if(!list_sts)
	{
		list_sts = new ListSts();
		if(!list_sts)
		{
			DFLTLOG(LEVEL_ERROR, "Failed to create new list\n");
			return false;
		}
		this->setListStsPriv(gw_id, spot_id, list_sts);
	}

	// Create the st
	new_st = new StFmtSimu(st_id, modcod);
	if(!new_st)
	{
		DFLTLOG(LEVEL_ERROR, "Failed to create a new ST\n");
		return false;
	}

	// insert it
	(*list_sts)[st_id] = new_st;

	return true;
}

bool StFmtSimuList::delTerminal(tal_id_t st_id, tal_id_t gw_id,
                                spot_id_t spot_id)
{
	RtLock lock(this->sts_mutex);
	ListSts* list_sts;
	ListSts::iterator it;

	if(st_id == gw_id)
	{
		// We are deleting a gw
		// Nothing to do, the gw might still be on
		// an other spot
	}
	else
	{
		// We are deleting a st
		this->sts_ids.erase(st_id);
	}

	// Find the good list
	list_sts = this->getListStsPriv(gw_id, spot_id);
	if(!list_sts)
	{
		DFLTLOG(LEVEL_ERROR, "List of Sts not found for spot %u and gw %u\n",
		        spot_id, gw_id);
		return false;
	}

	// find the entry to delete
	it = list_sts->find(st_id);
	if(it == list_sts->end())
	{
		DFLTLOG(LEVEL_ERROR, "ST with ID %u not found in list of STs\n", st_id);
		return false;
	}

	// delete the ST
	delete it->second;
	list_sts->erase(it);

	return true;
}

void StFmtSimuList::updateModcod(tal_id_t gw_id, spot_id_t spot_id,
                                 FmtSimulation* fmt_simu)
{
	RtLock lock(this->sts_mutex);
	ListSts* list;
	ListSts::iterator it;

	// Find the good list
	list = this->getListStsPriv(gw_id, spot_id);
	if(!list)
	{
		DFLTLOG(LEVEL_DEBUG, "List of Sts not found for spot %u and gw %u\n",
		        spot_id, gw_id);
		return;
	}

	for(it = list->begin(); it != list->end(); ++it)
	{
		StFmtSimu *st;
		tal_id_t st_id;
		unsigned long column;

		st = it->second;
		st_id = st->getId();
		column = st->getSimuColumnNum();

		DFLTLOG(LEVEL_DEBUG, "ST with ID %u uses MODCOD ID at column %lu\n",
		        st_id, column);

		if(fmt_simu->getModcodList().size() <= column)
		{
			DFLTLOG(LEVEL_WARNING, "cannot access MODCOD column %lu for ST%u\n"
			        "defaut MODCOD is used\n", column, st_id);
			column = fmt_simu->getModcodList().size() - 1;
		}
		// replace the current MODCOD ID by the new one
		st->updateModcodId(atoi(fmt_simu->getModcodList()[column].c_str()));

		DFLTLOG(LEVEL_DEBUG, "new MODCOD ID of ST with ID %u = %u\n", st_id,
		        atoi(fmt_simu->getModcodList()[column].c_str()));
	}
}

void StFmtSimuList::setRequiredModcod(tal_id_t st_id, uint8_t modcod_id)
{
	RtLock lock(this->sts_mutex);
	ListSts* list;
	ListSts::const_iterator st_iter;
	std::map<tal_id_t, std::pair<spot_id_t, tal_id_t> >::iterator it;
	spot_id_t spot_id;
	tal_id_t gw_id;

	if(this->gws_id.find(st_id) != this->gws_id.end())
	{
		// we have to set the gw's modcod for all the spot
		this->setRequiredModcodGw(st_id, modcod_id);
		return;
	}

	it = this->sts_ids.find(st_id);
	if(it == this->sts_ids.end())
	{
		// the st is not present
		DFLTLOG(LEVEL_ERROR, "St %u is not present\n", st_id);
		return;
	}
	spot_id = it->second.first;
	gw_id = it->second.second;

	// Find the good list
	list = this->getListStsPriv(gw_id, spot_id);
	if(!list)
	{
		DFLTLOG(LEVEL_ERROR, "List of Sts not found for spot %u and gw %u\n",
		        spot_id, gw_id);
		return;
	}

	st_iter = list->find(st_id);
	if(st_iter == list->end())
	{
		DFLTLOG(LEVEL_ERROR, "Sts %u not found\n", st_id);
		return;
	}
	st_iter->second->updateModcodId(modcod_id);
}

void StFmtSimuList::setRequiredModcodGw(tal_id_t gw_id, uint8_t modcod_id)
{
	ListStsPerSpot* list;
	ListStsPerSpot::iterator it;
	ListSts::iterator st_iter;
	spot_id_t spot_id;

	list = this->getListStsPerSpot(gw_id);
	if(!list)
	{
		DFLTLOG(LEVEL_ERROR, "List of Sts not found for gw %u\n", gw_id);
		return;
	}

	for(it = list->begin();
	    it != list->end(); it++)
	{
		st_iter = it->second->find(gw_id);
		spot_id = it->first;
		if(st_iter == it->second->end())
		{
			// Error here, the gw should be found every time
			DFLTLOG(LEVEL_ERROR, "Gw %u not found for spot %u\n",
			        spot_id, gw_id);
			continue;
		}
		st_iter->second->updateModcodId(modcod_id);
	}
}

uint8_t StFmtSimuList::getCurrentModcodId(tal_id_t st_id)
{
	RtLock lock(this->sts_mutex);
	ListSts* list;
	ListSts::const_iterator st_iter;
	std::map<tal_id_t, std::pair<spot_id_t, tal_id_t> >::iterator it;
	spot_id_t spot_id;
	tal_id_t gw_id;

	if(this->gws_id.find(st_id) != this->gws_id.end())
	{
		// the modcod is the same for all the spot
		return this->getCurrentModcodIdGw(st_id);
	}

	it = this->sts_ids.find(st_id);
	if(it == this->sts_ids.end())
	{
		// the st is not present
		DFLTLOG(LEVEL_ERROR, "St %u is not present\n", st_id);
		return 0;
	}
	spot_id = it->second.first;
	gw_id = it->second.second;

	// Find the good list
	list = this->getListStsPriv(gw_id, spot_id);
	if(!list)
	{
		DFLTLOG(LEVEL_ERROR, "List of Sts not found for spot %u and gw %u\n",
		        spot_id, gw_id);
		return 0;
	}

	st_iter = list->find(st_id);
	if(st_iter == list->end())
	{
		DFLTLOG(LEVEL_ERROR, "Sts %u not found\n", st_id);
		return 0;
	}

	return st_iter->second->getCurrentModcodId();
}

uint8_t StFmtSimuList::getCurrentModcodIdGw(tal_id_t gw_id)
{
	// No lock on mutex because this function is private
	// but the mutex should be locked when this fonction is called
	ListStsPerSpot* list;
	ListStsPerSpot::iterator it;
	ListSts::iterator gw_iter;

	list = this->getListStsPerSpot(gw_id);
	if(!list)
	{
		DFLTLOG(LEVEL_ERROR, "List of Sts not found for gw %u\n", gw_id);
		return 0;
	}

	it = list->begin();
	if(it == list->end())
	{
		DFLTLOG(LEVEL_ERROR, "No spot found\n");
		return 0;
	}
	gw_iter = it->second->find(gw_id);
	return gw_iter->second->getCurrentModcodId();
}

bool StFmtSimuList::isStPresent(tal_id_t st_id)
{
	RtLock lock(this->sts_mutex);
	std::map<tal_id_t, std::pair<spot_id_t, tal_id_t> >::iterator it;

	it = this->sts_ids.find(st_id);
	if(it == this->sts_ids.end())
	{
		// the st is not present
		return false;
	}

	return true;
}

void StFmtSimuList::clearListStsPerSpot(ListStsPerSpot* list)
{
	// No lock on mutex because this function is private
	// but the mutex should be locked when this fonction is called
	ListStsPerSpot::iterator it;

	if(!list)
	{
		// Nothing to do
		return;
	}

	for(it = list->begin();
	    it != list->end(); it++)
	{
		this->clearListSts(it->second);
	}

	list->clear();

}

void StFmtSimuList::clearListSts(ListSts* list)
{
	// No lock on mutex because this function is private
	// but the mutex should be locked when this fonction is called
	ListSts::iterator it;

	if(!list)
	{
		// Nothing to do
		return;
	}

	for(it = list->begin();
	    it != list->end(); it++)
	{
		delete it->second;
	}

	list->clear();
}


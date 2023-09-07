/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file SarpTable.cpp
 * @brief SARP table
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <opensand_output/Output.h>

#include "SarpTable.h"
#include "MacAddress.h"


SarpTable::SarpTable(unsigned int max_entries):
	eth_sarp{}
{
	this->max_entries = (max_entries == 0 ? SarpTable::SARP_MAX : max_entries);

	// Output Log
	this->log_sarp = Output::Get()->registerLog(LEVEL_WARNING, "Lan_Adaptation.SarpTable");
}


SarpTable::~SarpTable()
{
}


bool SarpTable::add(std::unique_ptr<MacAddress> mac_address, tal_id_t tal)
{
	LOG(this->log_sarp, LEVEL_INFO,
	    "add new entry in SARP table (%s)\n",
	    mac_address->str().c_str());

	if((this->eth_sarp.size() >= this->max_entries) || mac_address == nullptr)
	{
		LOG(this->log_sarp, LEVEL_ERROR,
		    "SARP table full or address is empry, "
		    "cannot add entry\n");
		return false;
	}

	// add entry to if not presents
	tal_id_t tal_id = 255;
	if(!SarpTable::getTalByMac(*mac_address, tal_id))
	{
		// set entry
		this->eth_sarp.push_back({std::move(mac_address), tal});
	}	

	return true;
}


bool SarpTable::getTalByMac(const MacAddress &mac_address, tal_id_t &tal_id) const
{
	tal_id = this->default_dest;

	for(auto&& entry : this->eth_sarp)
	{
		if(entry.mac->matches(mac_address))
		{
			tal_id = entry.tal_id;
			return true;
		}
	}

	return false;
}


bool SarpTable::getMacByTal(tal_id_t tal_id, std::vector<MacAddress> &mac_address) const
{
	for(auto&& entry : this->eth_sarp)
	{
		if(entry.tal_id == tal_id)
		{
			mac_address.emplace_back(entry.mac->str());
			return true;
		}
	}

	return false;
}


void SarpTable::setDefaultTal(tal_id_t dflt)
{
	this->default_dest = dflt;
}

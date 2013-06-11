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
 * @file SarpTable.cpp
 * @brief SARP table
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "SarpTable.h"

// debug
#define DBG_PACKAGE PKG_QOS_DATA
#include "opensand_conf/uti_debug.h"

#include <algorithm>
#include <vector>

// max_entries = SARP_MAX by default
SarpTable::SarpTable(unsigned int max_entries):
	ip_sarp(),
	eth_sarp()
{
	this->max_entries = (max_entries == 0 ? SARP_MAX : max_entries);
}

SarpTable::~SarpTable()
{
	for(list<sarpIpEntry *>::iterator it = this->ip_sarp.begin();
	    it != this->ip_sarp.end(); ++it)
	{
		if((*it)->ip != NULL)
			delete (*it)->ip;
		delete *it;
	}
	for(list<sarpEthEntry *>::iterator it = this->eth_sarp.begin();
	    it != this->eth_sarp.end(); ++it)
	{
		if((*it)->mac != NULL)
			delete (*it)->mac;
		delete *it;
	}
}

bool SarpTable::add(IpAddress *ip_addr, unsigned int mask_len,
                    tal_id_t tal)
{
	bool success = true;
	sarpIpEntry *entry;

	UTI_DEBUG("add new entry in SARP table (%s/%u)\n",
	          ip_addr->str().c_str(), mask_len);

	if((this->ip_sarp.size() >= this->max_entries) || ip_addr == NULL)
	{
		UTI_ERROR("SARP table full, cannot add entry\n");
		success = false;
		goto quit;
	}

	// get memory for a new entry
	entry = new sarpIpEntry;
	if(!entry)
	{
		// no more memory in the pool
		UTI_ERROR("cannot get memory from the SARP memory pool, "
		          "cannot add entry\n");
		success = false;
		goto quit;
	}

	// set entry
	entry->ip = ip_addr;
	entry->mask_len = mask_len;
	entry->tal_id = tal;

	// append entry to table
	this->ip_sarp.push_back(entry);

quit:
	return success;
}

bool SarpTable::add(MacAddress *mac_address,
                    tal_id_t tal)
{
	bool success = true;
	sarpEthEntry *entry;

	UTI_DEBUG("add new entry in SARP table (%s)\n",
	          mac_address->str().c_str());

	if((this->eth_sarp.size() >= this->max_entries) || mac_address == NULL)
	{
		UTI_ERROR("SARP table full or address is empry, "
		          "cannot add entry\n");
		success = false;
		goto quit;
	}

	// get memory for a new entry
	entry = new sarpEthEntry;
	if(!entry)
	{
		// no more memory
		UTI_ERROR("cannot get memory for an Ethernet SARP entry, "
		          "cannot add entry\n");
		success = false;
		goto quit;
	}

	// set entry
	entry->mac = mac_address;
	entry->tal_id = tal;

	// append entry to table
	this->eth_sarp.push_back(entry);

quit:
	return success;
}

bool SarpTable::getTalByIp(IpAddress *ip, tal_id_t &tal_id) const
{
	sarpIpEntry *entry;
	unsigned int max_mask_len;

	max_mask_len = 0;
	tal_id = this->default_dest; // if no set (-1) this will lead to an error

	// search IP matching with longer mask
	list <sarpIpEntry *>::const_iterator it;

	for(it = this->ip_sarp.begin(); it != this->ip_sarp.end(); it++)
	{
		entry = *it;
		if(entry->ip->matchAddressWithMask(ip, entry->mask_len))
		{
			if(entry->mask_len >= max_mask_len)
			{
				max_mask_len = entry->mask_len;
				tal_id = entry->tal_id;
				return true;
			}
		}
	}

	return false;
}

bool SarpTable::getTalByMac(MacAddress mac_address, tal_id_t &tal_id) const
{
	sarpEthEntry *entry;

	tal_id = this->default_dest; // if no set (-1) this will lead to an error

	list <sarpEthEntry *>::const_iterator it;
	for(it = this->eth_sarp.begin(); it != this->eth_sarp.end(); it++)
	{
		entry = *it;
		if(entry->mac->matches(&mac_address))
		{
			tal_id = entry->tal_id;
			return true;
		}
	}

	return false;
}

bool SarpTable::getMacByTal(tal_id_t tal_id, vector<MacAddress> &mac_address) const
{
	// TODO at the moment we got the first MAC with corresponding tal_id but
	// it could be the MAC of a workstation, add something in table to know if
	// it is the terminal
	sarpEthEntry *entry;

	list <sarpEthEntry *>::const_iterator it;
	for(it = this->eth_sarp.begin(); it != this->eth_sarp.end(); it++)
	{
		entry = *it;
		if(entry->tal_id == tal_id)
		{
			mac_address.push_back(MacAddress(entry->mac->str()));
			return true;
		}
	}

	return false;
}

void SarpTable::setDefaultTal(tal_id_t dflt)
{
	this->default_dest = dflt;
}



/**
 * @file SarpTable.cpp
 * @brief SARP table
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "SarpTable.h"

// debug
#define DBG_PACKAGE PKG_QOS_DATA
#include "platine_conf/uti_debug.h"


// max_entries = SARP_MAX by default
SarpTable::SarpTable(unsigned int max_entries): std::list < sarpEntry * >()
{
	this->setMaxEntries(max_entries);

	this->memory_pool.allocate(sizeof(sarpEntry), this->max_entries);
	this->memory_pool.setName("sarp_entry");
}

SarpTable::~SarpTable()
{
	std::list < sarpEntry * >::iterator it;

	for(it = this->begin(); it != this->end(); it++)
	{
		if((*it)->ip != NULL)
			delete (*it)->ip;
		this->memory_pool.release((char *) (*it));
	}
}

void SarpTable::setMaxEntries(unsigned int max_entries)
{
	this->max_entries = (max_entries == 0 ? SARP_MAX : max_entries);
}

bool SarpTable::add(IpAddress *ip_addr, unsigned int mask_len,
                   unsigned long mac_addr, unsigned int tal)
{
	const char *FUNCNAME = "[SarpTable::add]";
	bool success = true;
	sarpEntry *entry;

	UTI_DEBUG("%s add new entry in SARP table (%s/%u -> %lu)\n", FUNCNAME,
	          ip_addr->str().c_str(), mask_len, mac_addr);

	if(this->isFull() || ip_addr == NULL)
	{
		UTI_ERROR("%s SARP table full, cannot add entry\n", FUNCNAME);
		success = false;
		goto quit;
	}

	// get memory for a new entry
	entry = (sarpEntry *) this->memory_pool.get();
	if(!entry)
	{
		// no more memory in the pool
		UTI_ERROR("%s cannot get memory from the SARP memory pool, "
		          "cannot add entry\n", FUNCNAME);
		success = false;
		goto quit;
	}

	// set entry
	entry->ip = ip_addr;
	entry->mask_len = mask_len;
	entry->mac = mac_addr;
	entry->tal_id = tal;

	// append entry to table
	this->push_back(entry);

quit:
	return success;
}

bool SarpTable::isFull()
{
	return (this->length() >= this->max_entries);
}

unsigned int SarpTable::length()
{
	return this->size();
}

long SarpTable::getMacByIp(IpAddress *ip)
{
	sarpEntry *entry;
	unsigned int max_mask_len;
	long mac_addr;

	max_mask_len = 0;
	mac_addr = -1;

	// search IP matching with longer mask
	std::list < sarpEntry * >::iterator it;

	for(it = this->begin(); it != this->end(); it++)
	{
		entry = *it;
		if(entry->ip->matchAddressWithMask(ip, entry->mask_len))
		{
			if(entry->mask_len >= max_mask_len)
			{
				max_mask_len = entry->mask_len;
				mac_addr = entry->mac;
			}
		}
	}

	return mac_addr;
}

int SarpTable::getTalByIp(IpAddress *ip)
{
	sarpEntry *entry;
	unsigned int max_mask_len;
	int tal;

	max_mask_len = 0;
	tal = -1;

	// search IP matching with longer mask
	std::list < sarpEntry * >::iterator it;

	for(it = this->begin(); it != this->end(); it++)
	{
		entry = *it;
		if(entry->ip->matchAddressWithMask(ip, entry->mask_len))
		{
			if(entry->mask_len >= max_mask_len)
			{
				max_mask_len = entry->mask_len;
				tal = entry->tal_id;
			}
		}
	}

	return tal;
}


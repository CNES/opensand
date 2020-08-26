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
 * @file SarpTable.h
 * @brief SARP table
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef SARP_TABLE_H
#define SARP_TABLE_H

#include "MacAddress.h"
#include "OpenSandCore.h"

#include <opensand_output/OutputLog.h>

#include <list>
#include <vector>

using std::list;
using std::vector;

/// SARP table entry for Ethernet
typedef struct
{
	MacAddress *mac;
	tal_id_t tal_id;
} sarpEthEntry;

#define SARP_MAX 50

/**
 * @class SarpTable
 * @brief SARP table
 */
class SarpTable
{
 private:

	unsigned int max_entries;    ///< maximum number of entries in SARP table
	// TODO we have only one of these two list that is used each time so we
	//      need only one of these
	list<sarpEthEntry *> eth_sarp; ///< The Ethernet entries in SARP table
	tal_id_t default_dest;  ///< the default terminal ID if no entry is found

 protected:
	// Output Log
  std::shared_ptr<OutputLog> log_sarp;	

 public:

	/**
	 * Build a SARP table
	 *
	 * @param max_entries the maximum number of entries in the table
	 *                    SARP_MAX is the default value
	 */
	SarpTable(unsigned int max_entries = SARP_MAX);

	/**
	 * Destroy the SARP table
	 */
	~SarpTable();

	/**
	 * Add an Ethernet entry in the SARP table
	 *
	 * @param max_addr  the MAC address for the SARP entry
	 * @param tal the tal ID associated with the IP address
	 * @return true if the entry was successfully added (the table was
	 *         not full), false otherwise
	 */
	bool add(MacAddress *mac_address, tal_id_t tal);

	/**
	 * Get the tal ID associated with the MAC address in the SARP table
	 *
	 * @param mac     the MAC address to search for
	 * @param tal_id  the tal ID associated with the MAC address if found
	 *                the default tal_id otherwise (false will be returned)
	 * @return true on success, false otherwise
	 */
	bool getTalByMac(MacAddress mac_address, tal_id_t &tal_id) const;

	/**
	 * Get the MAC address associated with the terminal ID in the SARP table
	 *
	 * @param tal_id  the tal ID to seach for
	 * @param mac     the MAC address associated to the terminal ID if found
	 * @return true on success, false otherwise
	 */
	bool getMacByTal(tal_id_t tal_id, vector<MacAddress> &mac_address) const;

	/**
	 * @brief Set the default destination terminal if no entry is found
	 *
	 * @param dlft  the default terminal ID
	 */
	void setDefaultTal(tal_id_t dflt);

};

#endif

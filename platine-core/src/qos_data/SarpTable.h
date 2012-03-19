/**
 * @file SarpTable.h
 * @brief SARP table
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef SARP_TABLE_H
#define SARP_TABLE_H

#include <list>

#include "platine_margouilla/mgl_memorypool.h"

#include <IpAddress.h>


/// SARP table entry
typedef struct
{
	IpAddress *ip;
	unsigned int mask_len;
	unsigned long mac;
	unsigned int tal_id;
} sarpEntry;

#define SARP_MAX 50

/**
 * @class SarpTable
 * @brief SARP table
 */
class SarpTable: public std::list< sarpEntry * >
{
 private:

	mgl_memory_pool memory_pool; ///< memory pool for table entries
	unsigned int max_entries;    ///< maximum number of entries in SARP table

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
	 * Set the maximum number of entries in the SARP table
	 *
	 * @param max_entries the maximum number of entries in the table
	 */
	void setMaxEntries(unsigned int max_entries);

	/**
	 * Add an entry in the SARP table
	 *
	 * @param ip_addr  the IP address for the SARP entry
	 * @param mask_len the mask length of the IP address
	 * @param mac_addr the MAC address associated with the IP address
	 * @param tal the tal ID associated with the IP address
	 * @return true if the entry was successfully added (the table was
	 *         not full), false otherwise
	 */
	bool add(IpAddress *ip_addr, unsigned int mask_len,
	         unsigned long mac_addr, unsigned int tal);

	/**
	 * Is the SARP table full?
	 *
	 * @return true if the SARP table is full (no space left for additional
	 *         entries), false otherwise
	 */
	bool isFull();

	/**
	 * Get the number of entries in the table
	 *
	 * @return the number of entries in the table
	 */
	unsigned int length();

	/**
	 * Get the MAC address associated with the IP address in the SARP table
	 *
	 * @param ip the IP address to search for
	 * @return the MAC address associated with the IP address if found,
	 *         -1 otherwise
	 */
	long getMacByIp(IpAddress *ip);

	/**
	 * Get the tal ID associated with the IP address in the SARP table
	 *
	 * @param ip the IP address to search for
	 * @return the tal ID associated with the IP address if found,
	 *         -1 otherwise
	 */
	int getTalByIp(IpAddress *ip);

};

#endif

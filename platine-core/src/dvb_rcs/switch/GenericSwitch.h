/**
 * @file GenericSwitch.h
 * @brief Generic switch for Satellite Emulator (SE)
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef GENERIC_SWITCH__H
#define GENERIC_SWITCH__H

#include <NetPacket.h>
#include <map>

/**
 * @class GenericSwitch
 * @brief Generic switch for Satellite Emulator (SE)
 */
class GenericSwitch
{
 protected:

	/// The switch table: association between a terminal id and a
	/// satellite spot ID
	std::map < long, long > switch_table;

 public:

	/**
	 * Build a generic switch
	 */
	GenericSwitch();

	/**
	 * Destroy the generic switch
	 */
	virtual ~GenericSwitch();

	/**
	 * Add an entry in the switch table
	 *
	 * @param tal_id  the satellite terminal
	 * @param spot_id the satellite spot associated with the terminal
	 * @return true if entry was successfully added, false otherwise
	 */
	bool add(long tal_id, long spot_id);

	/**
	 * Find the satellite spot to send the packet to
	 *
	 * @param packet the encapsulation packet to send
	 * @return the satellite spot ID to send the packet to
	 */
	virtual long find(NetPacket *packet) = 0;
};

#endif


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file    Slot.h
 * @brief   Represent a RCS slot in a carrier
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _SLOT_H_
#define _SLOT_H_

#include "SlottedAlohaPacketData.h"

/**
 * @class Slot
 * @brief Represent a RCS slot in a carrier (i.e. a list of packets + attributes)
 */
class Slot: public saloha_packets_data_t
{
 public:

	/**
	 * @brief  Construct a group of carriers with the same characteristics
	 *
	 * @param  carriers_id   The carrier ID to which the slot belongs
	 * @param  slot_if       The slot ID
	 */
	Slot(unsigned int carriers_id,
	     unsigned int slot_id);

	/** Destructor */
	virtual ~Slot();

	/**
	 * @brief   Get carriers Id.
	 *
	 * @return  carriers Id.
	 */
	unsigned int getCarriersId() const;

	/**
	 * @brief   Get slot id
	 *
	 * @return  the slot id
	 */
	unsigned int getId() const;

	/**
	 * @brief Get the number of packets received on this slot
	 *
	 * @return the number of packets received
	 */
	unsigned int getNbrPackets(void) const;

	/**
	 * @brief Release all packets in slot
	 */
	void release(void);

 private:

	/** Carrier id */
	unsigned int carriers_id;

	/** Slot id */
	unsigned int slot_id;
};


#endif



/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file    CarriersGroupSaloha.h
 * @brief   Represent a group of carriers with the same characteristics
 *          for Slotted Aloha
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _CARRIERS_GROUP_SALOHA_H_
#define _CARRIERS_GROUP_SALOHA_H_

#include "CarriersGroup.h"

#include "Slot.h"

/**
 * @class CarriersGroupSaloha
 * @brief Represent a group of carriers with the same characteristics
 *        for Slotted Aloha
 */
class CarriersGroupSaloha: public CarriersGroup
{
public:
	/**
	 * @brief  Construct a group of carriers with the same characteristics
	 *
	 * @param  carriers_id     The carriers ID
	 * @param  fmt_group       The FMT group
	 * @param  ratio           The estimated occupation ratio
	 * @param  rate_syms       The symbol rate (sym/s)
	 * @param  access_type     The carriers access type
	 */
	CarriersGroupSaloha(unsigned int carriers_id,
	                    std::shared_ptr<const FmtGroup> fmt_group,
	                    unsigned int ratio,
	                    rate_symps_t rate_symps,
	                    AccessType access_type);

	/** Destructor */
	virtual ~CarriersGroupSaloha();

	/**
	 * @brief Set the slots number per carrier
	 *
	 * @param slots_nbr  The number of slots
	 * @param last       The last spot ID in the terminal category
	 */
	void setSlotsNumber(unsigned int slots_nbr,
	                    unsigned int last);

	/**
	 * @brief Get the slots number for the entire carriers group
	 *
	 * @return the slots number
	 */
	unsigned int getSlotsNumber(void) const;

	/**
	 * @brief Get the slots
	 *
	 * @return the slots
	 */
	const std::map<unsigned int, std::shared_ptr<Slot>> &getSlots() const;

private:
	/** The slots */
	std::map<unsigned int, std::shared_ptr<Slot>> slots;
};


#endif

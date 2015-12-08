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
 * @file    CarriersGroupSaloha.cpp
 * @brief   Represent a group of carriers with the same characteristics
 *          for Slotted Aloha
 * @author  Julien Bernard / Viveris Technologies
 */


#include "CarriersGroupSaloha.h"

#include "Slot.h"

#include <opensand_output/Output.h>

// TODO when receiving a frame, we do not know from wich carriers group it comes
//      and we can deduce TerminalCategory from source tal_id so we need to
//      handle frames at category level
//      An idea to know from wich carriers frames come: create one UDP
//      channel per carriers_group

CarriersGroupSaloha::CarriersGroupSaloha(unsigned int carriers_id,
                                         const FmtGroup *const fmt_group,
                                         unsigned int ratio,
                                         rate_symps_t symbol_rate_symps,
                                         access_type_t access_type):
	CarriersGroup(carriers_id, fmt_group, ratio, symbol_rate_symps, access_type),
	slots()
{
}

CarriersGroupSaloha::~CarriersGroupSaloha()
{
	for(map<unsigned int, Slot *>::iterator it = this->slots.begin();
	    it != this->slots.end(); ++it)
	{
		delete (*it).second;
	}
	this->slots.clear();
}

void CarriersGroupSaloha::setSlotsNumber(time_ms_t frame_duration_ms,
                                         vol_bytes_t packet_length_bytes,
                                         unsigned int last_id)
{
	unsigned int slot_nbr;
	rate_kbps_t rate_kbps;
	const FmtDefinitionTable *fmt_def = this->fmt_group->getModcodDefinitions();

	// there should be only one FMT ID here
	rate_kbps = fmt_def->symToKbits(this->fmt_group->getFmtIds().front(),
	                                this->symbol_rate_symps);
	slot_nbr = (rate_kbps * frame_duration_ms) / (packet_length_bytes * 8);
	// create the slots, we do as if all spots in a category were on
	// a single carrier
	for(unsigned int i = last_id;
	    i < last_id + slot_nbr * this->carriers_number;
	    i++)
	{
		Slot *slot = new Slot(this->carriers_id, i);
		this->slots[i] = slot;
	}
}

unsigned int CarriersGroupSaloha::getSlotsNumber(void) const
{
	return this->slots.size();
}

map<unsigned int, Slot *> CarriersGroupSaloha::getSlots(void) const
{
	return this->slots;
}

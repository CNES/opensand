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
 * @file    TerminalCategorySaloha.cpp
 * @brief   Represent a Terminal Category for Slotted Aloha
 * @author  Julien Bernard / Viveris Technologies
 */


#include "TerminalCategorySaloha.h"

#include "CarriersGroupSaloha.h"

#include <algorithm>


TerminalCategorySaloha::TerminalCategorySaloha(const std::string& label, AccessType access_type):
	TerminalCategory<CarriersGroupSaloha>{label, access_type},
	accepted_packets{},
	received_packets_nbr{0}
{
}


TerminalCategorySaloha::~TerminalCategorySaloha()
{
	this->accepted_packets.clear();
}


void TerminalCategorySaloha::computeSlotsNumber(UnitConverter &converter)
{
	unsigned int total = 0;
	unsigned int last = 0;

	for (auto &&carriers: this->carriers_groups)
	{
		unsigned int slots_nbr = 0;

		if(carriers.getFmtGroup()->getFmtIds().size() > 0)
		{
			fmt_id_t fmt_id = carriers.getFmtGroup()->getFmtIds().front();
			try
			{
				FmtDefinition &fmt_def = carriers.getFmtGroup()->getModcodDefinitions().getDefinition(fmt_id);
				converter.setModulationEfficiency(fmt_def.getModulationEfficiency());
				slots_nbr = converter.getSlotsNumber(carriers.getSymbolRate());
			}
			catch (const std::range_error&)
			{
				// Silence errors and keep slot_nbr at 0
			}
		}
		carriers.setSlotsNumber(slots_nbr, last);
		total += carriers.getSlotsNumber();
		last = total;
	}
}

unsigned int TerminalCategorySaloha::getSlotsNumber() const
{
	unsigned int total = 0;
	for (auto &&carriers: this->carriers_groups)
	{
		total += carriers.getSlotsNumber();
	}
	return total;
}

std::map<unsigned int, std::shared_ptr<Slot>> TerminalCategorySaloha::getSlots() const
{
	std::map<unsigned int, std::shared_ptr<Slot>> slots;
	for (auto &&carriers: this->carriers_groups)
	{
		const std::map<unsigned int, std::shared_ptr<Slot>> &sl = carriers.getSlots();

		slots.insert(sl.begin(), sl.end());
	}
	return slots;
}

saloha_packets_data_t &TerminalCategorySaloha::getAcceptedPackets()
{
	return this->accepted_packets;
}

void TerminalCategorySaloha::increaseReceivedPacketsNbr()
{
	this->received_packets_nbr++;
}

unsigned int TerminalCategorySaloha::getReceivedPacketsNbr() const
{
	return this->received_packets_nbr;
}

void TerminalCategorySaloha::resetReceivedPacketsNbr()
{
	this->received_packets_nbr = 0;
}


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
	accepted_packets{nullptr},
	received_packets_nbr{0}
{
	this->accepted_packets = new saloha_packets_data_t();
}

TerminalCategorySaloha::~TerminalCategorySaloha()
{
	this->accepted_packets->clear();
	delete this->accepted_packets;
}

void TerminalCategorySaloha::computeSlotsNumber(UnitConverter *converter)
{
	unsigned int total = 0;
	unsigned int last = 0;

	for(std::vector<CarriersGroupSaloha *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		CarriersGroupSaloha *carriers = *it;
		FmtDefinition *fmt_def = NULL;
		unsigned int slots_nbr = 0;

		if(carriers->getFmtGroup()->getFmtIds().size() > 0)
		{
			fmt_id_t fmt_id = carriers->getFmtGroup()->getFmtIds().front();
			fmt_def = carriers->getFmtGroup()->getModcodDefinitions()->getDefinition(fmt_id);
		}
		if(fmt_def)
		{
			converter->setModulationEfficiency(fmt_def->getModulationEfficiency());
			slots_nbr = converter->getSlotsNumber(carriers->getSymbolRate());
		}
		carriers->setSlotsNumber(slots_nbr, last);
		total += carriers->getSlotsNumber();
		last = total;
	}
}

unsigned int TerminalCategorySaloha::getSlotsNumber(void) const
{
	unsigned int total = 0;
	for(std::vector<CarriersGroupSaloha *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		CarriersGroupSaloha *carriers = *it;
		total += carriers->getSlotsNumber();
	}
	return total;
}

std::map<unsigned int, Slot *> TerminalCategorySaloha::getSlots(void) const
{
	std::map<unsigned int, Slot *> slots;
	for(std::vector<CarriersGroupSaloha *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		CarriersGroupSaloha *carriers = *it;
		std::map<unsigned int, Slot *> sl = carriers->getSlots();

		slots.insert(sl.begin(), sl.end());
	}
	return slots;
}

saloha_packets_data_t *TerminalCategorySaloha::getAcceptedPackets(void)
{
	return this->accepted_packets;
}

void TerminalCategorySaloha::increaseReceivedPacketsNbr(void)
{
	this->received_packets_nbr++;
}

unsigned int TerminalCategorySaloha::getReceivedPacketsNbr(void) const
{
	return this->received_packets_nbr;
}

void TerminalCategorySaloha::resetReceivedPacketsNbr(void)
{
	this->received_packets_nbr = 0;
}


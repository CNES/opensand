/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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


TerminalCategorySaloha::TerminalCategorySaloha(string label, access_type_t access_type):
	TerminalCategory<CarriersGroupSaloha>(label, access_type),
	accepted_packets(NULL),
	received_packets_nbr(0)
{
	this->accepted_packets = new saloha_packets_data_t();
}

TerminalCategorySaloha::~TerminalCategorySaloha()
{
	for(saloha_packets_data_t::iterator it = this->accepted_packets->begin();
	    it != this->accepted_packets->end(); ++it)
	{
		delete *it;
	}
	delete this->accepted_packets;
}

void TerminalCategorySaloha::setSlotsNumber(time_ms_t frame_duration_ms,
                                            vol_bytes_t packet_length_bytes)
{
	unsigned int total = 0;
	unsigned int last = 0;
	for(vector<CarriersGroupSaloha *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		CarriersGroupSaloha *carriers = *it;

		carriers->setSlotsNumber(frame_duration_ms, packet_length_bytes, last);
		total += carriers->getSlotsNumber();
		last = total;
	}
}

unsigned int TerminalCategorySaloha::getSlotsNumber(void) const
{
	unsigned int total = 0;
	for(vector<CarriersGroupSaloha *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		CarriersGroupSaloha *carriers = *it;
		total += carriers->getSlotsNumber();
	}
	return total;
}

map<unsigned int, Slot *> TerminalCategorySaloha::getSlots(void) const
{
	map<unsigned int, Slot *> slots;
	for(vector<CarriersGroupSaloha *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		CarriersGroupSaloha *carriers = *it;
		map<unsigned int, Slot *> sl = carriers->getSlots();

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


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file    TerminalCategory.cpp
 * @brief   Represent a Terminal Category
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */


#include "TerminalCategory.h"

#include <opensand_output/Output.h>

#include <algorithm>


TerminalCategory::TerminalCategory(string label):
	terminals(),
	carriers_groups(),
	label(label)
{
	// Output log
	this->log_terminal_category = Output::registerLog(LEVEL_WARNING,
	                                                  "Dvb.Ncc.Band");
}

TerminalCategory::~TerminalCategory()
{
	for(vector<CarriersGroup *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		delete *it;
	}
	// do not delete terminals, they will be deleted in DAMA
}

string TerminalCategory::getLabel() const
{
	return this->label;
}


double TerminalCategory::getWeightedSum() const
{
	// Compute weighted sum in ks/s since available bandplan is in kHz
	double weighted_sum_ksymps = 0.0;
	for(vector<CarriersGroup *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		// weighted_sum = ratio * Rs (ks/s)
		weighted_sum_ksymps += (*it)->getRatio() * (*it)->getSymbolRate() / 1E3;
	}
	return weighted_sum_ksymps;
}


unsigned int TerminalCategory::getRatio() const
{
	unsigned int ratio = 0;
	for(vector<CarriersGroup *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		ratio += (*it)->getRatio();
	}
	return ratio;
}


void TerminalCategory::updateCarriersGroups(unsigned int carriers_number,
                                            time_ms_t superframe_duration_ms)
{
	unsigned int total_ratio = this->getRatio();
	if(carriers_number < this->carriers_groups.size())
	{
		Output::sendLog(this->log_terminal_category, LEVEL_NOTICE, 
		                "Not enough carriers for category %s that contains %zu "
		                "groups. Increase carriers number to the number of "
		                "groups\n",
		    this->label.c_str(), this->carriers_groups.size());
		carriers_number = this->carriers_groups.size();
	}
	for(vector<CarriersGroup *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		unsigned int number;
		vol_sym_t capacity_sym;

		// get number per carriers from total number in category
		number = ceil(carriers_number * (*it)->getRatio() / total_ratio);
		(*it)->setCarriersNumber(number);
		Output::sendLog(this->log_terminal_category, LEVEL_NOTICE, 
		                "Carrier group %u: number of carriers %u\n",
		                (*it)->getCarriersId(), number);

		// get the capacity of the carriers
		capacity_sym = floor((*it)->getSymbolRate() * superframe_duration_ms / 1000);
		(*it)->setCapacity(capacity_sym);
		Output::sendLog(this->log_terminal_category, LEVEL_NOTICE, 
		                "Carrier group %u: capacity for Symbol Rate %.2E: %u "
		                "symbols\n", (*it)->getCarriersId(),
		                (*it)->getSymbolRate(), capacity_sym);
	}
}

void TerminalCategory::addTerminal(TerminalContext *terminal)
{
	terminal->setCurrentCategory(this->label);
	this->terminals.push_back(terminal);
}

bool TerminalCategory::removeTerminal(TerminalContext *terminal)
{
	vector<TerminalContext *>::iterator terminal_it
	                                    = this->terminals.begin();
	const tal_id_t tal_id = terminal->getTerminalId();
	while(terminal_it != this->terminals.end()
		  && (*terminal_it)->getTerminalId() != tal_id)
	{
		++terminal_it;
	}

	if(terminal_it != this->terminals.end())
	{
		this->terminals.erase(terminal_it);
	}
	else
	{
		Output::sendLog(this->log_terminal_category, LEVEL_ERROR, 
		                "ST#%u not registered on category %s",
		                tal_id, this->label.c_str());
		return false;
	}
	return true;
}

void TerminalCategory::addCarriersGroup(unsigned int carrier_id,
                                        const FmtGroup *const fmt_group,
                                        unsigned int ratio,
                                        rate_symps_t symbol_rate_symps)
{
	CarriersGroup *group = new CarriersGroup(carrier_id, fmt_group,
	                                         ratio, symbol_rate_symps);
	this->carriers_groups.push_back(group);
}

vector<CarriersGroup *> TerminalCategory::getCarriersGroups() const
{
	return this->carriers_groups;
}

unsigned int TerminalCategory::getCarriersNumber() const
{
	unsigned int carriers_number = 0;
	for(vector<CarriersGroup *>::const_iterator it = this->carriers_groups.begin();
	    it != this->carriers_groups.end(); ++it)
	{
		carriers_number += (*it)->getCarriersNumber();
	}
	return carriers_number;
}

vector<TerminalContext *> TerminalCategory::getTerminals() const
{
	return this->terminals;
}



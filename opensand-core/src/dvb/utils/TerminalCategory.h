/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file    TerminalCategory.h
 * @brief   Represent a category of terminal
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _TERMINAL_CATEGORY_H
#define _TERMINAL_CATEGORY_H

#include "OpenSandCore.h"
#include "TerminalContext.h"
#include "CarriersGroup.h"

#include <opensand_output/Output.h>

#include <string>
#include <map>
#include <vector>

using std::string;
using std::map;
using std::vector;


/**
 * @class TerminalCategory
 * @brief Template for a category of terminal.
 */
template<class T = CarriersGroup>
class TerminalCategory
{

 public:

	/**
	 * @brief  Create a terminal category.
	 *
	 * @param  label           label of the category.
	 * @param  desired_access  the access type we support for our carriers
	 */
	TerminalCategory(string label, access_type_t desired_access):
		terminals(),
		carriers_groups(),
		desired_access(desired_access),
		label(label),
		other_carriers()
	{
		// Output log
		this->log_terminal_category = Output::registerLog(LEVEL_WARNING,
		                                                  "Dvb.Ncc.Band");
	};

	virtual ~TerminalCategory()
	{
		typename vector<T *>::const_iterator it;
		vector<CarriersGroup *>::const_iterator other_it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			delete *it;
		}

		// delete other carriers in case updateCarriersGroups where not called
		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			delete *other_it;
		}
		// do not delete terminals, they will be deleted in DAMA or SlottedAloha
	};


	/**
	 * @brief  Get the label.
	 *
	 * @return  the label.
	 */
	string getLabel() const
	{
		return this->label;
	};


	/**
	 * @brief Get the weighted sum among all carriers groups on this category
	 *
	 * @return the weighted sum
	 */
	double getWeightedSum() const
	{
		// Compute weighted sum in ks/s since available bandplan is in kHz
		double weighted_sum_ksymps = 0.0;
		typename vector<T *>::const_iterator it;
		vector<CarriersGroup *>::const_iterator other_it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			// weighted_sum = ratio * Rs (ks/s)
			weighted_sum_ksymps += (*it)->getRatio() * (*it)->getSymbolRate() / 1E3;
		}

		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			// weighted_sum = ratio * Rs (ks/s)
			weighted_sum_ksymps += (*other_it)->getRatio() * (*other_it)->getSymbolRate() / 1E3;
		}

		return weighted_sum_ksymps;
	};

	/**
	 * @brief  Get the estimated occupation ratio other all carriers groups of
	 *         this category.
	 *
	 * @return  the estimated occupation ratio.
	 */
	unsigned int getRatio() const
	{
		unsigned int ratio = 0;
		typename vector<T *>::const_iterator it;
		vector<CarriersGroup *>::const_iterator other_it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			ratio += (*it)->getRatio();
		}

		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			ratio += (*other_it)->getRatio();
		}

		return ratio;
	};

	/**
	 * @brief  Set the number and the capacity of carriers in each group
	 *
	 * @param  carriers_number         The number of carriers in the group
	 * @param  superframe_duration_ms  The superframe duration (in ms)
	 */
	void updateCarriersGroups(unsigned int carriers_number,
	                          time_ms_t superframe_duration_ms)
	{
		unsigned int total_ratio = this->getRatio();
		unsigned int total_number = 0;
		typename vector<T *>::const_iterator it;
		vector<CarriersGroup *>::const_iterator other_it;

		if(carriers_number < this->carriers_groups.size())
		{
			LOG(this->log_terminal_category, LEVEL_WARNING, 
			    "Not enough carriers for category %s that contains %zu "
			    "groups. Increase carriers number to the number of "
			    "groups\n",
			    this->label.c_str(), this->carriers_groups.size());
			carriers_number = this->carriers_groups.size();
		}
		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			unsigned int number;
			vol_sym_t capacity_sym;
	
			// get number per carriers from total number in category
			number = round(carriers_number * (*it)->getRatio() / total_ratio);
			if(number == 0)
			{
				number = 1;
			}
			total_number += number;
			(*it)->setCarriersNumber(number);
			LOG(this->log_terminal_category, LEVEL_NOTICE, 
			    "Carrier group %u: number of carriers %u\n",
			    (*it)->getCarriersId(), number);
	
			// get the capacity of the carriers
			capacity_sym = floor((*it)->getSymbolRate() * superframe_duration_ms / 1000);
			(*it)->setCapacity(capacity_sym);
			LOG(this->log_terminal_category, LEVEL_NOTICE, 
			    "Carrier group %u: capacity for Symbol Rate %.2E: %u "
			    "symbols\n", (*it)->getCarriersId(),
			    (*it)->getSymbolRate(), capacity_sym);
		}
		// no need to update other groups, they won't be used anymore
		// then released them
		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			delete *other_it;
		}
		this->other_carriers.clear();
	};

	/**
	 * @brief  Add a terminal to the category
	 *
	 * @param  terminal  terminal to be added.
	 */
	void addTerminal(TerminalContext *terminal)
	{
		terminal->setCurrentCategory(this->label);
		this->terminals.push_back(terminal);
	};

	/**
	 * @brief  Remove a terminal from the category
	 *
	 * @param  terminal  terminal to be removed.
	 * @return true on success, false otherwise
	 */
	bool removeTerminal(TerminalContext *terminal)
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
			LOG(this->log_terminal_category, LEVEL_ERROR, 
			    "ST#%u not registered on category %s",
			    tal_id, this->label.c_str());
			return false;
		}
		return true;
	};

	/**
	 * @brief   Get the carriers groups with the desired access type
	 *
	 * @return  the carriers groups
	 */
	vector<T *> getCarriersGroups(void) const
	{
		return this->carriers_groups;
	};

	/**
	 * @brief  Add a carriers group to the category
	 *
	 * @param  carriers_id  The ID of the carriers group
	 * @param  fmt_group    The FMT group associated to the carrier
	 * @param  ratio        The estimated occupation ratio
	 * @param  rate_symps   The group symbol rate (symbol/s)
	 * @param  access_type  The carriers access type
	 */
	void addCarriersGroup(unsigned int carriers_id,
	                      const FmtGroup *const fmt_group,
	                      unsigned int ratio,
	                      rate_symps_t rate_symps,
	                      access_type_t access_type)
	{
		typename vector<T *>::const_iterator it;
		vector<CarriersGroup *>::const_iterator other_it;
		// first, check if we already have this carriers id in case
		// of VCM carriers
		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			if((*it)->getCarriersId() == carriers_id)
			{
				(*it)->addVcm(fmt_group, ratio);
				return;
			}
		}
		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			if((*other_it)->getCarriersId() == carriers_id)
			{
				(*other_it)->addVcm(fmt_group, ratio);
				return;
			}
		}

		if(access_type == this->desired_access)
		{
			T *group = new T(carriers_id, fmt_group,
			                 ratio, rate_symps,
			                 access_type);
			// we call that because with Dama we need to count this carriers
			// in the VCM list
			group->addVcm(fmt_group, ratio);
			this->carriers_groups.push_back(group);
		}
		else
		{
			CarriersGroup *group = new CarriersGroup(carriers_id, fmt_group,
			                                         ratio, rate_symps,
			                                         access_type);
			this->other_carriers.push_back(group);
		}
	};


	/**
	 * @brief   Get number of carriers with the desired access type.
	 *
	 * @return  number of carriers.
	 */
	unsigned int getCarriersNumber(void) const
	{
		unsigned int carriers_number = 0;
		typename vector<T *>::const_iterator it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			carriers_number += (*it)->getCarriersNumber();
		}
		return carriers_number;
	};

	/**
	 * @brief   Get terminal list.
	 *
	 * @return  terminal list.
	 */
	vector<TerminalContext *> getTerminals() const
	{
		return this->terminals;
	};

 protected:
	// Output Log
	OutputLog *log_terminal_category;

	/** List of terminals. */
	vector<TerminalContext *> terminals;

	/** List of carriers */
	vector<T *> carriers_groups;

	/** The access type of the carriers */
	access_type_t desired_access;
	
	/** The label */
	string label;

 private:
	/** The carriers groups that does not correspond to the desired access type
	 *   needed for band computation */
	vector<CarriersGroup *> other_carriers;
};


template<class T>
class TerminalCategories: public map<string, T *> {};
template<class T>
class TerminalMapping: public map<tal_id_t, T *> {};

#endif


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
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

#include <string>
#include <map>
#include <vector>

using std::string;
using std::map;
using std::vector;

/**
 * @class TerminalCategory
 * @brief Represent a category of terminal.
 */
class TerminalCategory
{

 public:

	/**
	 * @brief  Create a terminal category.
	 *
	 * @param  label  label of the category.
	 */
	TerminalCategory(string label);

	~TerminalCategory();

	/**
	 * @brief  Get the label.
	 *
	 * @return  the label.
	 */
	string getLabel() const;


	/**
	 * @brief Get the weighted sum among carriers groups on thsi category
	 *
	 * @return the weighted sum
	 */
	double getWeightedSum() const;

	/**
	 * @brief  Get the estimated occupation ratio.
	 *
	 * @return  the estimated occupation ratio.
	 */
	unsigned int getRatio() const;

	/**
	 * @brief  Set the number and the capacity of carriers in each group
	 *
	 * @param  carriers_number         The number of carriers in the group
	 * @param  superframe_duration_ms  The superframe duration (in ms)
	 */
	void updateCarriersGroups(unsigned int carriers_number,
	                         time_ms_t superframe_duration_ms);

	/**
	 * @brief  Add a terminal to the category
	 *
	 * @param  terminal  terminal to be added.
	 */
	void addTerminal(TerminalContext *terminal);

	/**
	 * @brief  Remove a terminal from the category
	 *
	 * @param  terminal  terminal to be removed.
	 * @return true on success, false otherwise
	 */
	bool removeTerminal(TerminalContext *terminal);

	/**
	 * @brief  Add a carriers group to the category
	 *
	 * @param  carriers_id  The ID of the carriers group
	 * @param  fmt_group    The FMT group associated to the carrier
	 * @param  ratio        The estimated occupation ratio
	 * @param  rate_symps   The group symbol rate (symbol/s)
	 */
	void addCarriersGroup(unsigned int carriers_id,
	                      const FmtGroup *const fmt_group,
	                      unsigned int ratio,
	                      rate_symps_t rate_symps);

	/**
	 * @brief   Get the carriers groups
	 *
	 * @return  the carriers groups
	 */
	vector<CarriersGroup *> getCarriersGroups() const;

	/**
	 * @brief   Get number of carriers.
	 *
	 * @return  number of carriers.
	 */
	unsigned int getCarriersNumber() const;

	/**
	 * @brief   Get terminal list.
	 *
	 * @return  terminal list.
	 */
	vector<TerminalContext *> getTerminals() const;

	/**
	 * @brief  Get the terminal list in a specific carriers group
	 *
	 * @tparam T            a terminal context
	 * @param  carrier_id   the carrier ID
	 * @return  terminal list in the carriers group
	 */
	template<class T>
	vector<T *> getTerminalsInCarriersGroup(
	                             unsigned int carrier_id) const;

 private:

	/** List of terminals. */
	vector<TerminalContext *> terminals;

	/** List of carriers */
	vector<CarriersGroup *> carriers_groups;

	/** The label */
	string label;

};


typedef map<string, TerminalCategory *> TerminalCategories;
typedef map<unsigned int, TerminalCategory *> TerminalMapping;

template<class T>
vector<T *> TerminalCategory::getTerminalsInCarriersGroup(
                                          unsigned int carrier_id) const
{
	vector<T *> entries;
	vector<TerminalContext *>::const_iterator it;
	for(it = this->terminals.begin();
	    it != this->terminals.end(); ++it)
	{
		if((*it)->getCarrierId() == carrier_id)
		{
			entries.push_back((T *)(*it));
		}
	}
	return entries;
}

#endif


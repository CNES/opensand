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
 * @file    TerminalCategoryDama.h
 * @brief   Represent a category of terminal for DAMA
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _TERMINAL_CATEGORY_DAMA_H
#define _TERMINAL_CATEGORY_DAMA_H

#include "TerminalCategory.h"

#include "TerminalContextDama.h"
#include "CarriersGroupDama.h"

/**
 * @class TerminalCategoryDama
 * @brief Represent a category of terminal for DAMA
 */
class TerminalCategoryDama: public TerminalCategory<CarriersGroupDama>
{
 public:
	/**
	 * @brief  Create a terminal category.
	 *
	 * @param  label  label of the category.
	 * @param  desired_access  the access type we support for our carriers
	 */
	TerminalCategoryDama(const std::string& label, AccessType desired_access=AccessType::DAMA);

	~TerminalCategoryDama();

	/**
	 * @brief  Get the terminal list in a specific carriers group
	 *
	 * @tparam T            a terminal context
	 * @param  carrier_id   the carrier ID
	 * @return  terminal list in the carriers group
	 */
	template<class T>
	std::vector<std::shared_ptr<T>> getTerminalsInCarriersGroup(unsigned int carrier_id) const;
};


template<class T>
std::vector<std::shared_ptr<T>> TerminalCategoryDama::getTerminalsInCarriersGroup(unsigned int carrier_id) const
{
	std::vector<std::shared_ptr<T>> entries;
	for (auto &&t: this->terminals)
	{
		std::shared_ptr<T> terminal = std::dynamic_pointer_cast<T>(t);
		if(terminal->getCarrierId() == carrier_id)
		{
			entries.push_back(terminal);
		}
	}
	return entries;
}


#endif

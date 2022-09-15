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
 * @file TerminalContext.h
 * @brief The termial context
 * @author Julien Bernard / Viveris Technologies
 *
 */


#ifndef _TERMINAL_CONTEXT_H_
#define _TERMINAL_CONTEXT_H_

#include "OpenSandCore.h"

#include <string>
#include <memory>


class OutputLog;


/**
 * @class TerminalContext
 * @brief Interface for a terminal context
 */
class TerminalContext
{
public:
	/**
	 * @brief  Create a terminal context.
	 *
	 * @param  tal_id  terminal id.
	 */
	TerminalContext(tal_id_t tal_id);
	virtual ~TerminalContext();

	/**
	 * @brief  Get the terminal id.
	 *
	 * @return  terminal id.
	 */
	tal_id_t getTerminalId() const;

	/**
	 * @brief Set the current terminal category
	 *
	 * param name  The name of the terminal category
	 */
	void setCurrentCategory(std::string name);

	/**
	 * @brief Get the current terminal category
	 *
	 * @return  The name of the terminal current category
	 */
	std::string getCurrentCategory() const;

protected:
	/** Output Log*/
	std::shared_ptr<OutputLog> log_band;

	/** Terminal id */
	tal_id_t tal_id;

	/** The terminal category */
	std::string category;

};

#endif

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
 * @file    TerminalContext.cpp
 * @brief   The terminal context.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#include "TerminalContext.h"

#include <opensand_output/Output.h>


TerminalContext::TerminalContext(tal_id_t tal_id):
	tal_id(tal_id),
	category("")
{
	this->log_band = Output::registerLog(LEVEL_WARNING, "Dvb.Ncc.Band");
}

TerminalContext::~TerminalContext()
{
}

tal_id_t TerminalContext::getTerminalId() const
{
	return this->tal_id;
}

void TerminalContext::setCurrentCategory(string name)
{
	this->category = name;
}

string TerminalContext::getCurrentCategory() const
{
	return this->category;
}


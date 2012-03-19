/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file GenericSwitch.cpp
 * @brief Generic switch for Satellite Emulator (SE)
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "GenericSwitch.h"

GenericSwitch::GenericSwitch()
{
}

GenericSwitch::~GenericSwitch()
{
}

bool GenericSwitch::add(long tal_id, long spot_id)
{
	bool success = false;
	std::map <long, long >::iterator it =  this->switch_table.find(tal_id);

	if(it == this->switch_table.end())
	{
		// switch entry does not exist yet
		std::pair < std::map < long, long >::iterator, bool > infos;
		infos = this->switch_table.insert(std::make_pair(tal_id, spot_id));

		if(!infos.second)
			goto quit;
	}

	success = true;

quit:
	return success;
}

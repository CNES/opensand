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

bool GenericSwitch::add(uint8_t tal_id, uint8_t spot_id)
{
	bool success = false;
	std::map <uint8_t, uint8_t >::iterator it =  this->switch_table.find(tal_id);

	if(it == this->switch_table.end())
	{
		// switch entry does not exist yet
		std::pair < std::map < uint8_t, uint8_t >::iterator, bool > infos;
		infos = this->switch_table.insert(std::make_pair(tal_id, spot_id));

		if(!infos.second)
			goto quit;
	}

	success = true;

quit:
	return success;
}

uint8_t GenericSwitch::find(NetPacket *packet)
{
	std::map < uint8_t, uint8_t >::iterator it;
	uint8_t spot_id = 0;

  	if(packet == NULL)
		return spot_id;

  	it = this->switch_table.find(packet->getDstTalId());

	if(it != this->switch_table.end())
		spot_id = (*it).second;

	return spot_id;
}


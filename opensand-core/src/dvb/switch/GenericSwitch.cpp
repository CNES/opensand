/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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

#include "OpenSandConf.h"

GenericSwitch::GenericSwitch():
	default_spot(0)
{
}

GenericSwitch::~GenericSwitch()
{
}

bool GenericSwitch::add(tal_id_t tal_id, spot_id_t spot_id)
{
	bool success = false;
	std::map <tal_id_t, spot_id_t>::iterator it =  this->switch_table.find(tal_id);

	if(it == this->switch_table.end())
	{
		// switch entry does not exist yet
		std::pair < std::map <tal_id_t, spot_id_t>::iterator, bool > infos;
		infos = this->switch_table.insert(std::make_pair(tal_id, spot_id));

		if(!infos.second)
			goto quit;
	}

	success = true;

quit:
	return success;
}

void GenericSwitch::setDefault(spot_id_t spot_id)
{
	this->default_spot = spot_id;
}

spot_id_t GenericSwitch::find(NetPacket *packet)
{
	std::map <tal_id_t, spot_id_t >::iterator it;
	spot_id_t spot_id = 0;
	tal_id_t tal_id = 0;

  	if(packet == NULL)
		return spot_id;

	tal_id = packet->getDstTalId();
	// for GW as destination we need to use the source to determine the spot
	if(OpenSandConf::isGw(tal_id))
	{
		tal_id = packet->getSrcTalId();
	}

	it = this->switch_table.find(tal_id);

	if(it != this->switch_table.end())
		spot_id = (*it).second;
	else
		spot_id = this->default_spot;

	return spot_id;
}


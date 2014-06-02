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
 * @file    TerminalContextSaloha.cpp
 * @brief   The terminal context for Slotted Aloha
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#include "TerminalContextSaloha.h"

#include <opensand_output/Output.h>

TerminalContextSaloha::TerminalContextSaloha(tal_id_t tal_id):
	TerminalContext(tal_id),
	wait_propagation(),
	last_propagated()
{
}

TerminalContextSaloha::~TerminalContextSaloha()
{
	for(map<qos_t, saloha_packets_t *>::iterator it = this->wait_propagation.begin();
	    it != this->wait_propagation.end(); ++it)
	{
		for(saloha_packets_t::iterator iter = (*it).second->begin();
		    iter != (*it).second->end(); ++iter)
		{
			delete *iter;
		}
		delete (*it).second;
	}
	this->wait_propagation.clear();
}

saloha_packets_t *TerminalContextSaloha::getWaitPropagationPackets(qos_t qos)
{
	if(!this->wait_propagation[qos])
	{
		this->wait_propagation[qos] = new saloha_packets_t();
	}
	return this->wait_propagation[qos];
}

saloha_id_t TerminalContextSaloha::getLastPropagatedId(qos_t qos)
{
	return this->last_propagated[qos];
}

void TerminalContextSaloha::setLastPropagatedId(qos_t qos, saloha_id_t id)
{
	this->last_propagated[qos] = id;
}

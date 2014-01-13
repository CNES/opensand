/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file    Logoff.cpp
 * @brief   Represent a Logoff
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#include <opensand_conf/uti_debug.h>

#include "Logoff.h"


Logoff::Logoff(tal_id_t mac):
	DvbFrameTpl<T_DVB_LOGOFF>()
{
	this->setMessageType(MSG_TYPE_SESSION_LOGOFF);
	this->setMessageLength(sizeof(T_DVB_LOGOFF));
	this->frame()->mac = htons(mac);
}

Logoff::~Logoff()
{
}

tal_id_t Logoff::getMac(void) const
{
	return ntohs(this->frame()->mac);
}



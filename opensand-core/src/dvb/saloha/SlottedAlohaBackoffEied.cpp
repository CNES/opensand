/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file SlottedAlohaBackoffEied.cpp
 * @brief The EIED backoff algorithm
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#include "SlottedAlohaBackoffEied.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>

using std::min;

SlottedAlohaBackoffEied::SlottedAlohaBackoffEied(uint16_t max, uint16_t multiple):
	SlottedAlohaBackoff(max, multiple)
{
	this->setReady();
}

SlottedAlohaBackoffEied::~SlottedAlohaBackoffEied()
{
}

uint16_t SlottedAlohaBackoffEied::setReady()
{
	this->cw = min((int)this->cw * (int)sqrt(this->multiple), (int)this->cw_max);
	this->randomize();
	return this->backoff;
}

uint16_t SlottedAlohaBackoffEied::setCollision()
{
	this->cw = min((int)this->cw * (int)this->multiple, (int)this->cw_max);
	this->randomize();
	return this->backoff;
}


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file SlottedAlohaBackoff.cpp
 * @brief The backoff algorithms generic class
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#include "SlottedAlohaBackoff.h"

#include <stdlib.h>
#include <algorithm>


SlottedAlohaBackoff::SlottedAlohaBackoff(uint16_t max, uint16_t multiple):
	cw_min(1),
	cw_max(max),
	cw(0),
	backoff(0),
	multiple(multiple)
{
}

SlottedAlohaBackoff::~SlottedAlohaBackoff()
{
}

void SlottedAlohaBackoff::tick()
{
	this->backoff = std::max((int)this->backoff - 1, 0);
}

void SlottedAlohaBackoff::randomize()
{
	this->backoff = (rand() / (double)RAND_MAX) * this->cw;
}

bool SlottedAlohaBackoff::isReady() const
{
	return (this->backoff <= 0);
}


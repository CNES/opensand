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
 * @file SlottedAlohaBackoffMimd.cpp
 * @brief The MIMD backoff algorithm
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
*/

#include "SlottedAlohaBackoffMimd.h"

#include <stdlib.h>
#include <algorithm>

using std::min;

/**
 * @class SlottedAlohaBackoffMimd
 * @brief The MIMD backoff algorithm
*/

SlottedAlohaBackoffMimd::SlottedAlohaBackoffMimd(uint16_t max, uint16_t multiple):
	SlottedAlohaBackoff(max, multiple)
{
	this->setOk();
}

SlottedAlohaBackoffMimd::~SlottedAlohaBackoffMimd()
{
}

uint16_t SlottedAlohaBackoffMimd::setOk()
{
	this->cw = min((int)this->cw / (int)this->multiple, (int)this->cw_max);
	this->setRandom();
	return this->backoff;
}

uint16_t SlottedAlohaBackoffMimd::setNok()
{
	this->cw = min((int)this->cw * (int)this->multiple, (int)this->cw_max);
	this->setRandom();
	return this->backoff;
}


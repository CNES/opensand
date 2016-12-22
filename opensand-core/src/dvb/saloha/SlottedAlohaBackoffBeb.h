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
 * @file SlottedAlohaBackoffBeb.h
 * @brief The BEB backoff algorithm
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#ifndef SALOHA_BACKOFF_BEB_H
#define SALOHA_BACKOFF_BEB_H

#include "SlottedAlohaBackoff.h"


/**
 * @class SlottedAlohaBackoffBeb
 * @brief The BEB backoff algorithm
*/
class SlottedAlohaBackoffBeb: public SlottedAlohaBackoff
{
 public:

	/**
	 * Build class
	 *
	 * @param max		maximum value for the contention window
	 * @param multiple	multiple used to refresh the backoff
	 */
	SlottedAlohaBackoffBeb(uint16_t max, uint16_t multiple);

	/**
	 * Class destructor
	 */
	~SlottedAlohaBackoffBeb();

 private:
	uint16_t setReady();
	uint16_t setCollision();
};

#endif


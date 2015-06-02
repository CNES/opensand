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
 * @file SlottedAlohaBackoff.h
 * @brief The backoff algorithms generic class
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#ifndef SALOHA_BACKOFF_H
#define SALOHA_BACKOFF_H

#include <stdint.h>

/**
 * @class SlottedAlohaBackoff
 * @brief The backoff algorithms
*/

class SlottedAlohaBackoff
{
protected:
	/// Minimum value of the contention window
	uint16_t cw_min;

	/// Maximum value of the contention window
	uint16_t cw_max;

	/// Current value of the contention window
	uint16_t cw;

	/// Current value of the backoff
	uint16_t backoff;

	/// Multiple used to refresh the backoff
	uint16_t multiple;

	/**
	 * Set a random value of the backoff
	 */
	void randomize();

public:
	/**
	 * Build generic backoff class
	 *
	 * @param max		maximum value for the contention window
	 * @param multiple	multiple used to refresh the backoff
	 */
	SlottedAlohaBackoff(uint16_t max, uint16_t multiple);

	/**
	 * Class destructor
	 */
	virtual ~SlottedAlohaBackoff();

	/**
	 * Decrease the backoff
	 */
	void tick();

	/**
	 * Return true if backoff is ready, false otherwise
	 *
	 * @return true if backoff is ready, false otherwise
	 */
	bool isReady() const;

	/**
	 * Refresh the backoff after a successful transmission
	 *
	 * @return the backoff value
	 */
	virtual uint16_t setReady() = 0;

	/**
	 * Refresh the backoff after a collision
	 *
	 * @return the backoff value
	 */
	virtual uint16_t setCollision() = 0;
};

#endif


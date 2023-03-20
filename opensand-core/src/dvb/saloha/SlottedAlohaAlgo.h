/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file SlottedAlohaAlgo.h
 * @brief The Slotted Aloha algos
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#ifndef SALOHA_ALGO_H
#define SALOHA_ALGO_H

#include "SlottedAlohaFrame.h"
#include "Slot.h"

#include <algorithm>
#include <map>
#include <set>


/// A list of TS
typedef std::set<uint16_t> saloha_ts_list_t;
class OutputLog;


/**
 * @class SlottedAlohaAlgo
 * @brief The Slotted Aloha algos
*/

class SlottedAlohaAlgo
{
public:
	/**
	 * Class constructor without any parameters
	 */
	SlottedAlohaAlgo();

	/**
	 * Class destructor
	 */
	virtual ~SlottedAlohaAlgo();

	/**
	 * Remove collisions with a specific algorithm
	 *
	 * @param slots    Slots containing the received Slotted Aloha data packets
	 * @param fifo     the packets that are not collisionned
	 * @return the number of collisionned packets
	 */
	virtual uint16_t removeCollisions(std::map<unsigned int, std::shared_ptr<Slot>> &slots,
	                                  saloha_packets_data_t &accepted_packets) = 0;

protected:
	std::shared_ptr<OutputLog> log_saloha;
};


#endif


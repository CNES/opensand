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
 * @file SlottedAlohaCrdsa.h
 * @brief The CRDSA algo
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

#ifndef SALOHA_ALGO_CRDSA_H
#define SALOHA_ALGO_CRDSA_H

#include "SlottedAlohaAlgo.h"

/**
 * @class SlottedAlohaCrdsa
 * @brief The CRDSA algo
*/
class SlottedAlohaAlgoCrdsa: public SlottedAlohaAlgo
{
 public:

	SlottedAlohaAlgoCrdsa();

	~SlottedAlohaAlgoCrdsa();

 private:
	uint16_t removeCollisions(std::map<unsigned int, Slot *> &slots,
	                          saloha_packets_data_t *accepted_packets);
};

#endif

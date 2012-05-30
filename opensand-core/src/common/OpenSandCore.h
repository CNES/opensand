/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 * Copyright © 2012 CNES
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
 * @file OpenSandCore.h
 * @brief Some OpenSAND core utilities
 */

#ifndef OPENSAND_CORE_H
#define OPENSAND_CORE_H

using std::string;

/** The different types of DVB components */
typedef enum
{
	satellite,
	gateway,
	terminal
} t_component;

/** @brief Get the name of a component
 *
 * @param host The component type
 * @return the abbreviated name of the component
 */
inline string getComponentName(t_component host)
{
	switch(host)
	{
		case satellite:
			return "sat";
			break;
		case gateway:
			return "gw";
			break;
		case terminal:
			return "st";
			break;
		default:
			return "";
	}
}

// Broadcast tal id is maximal tal_id value authorized (5 bits).
#define BROADCAST_TAL_ID 0x1F

/// The types used in OpenSAND
typedef uint8_t tal_id_t;
typedef uint8_t spot_id_t;
typedef uint8_t qos_t;

#endif


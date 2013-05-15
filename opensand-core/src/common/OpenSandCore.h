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


#include <string>
#include <stdint.h>

/** unused macro to avoid compilation warning with unused parameters. */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */


using std::string;

/// The different types of DVB components
typedef enum
{
	satellite,
	gateway,
	terminal
} component_t;


/** @brief Get the name of a component
 *
 * @param host The component type
 * @return the abbreviated name of the component
 */
inline string getComponentName(component_t host)
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
};


// Broadcast tal id is maximal tal_id value authorized (5 bits).
#define BROADCAST_TAL_ID 0x1F

// The types used in OpenSAND

// addressing
typedef uint8_t tal_id_t; ///< Terminal ID (5 bits)
typedef uint8_t spot_id_t; ///< Spot ID (5 bits)
typedef uint8_t qos_t; ///< QoS (3 bits)
typedef uint8_t group_id_t; ///< Groupe ID (no used but in the standard)

// TODO check types according to max value
// data
typedef uint16_t rate_kbps_t; ///< Bitrate in kb/s (suffix kbps)
typedef uint16_t rate_pktpsf_t; ///< Rate in packets/cells per superframe (suffix pktpsf)

// time
typedef uint8_t time_sf_t; ///< time in number of superframes (suffix sf)
typedef uint8_t time_frame_t; ///< time in number of frames (5 bits) (suffix frame)
typedef uint32_t time_ms_t; ///< time in ms (suffix ms)
typedef uint16_t time_pkt_t; ///< time in number of packets, cells, ... (suffix pkt)

// volume
typedef uint16_t vol_pkt_t; ///< volume in number of packets/cells (suffix pkt)
typedef uint16_t vol_kb_t; ///< volume in kbits (suffix kb)
typedef uint32_t vol_b_t; ///< volume in bits (suffix b)


/**
 * @brief Generic Superframe description
 *
 *  freq
 *  ^
 *  |
 *  | +--------------+
 *  | |  f   |       |
 *  | |---+--|  sf   | sf_id
 *  | | f |f |       |
 *  | |--------------+
 *  | |   |  sf   |  | sf_id
 *  | +--------------+
 *  |
 *  +-----------------------> time
 *
 *  with sf = superframe and f = frame
 */

/**
 * @brief Superframe for DVB-RCS in OpenSAND
 *
 *  freq
 *  ^
 *  | frame duration (default: 53ms)
 *  | <-->
 *  | +---------------+
 *  | | f | f |  sf   | sf_id
 *  | |---------------+
 *  | |  sf   |  sf   | sf_id
 *  | +---------------+
 *  |
 *  +-----------------------> time
 */


#endif


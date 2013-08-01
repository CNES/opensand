/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file msg_dvb_rcs.h
 * @brief This file defines message type for DVB-S/RCS related packets
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
*/

#ifndef MSG_DVB_RCS_H
#define MSG_DVB_RCS_H

// TODO move in lib_dvb_rcs... and rename lib_dvb_rcs into OpenSandFrames ?
enum
{
	msg_data = 0,  ///< message containing useful data (DVB, encap, ...)
	               //   default value of sendUp/Down function
	msg_link_up,   ///< link up message
};


/// The maximum size of a DVB-RCS frame is choosen to be totally
/// included in one sat_carrier packet
const unsigned long MSG_DVB_RCS_SIZE_MAX = 1200;
const unsigned long MSG_BBFRAME_SIZE_MAX = 8100;
const unsigned long MSG_PHYFRAME_SIZE_MAX = 8;

/// This message is used by dvb rcs layer to advertise the upper layer
/// that the link is up
typedef struct
{
	long group_id;  /// The id of the station
	long tal_id;
} T_LINK_UP;


#endif

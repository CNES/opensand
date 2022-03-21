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
 * @file OpenSandFrames.h
 * @brief The headers and related information for OpenSAND frames
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.fr>
 */


#ifndef _OPENSAND_FRAMES_H_
#define _OPENSAND_FRAMES_H_

// TODO only class inheriting from frame should include this !
// TODO move T_DVB_XXX into related classes
//      move other content into DvbFrames
// MSG_TYPES should be in these classes as for NET_PROTO in NetPacket


#include <opensand_output/Output.h>

#include "OpenSandCore.h"

#include <string>
#include <stdint.h>
#include <arpa/inet.h>
#include <cstring>
#include <bits/endian.h>


/// The maximum size of a DVB-RCS frame is choosen to be totally
/// included in one sat_carrier packet
#define MSG_DVB_RCS_SIZE_MAX 1200 + sizeof(T_DVB_PHY)
/// The maximum size of a BBFrame
#define MSG_BBFRAME_SIZE_MAX 8100 + sizeof(T_DVB_PHY)
#define MSG_SALOHA_SIZE_MAX 1200 + sizeof(T_DVB_PHY)

/// Whether the frame has to be attenuated
#define IS_ATTENUATED_FRAME(msg_type) \
    (msg_type == MSG_TYPE_BBFRAME || msg_type == MSG_TYPE_DVB_BURST || \
     msg_type == MSG_TYPE_SALOHA_DATA || msg_type == MSG_TYPE_SALOHA_CTRL || \
     msg_type == MSG_TYPE_SAC || msg_type == MSG_TYPE_TTP)

/// Whether the frame has to be delayed
#define IS_DELAYED_FRAME(msg_type) \
    (msg_type != MSG_TYPE_SOF)

/// Whether the frame can carry C/N information
/// Warning: all frames that are not C/N capable
///          should not be attenuated.
#define IS_CN_CAPABLE_FRAME(msg_type) \
    (msg_type != MSG_TYPE_SOF)

/**
 * Here are defined internal dvb message types
 * They are of different kind:
 *       NCC -> ST:  this message is to be emitted from the NCC to the ST  only
 *       ST -> NCC:  -------------------------------------- ST  ------ NCC ----
 *       ST -> ST:   -----------------------------------an  ST  to an  ST  ----
 *       NCC internal: Internal NCC message but also SE to NCC message
 *       NCC -> ST: etc
 *       ST -> NCC: etc
 */

/**
 * Error type, could be used as default value that should later be replaced
 */
#define MSG_TYPE_ERROR 0

/**
 * Start of Frame, NCC -> ST
 */
#define MSG_TYPE_SOF 1

/**
 * Satellie Access Control, ST -> NCC
 */
#define MSG_TYPE_SAC 10

/**
 * FIXME: to be documented, ST->NCC
 */
#define MSG_TYPE_CSC 11

/**
 * DVB burst, ST->ST
 */
#define MSG_TYPE_DVB_BURST 12

/**
 * BBFRAME
 */
#define MSG_TYPE_BBFRAME 13

/**
 * Slotted Aloha data burst
 */
#define MSG_TYPE_SALOHA_DATA 14

/**
 * Slotted Aloha control burst
 */
#define MSG_TYPE_SALOHA_CTRL 15

/**
 * Allocation Table, NCC -> ST
 */
#define MSG_TYPE_TTP 21

/**
 * Synchronization message (unused), NCC->ST
 */
#define MSG_TYPE_SYNC 22

/**
 * Request a logon, ST -> NCC
 */
#define MSG_TYPE_SESSION_LOGON_REQ 50

/**
 * Response from the NCC, NCC ->ST
 */
#define MSG_TYPE_SESSION_LOGON_RESP 52

/**
 * Announce a logoff, ST -> NCC
 */
#define MSG_TYPE_SESSION_LOGOFF 51

/**
 * Basic DVB Header, other structures defined below should follow in a packet
 */
typedef struct
{
	uint16_t msg_length; ///< Total length of the message (including _this_ header)
#if __BYTE_ORDER == __BIG_ENDIAN
	uint8_t corrupted:1;  ///< Whether the frame is corrupted by physical layer
	uint8_t msg_type:7;   ///< Type of the message (see \#defines above)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t msg_type:7;   ///< Type of the message (see \#defines above)
	uint8_t corrupted:1;  ///< Whether the frame is corrupted by physical layer
#else
#error "Please fix <bits/endian.h>"
#endif

} __attribute__((__packed__)) T_DVB_HDR;

/**
 * Generic Frame
 */
typedef struct
{
	T_DVB_HDR hdr;
} T_DVB_FRAME;

/**
 * Carry information about physicalLayer block.
 */
typedef struct
{
	uint32_t cn_previous;  ///< The C/N computed on the link (* 100)
} __attribute__((__packed__)) T_DVB_PHY;

/**
 * This message type is a trick.
 * It is managed by the lowest layer on top of ethernet in order to emulate
 * a synchronization algorithm.
 * Namely it is a "Start of superFrame, let us go" message.
 * It is used to tick entities every superframes.
 * An internal mechanism must be designed to awake a process every frame
 * A SOF message isn't subject to satellite delay emulation (it goes quicker
 * than light !)
 */
typedef struct
{
	T_DVB_HDR hdr;    ///< Basic DVB Header, used only to be caught by the dvb layer
	uint16_t sf_nbr;  ///< SuperFrame Number
} __attribute__((__packed__)) T_DVB_SOF;


/**
 * Logon Request
 */
typedef struct
{
	T_DVB_HDR hdr;            ///< Basic DVB Header
	tal_id_t mac;             ///< ST MAC address
	rate_kbps_t rt_bandwidth; ///< the real time fixed bandwidth in kbits/s
	rate_kbps_t max_rbdc;     ///< the maximum RBDC value in kbits/s
	vol_kb_t max_vbdc;        ///< the maximum VBDC value in kbits/s
	bool is_scpc;             ///< is the terminal scpc
} __attribute__((__packed__)) T_DVB_LOGON_REQ;


/**
 * Logon response emitted by the NCC
 */
typedef struct
{
	T_DVB_HDR hdr;       ///< Basic DVB Header
	tal_id_t mac;        ///< Terminal MAC address
	group_id_t group_id; ///< Assigned Group Id
	tal_id_t  logon_id;  ///< Assigned Logon Id
} __attribute__((__packed__)) T_DVB_LOGON_RESP;


/**
 * Logoff Signalling from the ST
 */
typedef struct
{
	T_DVB_HDR hdr; ///< Basic DVB Header
	tal_id_t mac;  ///< Satellite MAC ST address
} __attribute__((__packed__)) T_DVB_LOGOFF;

/**
 * BB frame header
 */
typedef struct
{
	T_DVB_HDR hdr;
	uint16_t data_length;
	uint8_t used_modcod;
} __attribute__((__packed__)) T_DVB_BBFRAME;


/**
 * Format of an encapsulation frame burst
 * Essentially an encapsulation packets array descriptor
 */
typedef struct
{
	T_DVB_HDR hdr;         ///< Basic DVB_RCS Header
	uint16_t qty_element;  ///< Number of following encapsulation packets
	uint8_t modcod;        ///< The MODCOD of the data carried in frame
} __attribute__((__packed__)) T_DVB_ENCAP_BURST;


/**
 * Slotted Aloha header
 */
typedef struct
{
	T_DVB_HDR hdr;
	uint16_t data_length;
} __attribute__((__packed__)) T_DVB_SALOHA;


/// This message is used by dvb rcs layer to advertise the upper layer
/// that the link is up
typedef struct
{
	group_id_t group_id;  /// The id of the station
	tal_id_t tal_id;      /// The terminal ID
} T_LINK_UP;

// TODO rename into OpenSandHeaders
#endif

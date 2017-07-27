/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file Types.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  Types for opensand-rt
 *
 */



#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>
#include <cstring>


#define MAX_SOCK_SIZE 9000

/// opensand-rt event types
typedef enum
{
	evt_net_socket,  ///< Event of type NetSocket
	evt_timer,       ///< Event of type Timer
	evt_message,     ///< Event of type Message
	evt_signal,      ///< Event of type Signal
	evt_file,        ///< Event of type File
	evt_tcp_listen,  ///< Event of type TcpListen
} event_type_t;


/// the channel direction
// TODO won't be necessary anymore once everything will be done in channel
typedef enum
{
	upward_chan,   ///< upward channel
	downward_chan, ///< downward channel
} chan_type_t;

typedef int32_t event_id_t;

typedef struct
{
	void *data;
	size_t length;
	uint8_t type;
} rt_msg_t;

#endif

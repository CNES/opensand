/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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

#include <cstddef>
#include <cstdint>


constexpr std::size_t MAX_SOCK_SIZE{9000};


/// opensand-rt event types
enum EventType
{
	NetSocket,   ///< Event of type NetSocket
	Timer,       ///< Event of type Timer
	Message,     ///< Event of type Message
	Signal,      ///< Event of type Signal
	File,        ///< Event of type File
	TcpListen,   ///< Event of type TcpListen
};


using event_id_t = int32_t;


struct rt_msg_t
{
	void *data;
	size_t length;
	uint8_t type;
};


#endif

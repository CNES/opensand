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
 * @file NetSocketEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The event for message read on network socket
 *
 */


#ifndef NET_SOCKET_EVENT_H
#define NET_SOCKET_EVENT_H

#include <netinet/in.h>

#include "FileEvent.h"
#include "Types.h"


/**
  * @class NetSocketEvent
  * @brief Events describing data received on a nework socket
  *
  */
class NetSocketEvent: public FileEvent
{
 public:
	/**
	 * @brief NetSocketEvent constructor
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor for the event
	 * @param max_size  The maximum data size
	 * @param priority  The priority of the event
	 */
	NetSocketEvent(const std::string &name,
	               int32_t fd = -1,
	               std::size_t max_size = MAX_SOCK_SIZE,
	               uint8_t priority = 4);

	/**
	 * @brief Get the message source address
	 *
	 * @return the message source address
	 */
	inline struct sockaddr_in getSrcAddr(void) const {return this->src_addr;};

	bool handle(void) override;

 protected:
	/// The source address of the message;
	struct sockaddr_in src_addr;
};


#endif

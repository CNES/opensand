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
 * @file TcpListenEvent.h
 * @author Adrien THIBAUD / <athibaud@toulouse.viveris.com>
 * @brief  The event for message read on network socket
 *
 */


#ifndef TCP_LISTEN_EVENT_H
#define TCP_LISTEN_EVENT_H

#include "FileEvent.h"
#include "Types.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
  * @class TcpListenEvent
  * @brief Events describing data received on a nework socket
  *
  */
class TcpListenEvent: public FileEvent
{

  public:

	/**
	 * @brief TcpListenEvent constructor
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor for the event
	 * @param max_size  The maximum data size
	 * @param priority  The priority of the event
	 */
	TcpListenEvent(const std::string &name,
	               int32_t fd = -1,
	               size_t max_size = MAX_SOCK_SIZE,
	               uint8_t priority = 4):
		FileEvent(name, fd, max_size, priority, evt_tcp_listen)
	{};

	~TcpListenEvent();

	/**
	 * @brief Get the message content
	 *
	 * @return the data contained in the message, it can be casted as a char*
	 */
	unsigned char *getData(void);

	/**
	 * @brief Get the file descriptor of the socket client
	 *
	 * @return the file descriptor of the socket client
	 */
	int32_t getSocketClient(void) const {return this->socket_client;};

	/**
	 * @brief  handle event
	 * 
	 * @return  True on success, false otherwise
	 */
	virtual bool handle(void);

  protected:

	/// The file descriptor of the socket client
	int32_t socket_client;

};

#endif

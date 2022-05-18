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
 * @file NetSocketEvent.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The event for message read on network socket
 *
 */

#include "NetSocketEvent.h"
#include "Rt.h"

#include <opensand_output/Output.h>

#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>


// TODO add send functions
NetSocketEvent::NetSocketEvent(const std::string &name,
                               int32_t fd,
                               std::size_t max_size,
                               uint8_t priority):
	FileEvent{name, fd, max_size, priority, EventType::NetSocket}
{
}


bool NetSocketEvent::handle(void)
{
	if(this->data)
	{
		Rt::reportError(this->name, std::this_thread::get_id(), false,
		                "event %s: previous data was not handled\n",
		                this->name.c_str());
    delete [] this->data;
	}
	// one more byte so we can use it as char*
	this->data = new unsigned char[this->max_size + 1]();  // parens do value initialization to 0

	socklen_t addrlen = sizeof(struct sockaddr_in);
	int ret = recvfrom(this->fd, this->data, this->max_size, 0,
	                   reinterpret_cast<struct sockaddr *>(&this->src_addr), &addrlen);
  std::size_t actual_size = static_cast<std::size_t>(ret);

	if(ret < 0)
	{
		Rt::reportError(this->name, std::this_thread::get_id(), false,
		                "event %s: unable to read on socket [%u: %s]",
		                this->name.c_str(), errno, strerror(errno));
		goto error;
	}

	if(actual_size > this->max_size)
	{
		Rt::reportError(this->name, std::this_thread::get_id(), false,
		                "event %s: too many data received (%zu > %zu)\n",
		                this->name.c_str(), this->size, this->max_size);
		goto error;
	}
	else if(actual_size == 0)
	{
		Rt::reportError(this->name, std::this_thread::get_id(), false,
		                 "event %s: distant host disconnected\n",
		                 this->name.c_str());
		goto error;
	}
	this->size = ret;

	return true;

error:
	delete [] this->data;
	this->data = nullptr;
	return false;
}

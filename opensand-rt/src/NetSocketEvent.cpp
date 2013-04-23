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
 * @brief  The event for message read on network socket, can also be used
 *         by any fd-like oject such as file
 *
 */

#include "NetSocketEvent.h"
#include "Rt.h"

#include <cstring>
#include <unistd.h>
#include <errno.h>


NetSocketEvent::NetSocketEvent(const string &name, int32_t fd, uint8_t priority):
	Event(evt_net_socket, name, fd, priority),
	size(0)
{
}

NetSocketEvent::~NetSocketEvent()
{
}

bool NetSocketEvent::handle(void)
{
	this->size = read(this->fd, this->data, MAX_SOCK_SIZE);
	if(this->size < 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "unable to read on socket", errno);
		return false;
	}

	return true;
}

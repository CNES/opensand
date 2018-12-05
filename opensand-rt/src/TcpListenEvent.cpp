/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
 * @file TcpListenEvent.cpp
 * @author Adrien THIBAUD / <athibaud@toulouse.viveris.com>
 * @brief  The event for message read on network socket
 *
 */

#include "TcpListenEvent.h"
#include "Rt.h"

#include <opensand_output/Output.h>

#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


// TODO add send functions

TcpListenEvent::~TcpListenEvent()
{
	if(this->data)
	{
		free(this->data);
	}
}

bool TcpListenEvent::handle(void)
{
	struct sockaddr_in addr;
	socklen_t addr_length;
	long option;
	
	addr_length = sizeof(struct sockaddr_in);
	
	// wait for a client to connect (this should not block because the
	// function is called only when there is an event on the listen socket)
	this->socket_client = accept(this->fd,
	                             (struct sockaddr *) &addr,
	                             &addr_length);
	if(this->socket_client < 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		    "failed to accept new connection on socket: "
		    "%s (%d)\n", strerror(errno), errno);
		goto error;
	}

	// set the new socket in non blocking mode
	option = O_NONBLOCK;
	if(fcntl(this->socket_client, F_SETFL, option) != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		    "set socket in non blocking mode failed: %s (%d)\n",
		    strerror(errno), errno);
		goto close;
	}

	return true;
close:
	close(this->socket_client);
error:
	free(this->data);
	this->data = NULL;
	return false;
}


unsigned char *TcpListenEvent::getData(void)
{
	unsigned char *buf = this->data;
	this->data = NULL;
	return buf;
}



/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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


// TODO add send functions

NetSocketEvent::~NetSocketEvent()
{
	if(this->data)
	{
		free(this->data);
	}
}

bool NetSocketEvent::handle(void)
{
	int ret;
	socklen_t addrlen;
	if(this->data)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "event %s: previous data was not handled\n",
		                this->name.c_str());
		free(this->data);
	}
	// one more byte so we can use it as char*
	this->data = (unsigned char *)calloc(this->max_size + 1, sizeof(unsigned char));

	addrlen = sizeof(struct sockaddr_in);
	ret = recvfrom(this->fd, this->data, this->max_size, 0,
	               (struct sockaddr *) &(this->src_addr), &addrlen);
	if(ret < 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "event %s: unable to read on socket [%u: %s]",
		                this->name.c_str(), errno, strerror(errno));
		goto error;
	}
	else if((size_t)ret > this->max_size)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "event %s: too many data received (%zu > %zu)\n",
		                this->name.c_str(), this->size, this->max_size);
		goto error;
	}
	else if(ret == 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                 "event %s: distant host disconnected\n",
		                 this->name.c_str());
		goto error;
	}
	this->size = ret;

	return true;
error:
	free(this->data);
	this->data = NULL;
	return false;
}


unsigned char *NetSocketEvent::getData(void)
{
	unsigned char *buf = this->data;
	this->data = NULL;
	return buf;
}

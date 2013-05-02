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
 * @file FileEvent.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The event for message read on fd-like object
 *
 */

#include "FileEvent.h"
#include "Rt.h"

#include <cstring>
#include <unistd.h>
#include <errno.h>


FileEvent::FileEvent(const string &name, int32_t fd, uint8_t priority):
	RtEvent(evt_file, name, fd, priority),
	size(0)
{
}

FileEvent::~FileEvent()
{
}

bool FileEvent::handle(void)
{
	int ret;
	
	if(this->data)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "event %s: previous data was not handled\n",
		                this->name.c_str());
		free(this->data);
	}
	/// on more byte so we can use it as char*
	this->data = (unsigned char *)calloc(MAX_READ_SIZE + 1, sizeof(unsigned char));

	ret = read(this->fd, this->data, MAX_READ_SIZE);
	if(ret < 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "unable to read on socket [%u: %s]", errno, strerror(errno));
		goto error;
	}
	else if(ret == 0)
	{
		// EOF
		free(this->data);
		this->data = NULL;
	}
	this->size = (size_t)ret;

	return true;
error:
	free(this->data);
	this->data = NULL;
	return false;
}

unsigned char *FileEvent::getData(void)
{
	unsigned char *buf = this->data;
	this->data = NULL;
	return buf;
}

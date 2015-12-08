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
 * @file MessageEvent.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The message event
 *
 */

#include "MessageEvent.h"
#include "RtFifo.h"
#include "Rt.h"

#include <cstring>
#include <errno.h>
#include <unistd.h>


MessageEvent::MessageEvent(RtFifo *const fifo,
                           const string &name,
                           int32_t fd,
                           uint8_t priority):
	RtEvent(evt_message, name, fd, priority),
	fifo(fifo)
{
}

MessageEvent::~MessageEvent()
{
}

bool MessageEvent::handle(void)
{
	unsigned char data[strlen(MAGIC_WORD)];
	int rlen;

	// read the pipe to clear it, and check that if contains
	// the correct signaling
	rlen = read(this->fd, data, strlen(MAGIC_WORD));
	if(rlen != strlen(MAGIC_WORD) ||
	   strncmp((char *)data, MAGIC_WORD, strlen(MAGIC_WORD)) != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "pipe signaling message from previous block contain wrong data ",
		                "[%u: %s]", errno, strerror(errno));
		return false;
	}

	// set the event content
	if(!this->fifo->pop(this->message))
	{
		return false;
	}
	return true;

}

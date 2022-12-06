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
 * @file MessageEvent.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The message event
 *
 */

#include <cstring>

#include "MessageEvent.h"
#include "Rt.h"
#include "RtFifo.h"
#include "RtCommunicate.h"


namespace Rt
{


MessageEvent::MessageEvent(std::shared_ptr<Fifo> &fifo,
                           const std::string &name,
                           int32_t fd,
                           uint8_t priority):
	Event{EventType::Message, name, fd, priority},
	message{nullptr},
	fifo{fifo}
{
}


bool MessageEvent::handle(void)
{
	// read the pipe to clear it, and check that if contains
	// the correct signaling
	if (!check_read(this->fd))
	{
		Rt::reportError(this->name, std::this_thread::get_id(), false,
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


};

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
 * @file MessageEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The message events
 *
 */

#ifndef MESSAGE_EVENT_H
#define MESSAGE_EVENT_H

#include "RtEvent.h"
#include "Types.h"

#include <string>
#include <utility>

using std::string;
using std::pair;


class RtFifo;

/**
  * @class MessageEvent
  * @brief Event describing a message transmitted between blocks
  *
  */
class MessageEvent: public RtEvent
{
  public:

	/**
	 * @brief MessageEvent constructor
	 *
	 * @param fifo      The signaling fifo
	 * @param name      The event name
	 * @param fd        The file descriptor to monitor for the event
	 * @param priority  The priority of the event
	 */
	MessageEvent(RtFifo *const fifo,
	             const string &name,
	             int32_t fd,
	             uint8_t priority = 3);

	~MessageEvent();

	/**
	 * @brief Get the message
	 *
	 * @return the message
	 */
	rt_msg_t getMessage() const {return this->message;};

	/**
	 * @brief Get the message type
	 *
	 * @return the message type
	 */
	uint8_t getMessageType() const {return this->message.type;};

	/**
	 * @brief Get the message content
	 *
	 * @return the message conetnt
	 */
	void *getData() const {return this->message.data;};
	
	/**
	 * @brief Get the message length
	 *
	 * @return the message length
	 */
	size_t getLength() const {return this->message.length;};


	virtual bool handle(void);

  protected:

	/// the message
	rt_msg_t message;

	/// the fifo
	RtFifo *const fifo;

};

#endif

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
 * @file MessageEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The message events
 *
 */

#ifndef MESSAGE_EVENT_H
#define MESSAGE_EVENT_H

#include <memory>

#include "RtEvent.h"
#include "Types.h"


namespace Rt
{


class Fifo;


/**
  * @class MessageEvent
  * @brief Event describing a message transmitted between blocks
  *
  */
class MessageEvent: public Event
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
	MessageEvent(std::shared_ptr<Fifo> &fifo,
	             const std::string &name,
	             int32_t fd,
	             uint8_t priority = 3);

	/**
	 * @brief Get the message
	 *
	 * @return the message
	 */
	template<class T>
	inline Ptr<T> getMessage() const {return this->message.release<T>();};
	inline uint8_t getMessageType() const {return this->message.type;};

	bool handle(void) override;

 protected:
	/// the message
	mutable Message message;

	/// the fifo
	const std::shared_ptr<Fifo> fifo;
};


};  // namespace Rt


#endif

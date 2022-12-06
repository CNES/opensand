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
 * @file Event.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The generic event
 */


#ifndef RT_EVENT_H
#define RT_EVENT_H

#include <chrono>
#include <string>

#include "Types.h"


namespace Rt
{


using time_point_t = std::chrono::high_resolution_clock::time_point;
using time_val_t = std::chrono::high_resolution_clock::rep;


/**
  * @class Event
  * @brief virtual class that Events are based on
  */
class Event
{
 public:
	/**
	 * @brief Event constructor
	 *
	 * @aram type       The type of event
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor for the event
	 * @param priority  The priority of the event
	 */
	Event(EventType type, const std::string &name, int32_t fd, uint8_t priority);
	virtual ~Event();

	/**
	 * @brief Get the type of the event
	 *
	 * @return the type of the event
	 */
	EventType getType(void) const {return this->type;};

	/**
	 * @brief Get the time since event trigger, in microseconds
	 *
	 * @return the time elapsed since event trigger
	 */
	time_val_t getTimeFromTrigger(void) const;
	
	/**
	 * @brief Get the time since event custom timer set, in microseconds
	 *
	 * @return the time elapsed since custom timer set
	 */
	time_val_t getTimeFromCustom(void) const;

	/**
	 * @brief Get the event priority
	 *
	 * @return the event priority
	 */
	uint8_t getPriority(void) const {return this->priority;};

	/**
	 * @brief Get the event name
	 *
	 * @return the event name
	 *
	 */
	std::string getName(void) const {return this->name;};

	/**
	 * @brief Get the file descriptor on the event
	 *
	 * @return the event file descriptor
	 */
	int32_t getFd(void) const {return this->fd;};
	
	/**
	 * @brief This event was received, handle it
	 * 
	 * @return true on success, false otherwise
	 */
	virtual bool handle(void) = 0;

	/**
	 * @brief Update the trigger time
	 *
	 */
	void setTriggerTime(void);
	
	/**
	 * @brief Update the custom time
	 *
	 */
	void setCustomTime(void) const;

	/**
	 * @brief Get the custom time interval, in microseconds, and update it
	 *
	 * @return tue custom time interval
	 */
	time_val_t getAndSetCustomTime(void) const;

	/// operator < used by sort on events priority
	bool operator <(const Event &event) const;

	/// operator == used to check if the event id corresponds
	bool operator ==(const event_id_t id) const;

	/// operator != used to check if the event id corresponds
	bool operator !=(const event_id_t id) const;

 protected:
	/// type of event, for now Message, Signal, Timer or NetSocket
	EventType type;

	/// event name
	const std::string name;

	/// Event input file descriptor
	int32_t fd;

	/// Event priority
	uint8_t priority;

	/// date, used as event trigger date
	time_point_t trigger_time;
	
	/// date, used as custom processing date
	mutable time_point_t custom_time;
};


};  // namespace Rt


#endif

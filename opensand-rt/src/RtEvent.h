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
 * @file RtEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The generic event
 *
 */


#ifndef RT_EVENT_H
#define RT_EVENT_H


#include <sys/time.h>
#include <string>

#include "stdlib.h"

#include "Types.h"

/**
  * @class RtEvent
  * @brief virtual class that Events are based on
  *
  */
class RtEvent
{
  public:

	/**
	 * @brief RtEvent constructor
	 *
	 * @aram type       The type of event
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor for the event
	 * @param priority  The priority of the event
	 */
	RtEvent(event_type_t type, const std::string &name, int32_t fd, uint8_t priority);
	virtual ~RtEvent();

	/**
	 * @brief Get the type of the event
	 *
	 * @return the type of the event
	 */
	event_type_t getType(void) const {return this->type;};

	/**
	 * @brief Get the time since event trigger
	 *
	 * @return the time elapsed since event trigger
	 */
	timeval getTimeFromTrigger(void) const;
	
	/**
	 * @brief Get the time since event custom timer set
	 *
	 * @return the time elapsed since custom timer set
	 */
	timeval getTimeFromCustom(void) const;

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
	 * @brief Get the custom time interval and update it
	 *
	 * @return tue custom time interval
	 */
	timeval getAndSetCustomTime(void) const;

	static bool compareEvents(const RtEvent* e1, const RtEvent* e2)
	{
		return ((*e1) < (*e2));
	}

	/// operator < used by sort on events priority
	bool operator<(const RtEvent &event) const
	{
 		long int delta = 100000000 * (long int) (this->priority - event.priority);
		delta += 1000000 * (this->trigger_time.tv_sec - event.trigger_time.tv_sec);
		delta += this->trigger_time.tv_usec - event.trigger_time.tv_usec;
		return (delta < 0);		
	}   

	/// operator == used to check if the event id corresponds
	bool operator==(const event_id_t id) const
	{
		return (this->fd == id);
	}

	/// operator != used to check if the event id corresponds
	bool operator!=(const event_id_t id) const
	{
		return (this->fd != id);
	}

  protected:

	/// type of event, for now Message, Signal, Timer or NetSocket
	event_type_t type;

	/// event name
	const std::string name;

	/// Event input file descriptor
	int32_t fd;

	/// Event priority
	uint8_t priority;

	/// date, used as event trigger date
	timeval trigger_time;
	
	/// date, used as custom processing date
	mutable timeval custom_time;
};

#endif

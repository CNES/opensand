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
/* $Id: Event.h,v 1.1.1.1 2013/02/04 16:16:54 cgaillardet Exp $ */



#ifndef EVENT_H
#define EVENT_H


#include <sys/time.h>
#include <string>

#include "stdlib.h"

#include "Types.h"

using std::string;


/**
  * @class Event
  * @brief virtual class that Events are based on
  *
  *
  */
class Event
{

public:


	/*
	 * Constructor
	 *
	 * @param new_name string containing the name of the event
	 *
     * @param new_input_fd file descriptor of this event, default -1
     *
     * @param new_priority priority of the event, default 0
	 */
    Event(string new_name="event", int32_t new_input_fd = -1, uint8_t new_priority = 0);
    virtual ~Event();


	/*
	 * GetType
	 *
	 * @return enum containing the type of event
	 */
    EventType GetType(void) {return this->event_type;};

	/*
	 * GetLifetime
	 *
	 * @return timecal containing the currently set event date
	 */
    timeval GetLifetime(void);

	/*
	 * GetPriority
	 *
	 * @return event priority
	 */

    uint8_t GetPriority(void) {return this->priority;};

	/*
	 * SetPriority
	 *
	 * @param new_priority new event priority
	 */
	void setPriority(uint8_t new_priority){ this->priority = new_priority;};


	/*
	 * SetCreationTime
	 *
	 * sets the internal date to the date when this method is called
	 *
	 */
	void SetCreationTime(void);

	/*
	 * SetName changes the event name
	 *
	 * @param new_name
	 *
	 */
    void SetName(string new_name) {this->name = new_name;};


    /*
	 * GetName name getter
	 *
	 * @return event name
	 *
	 */
    string GetName (void) {return this->name;};


    /*
	 * GetFd fd getter
	 *
	 * @return event file descriptor
	 *
	 */
	int32_t GetFd(void) {return this->input_fd;};

    /*
	 * SetFd fd setter
	 *
	 * @param new_fd sets event file descriptor
	 *
	 */

	void SetFd(int32_t new_fd) {this->input_fd = new_fd;};

protected:

    /// event name
    string name;

    /// date, used by default as event creation date
    timeval time_in;

    /// type of event, for now Message, Signal, Timer or NetSocket
    EventType event_type;

    /// Event priority
	uint8_t priority;

	/// Event input file descriptor
    int32_t input_fd;

private:

};

#endif
